/***************************************************************************
Copyright (c) 2016, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation 
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors 
may be used to endorse or promote products derived from this software 
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#ifndef _XF_THRESHOLD_HPP_
#define _XF_THRESHOLD_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

typedef unsigned short  uint16_t;
typedef unsigned char  uchar;

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

/**
 * xFThresholdKernel: Thresholds an input image and produces an output boolean image depending
 * 		upon the type of thresholding.
 * Input   : _src, _thresh_type, _binary_thresh_val,  _upper_range and _lower_range
 * Output  : _dst
 */
template<int ROWS, int COLS,int DEPTH, int NPC, int WORDWIDTH_SRC,
int WORDWIDTH_DST, int COLS_TRIP>
void xFThresholdKernel(hls::stream <XF_SNAME(WORDWIDTH_SRC)> & _src,
		hls::stream <XF_SNAME(WORDWIDTH_DST)> & _dst, bool _thresh_type,
		short _binary_thresh_val, short _upper_range, short _lower_range, unsigned short height, unsigned short width)
{
	XF_SNAME(WORDWIDTH_SRC) val_src;
	XF_SNAME(WORDWIDTH_DST) val_dst;
	XF_PTNAME(DEPTH) result, p;

	ap_uint<13> i, j, k;
	rowLoop:
	for(i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for(j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline

			val_src = (XF_SNAME(WORDWIDTH_SRC)) (_src.read());  //reading the source stream _src into val_src

			//Binary Thresholding
			if(_thresh_type == XF_THRESHOLD_TYPE_BINARY)
			{
				procBinaryLoop:
				for(k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
						k += XF_PIXELDEPTH(DEPTH))
				{
#pragma HLS unroll
					p = val_src.range(k + (XF_PIXELDEPTH(DEPTH)-1), k);	//packing a pixel value into p

					if(p > _binary_thresh_val)
						val_dst.range(k + (XF_PIXELDEPTH(DEPTH)-1), k) = 255;
					else
						val_dst.range(k + (XF_PIXELDEPTH(DEPTH)-1), k) = 0;
				}
			}

			//Range thresholding
			if(_thresh_type == XF_THRESHOLD_TYPE_RANGE)
			{
				procRangeLoop:
				for(k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
						k += XF_PIXELDEPTH(DEPTH))
				{
#pragma HLS unroll
					p = val_src.range(k + (XF_PIXELDEPTH(DEPTH)-1), k);  //packing a pixel value into p

					if((p > _upper_range) || (p < _lower_range))
					{
						val_dst.range(k + (XF_PIXELDEPTH(DEPTH)-1), k) = 0;
					}
					else
					{
						val_dst.range(k + (XF_PIXELDEPTH(DEPTH)-1), k) = 255;
					}
				}
			}
			_dst.write(val_dst);  //writing the val_dst into output stream _dst
		}
	}
}


/**
 * xFthreshold: This function acts as wrapper and calls the kernel function.
 */
template<int ROWS, int COLS,int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST>
void xFThresholding(hls::stream <XF_SNAME(WORDWIDTH_SRC)> & _src,
				 hls::stream <XF_SNAME(WORDWIDTH_DST)> & _dst,
				 bool _thresh_type, short _binary_thresh_val, short _upper_range, short _lower_range, unsigned short height, unsigned short width)
{

	width = width >> XF_BITSHIFT(NPC);

	assert((DEPTH == XF_8UP) && "Depth must be XF_8UP");
	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8) || (NPC == XF_NPPC16)) &&
			"NPC must be XF_NPPC1, XF_NPPC8 or XF_NPPC16");
	assert(((WORDWIDTH_SRC == XF_8UW) || (WORDWIDTH_SRC == XF_64UW) ||
			(WORDWIDTH_SRC == XF_128UW)) &&
			"WORDWIDTH_SRC must be XF_8UW, XF_64UW or XF_128UW");
	assert(((WORDWIDTH_DST == XF_8UW) || (WORDWIDTH_DST == XF_64UW) ||
			(WORDWIDTH_DST == XF_128UW)) &&
			"WORDWIDTH_DST must be XF_8UW, XF_64UW or XF_128UW");
	assert(((_thresh_type == XF_THRESHOLD_TYPE_BINARY) ||
			(_thresh_type == XF_THRESHOLD_TYPE_RANGE)) &&
			"_thresh_type must be either XF_THRESHOLD_TYPE_BINARY or XF_THRESHOLD_TYPE_BINARY");
	assert(((_binary_thresh_val >= 0) && (_binary_thresh_val <= 255)) &&
			"_binary_thresh_val must be with the range of 0 to 255");
	assert(((_upper_range >= 0) && (_upper_range <= 255)) &&
			"_upper_range must be with the range of 0 to 255");
	assert(((_lower_range >= 0) && (_lower_range <= 255)) &&
			"_lower_range must be with the range of 0 to 255");
	assert((_upper_range >= _lower_range)  &&
			"_upper_range must be greater than _lower_range");

	assert(((height <= ROWS ) && (width <= COLS)) && "ROWS and COLS should be greater than input image");
	if(_thresh_type == XF_THRESHOLD_TYPE_BINARY)
	{
		assert(((_upper_range == 0) && (_lower_range == 0)) &&
				"for _thresh_type of 'XF_THRESHOLD_TYPE_BINARY', the range threshold parameters '_upper_range' and '_lower_range' must be 0");
	}
	else if(_thresh_type == XF_THRESHOLD_TYPE_RANGE)
	{
		assert((_binary_thresh_val == 0) &&
				"for _thresh_type of 'XF_THRESHOLD_TYPE_RANGE', the binary threshold parameter '_binary_thresh_val' must be 0");
	}

	xFThresholdKernel<ROWS,COLS,DEPTH,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,
	(COLS>>XF_BITSHIFT(NPC))>(_src,_dst,_thresh_type,_binary_thresh_val,_upper_range,_lower_range, height,  width);
}



#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL, "_dst_mat.data":SEQUENTIAL)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"], "_dst_mat.data"[0:"_dst_mat.size"])
#pragma SDS data mem_attribute("_src_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_dst_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data data_mover("_src_mat.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_dst_mat.data":AXIDMA_SIMPLE)


template<int THRESHOLD_TYPE, int SRC_T, int ROWS, int COLS,int NPC=1>
void xFThreshold(xF::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,xF::Mat<SRC_T, ROWS, COLS, NPC> & _dst_mat,short thresh_val,short thresh_upper,short thresh_lower)
{

#pragma HLS DATAFLOW
#pragma HLS INLINE OFF
	hls::stream< XF_TNAME(SRC_T,NPC) > _src;
	hls::stream< XF_TNAME(SRC_T,NPC) > _dst;

	for(int i=0; i<_src_mat.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src_mat.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
#pragma HLS LOOP_FLATTEN off
			#pragma HLS PIPELINE
			_src.write( *(_src_mat.data + i*(_src_mat.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}


	xFThresholding<ROWS,COLS,XF_DEPTH(SRC_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(SRC_T,NPC)>(_src,_dst,THRESHOLD_TYPE,thresh_val,thresh_upper,thresh_lower,_src_mat.rows,_src_mat.cols);

	for(int i=0; i<_dst_mat.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst_mat.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
#pragma HLS LOOP_FLATTEN off
			*(_dst_mat.data + i*(_dst_mat.cols>>(XF_BITSHIFT(NPC))) +j) = _dst.read();
		}
	}
}

#endif
