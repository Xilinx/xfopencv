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

#ifndef _XF_LUT_HPP_
#define _XF_LUT_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.h"

/**
 *  xfLUTKernel: The Table Lookup Image Kernel.
 *  This kernel uses each pixel in an image to index into a LUT
 *  and put the indexed LUT value into the output image.
 *	Input		 : _src, _lut
 *	Output		 : _dst
 */
template <int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,
int WORDWIDTH_DST, int COLS_TRIP>
void xFLUTKernel(
		hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src,
		hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _dst,
		hls::stream< unsigned char >& _lutptr,uint16_t height,uint16_t width)
{
	width=width>>XF_BITSHIFT(NPC);
    ap_uint<13> i,j,k;
    uchar_t npc = XF_NPIXPERCYCLE(NPC);
	uchar_t _lut[256];

	for( i=0;i<256;i++)
	{
		_lut[i]=_lutptr.read();
	}

	uchar_t lut[npc][256];

	if(NPC)
	{
#pragma HLS ARRAY_PARTITION variable=lut complete dim=1
	}

	// creating a temporary buffers for Resource Optimization and Performance optimization
	if(NPC)
	{
		for( i = 0; i < npc; i++)
		{
			for( j = 0; j < 256; j++)
			{
				lut[i][j] = _lut[j];
			}
		}
	}

	XF_SNAME(WORDWIDTH_SRC) val_src;
	XF_SNAME(WORDWIDTH_DST) val_dst;

	rowLoop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline

			val_src = (XF_SNAME(WORDWIDTH_SRC)) (_src.read());  // read the data from the input stream

			uchar_t l = 0;

			procLoop:
			for ( k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
					k += XF_PIXELDEPTH(DEPTH))
			{
#pragma HLS unroll
				XF_PTNAME(DEPTH) p;
				p = val_src.range(k + (XF_PIXELDEPTH(DEPTH)-1), k);     	// Get bits from certain range of positions.

				// for Normal operation
				if(NPC == XF_NPPC1)
				{
					val_dst.range(k + (XF_PIXELDEPTH(DEPTH)-1), k) = _lut[p];	// Set bits in a range of positions.
				}

				// resource optimization and performance optimization
				else
				{
					val_dst.range(k + (XF_PIXELDEPTH(DEPTH)-1), k) = lut[l][p];	// Set bits in a range of positions.
					l++;				
				}

			}
			_dst.write(val_dst);		// write the data into the output stream
		}
	}
}



template <int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void LUT(hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src,hls::stream< XF_SNAME(WORDWIDTH_DST) >& _dst,hls::stream< unsigned char >& _lut,uint16_t height,uint16_t width)
{


	assert((DEPTH == XF_8UP) && "Depth must be AU_8UP");
	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be AU_NPPC1 or AU_NPPC8");
	assert(((WORDWIDTH_SRC == XF_8UW) || (WORDWIDTH_SRC == XF_64UW)) &&
			"WORDWIDTH_SRC must be AU_8UW or AU_64UW ");
	assert(((WORDWIDTH_DST == XF_8UW) || (WORDWIDTH_DST == XF_64UW)) &&
			"WORDWIDTH_DST must be AU_8UW or AU_64UW");
	assert(((height <= ROWS ) && (width <= COLS)) && "ROWS and COLS should be greater than input image");

	xFLUTKernel<ROWS,COLS,DEPTH,NPC,WORDWIDTH_SRC, WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(_src, _dst, _lut, height, width);
}
#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_dst.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover(_lut:AXIDMA_SIMPLE)
								
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data access_pattern(_lut:SEQUENTIAL)

#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])
#pragma SDS data copy(_lut[0:256])

template <int SRC_T, int ROWS, int COLS,int NPC=1>
void xFLUT(xF::Mat<SRC_T, ROWS, COLS, NPC> & _src, xF::Mat<SRC_T, ROWS, COLS, NPC> & _dst,unsigned char* _lut)
{

	hls::stream< XF_TNAME(SRC_T,NPC)> _src_stream;
	hls::stream< XF_TNAME(SRC_T,NPC)> _dst_stream;
	hls::stream< unsigned char> _lut_stream;
#pragma HLS INLINE OFF
#pragma HLS dataflow

	Read_Loop:
	for(int i=0; i<_src.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			_src_stream.write( *(_src.data + i*(_src.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	Read_LUT:
	for(int i=0;i<256;i++)
	{
#pragma HLS pipeline ii=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=256
		_lut_stream.write(*(_lut + i));
	}

	LUT<ROWS,COLS,XF_DEPTH(SRC_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC)>(_src_stream, _dst_stream, _lut_stream, _src.rows, _src.cols);

	for(int i=0; i<_dst.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_dst.data + i*(_dst.cols>>(XF_BITSHIFT(NPC))) +j) = _dst_stream.read();

		}
	}

}

#endif
