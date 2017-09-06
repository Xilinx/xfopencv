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
HOWEVER CXFSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#ifndef _XF_INTEGRAL_IMAGE_HPP_
#define _XF_INTEGRAL_IMAGE_HPP_

typedef unsigned short  uint16_t;

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

namespace xf{

template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC>
void XFIntegralImageKernel(
		hls::stream < XF_SNAME(WORDWIDTH_SRC) >& _in,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& _out, uint16_t height, uint16_t width)
{
#pragma hls inline
	XF_SNAME(XF_32UW) linebuff[COLS];

	uint32_t cur_sum;
	ap_uint<22> prev;
	ap_uint<13> i, j;
	RowLoop:
	for(i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS

		XF_SNAME(XF_8UW) val;cur_sum = 0; prev = 0;

		ColLoop:
		for(j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
#pragma HLS LOOP_FLATTEN OFF
#pragma HLS PIPELINE

			val = (XF_SNAME(XF_8UW))_in.read();

			prev = prev + val;

			if(i == 0)
			{
				cur_sum = prev;
			}
			else
			{
				cur_sum = (prev + linebuff[j]);
			}

			linebuff[j] = cur_sum;
			_out.write(cur_sum);
		}//ColLoop

	}//rowLoop
}

template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFIntegralImg(
		hls::stream < XF_SNAME(WORDWIDTH_SRC) >&  _src,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& _dst, uint16_t height, uint16_t width)
{
	assert((( NPC == XF_NPPC8 )||( NPC == XF_NPPC1 )) && "NPC must be XF_NPPC1 or XF_NPPC8 or XF_NPPC16");
	assert(((height <= ROWS ) && (width <= COLS)) && "ROWS and COLS should be greater than input image");

#pragma HLS INLINE

		XFIntegralImageKernel<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(_src, _dst, height, width);

}
// XFIntegralImage


#pragma SDS data mem_attribute("_src_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_dst_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL, "_dst_mat.data":SEQUENTIAL)
//#pragma SDS data data_mover("_src_mat.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst_mat.data":AXIDMA_SIMPLE)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"], "_dst_mat.data"[0:"_dst_mat.size"])

template<int SRC_TYPE,int DST_TYPE, int ROWS, int COLS, int NPC>
void integral(xf::Mat<SRC_TYPE, ROWS, COLS, NPC> & _src_mat, xf::Mat<DST_TYPE, ROWS, COLS, NPC> & _dst_mat)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW

	hls::stream< XF_TNAME(SRC_TYPE,NPC) > in_stream;
	hls::stream< XF_TNAME(DST_TYPE,NPC) > out_stream;
	for(int i=0; i<_src_mat.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src_mat.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
#pragma HLS LOOP_FLATTEN off
			#pragma HLS PIPELINE
			in_stream.write( *(_src_mat.data + i*(_src_mat.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}


	xFIntegralImg<ROWS,COLS,NPC,XF_WORDWIDTH(SRC_TYPE,NPC),XF_WORDWIDTH(DST_TYPE,NPC)>(in_stream,out_stream,_src_mat.rows,_src_mat.cols);

	for(int i=0; i<_dst_mat.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst_mat.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
#pragma HLS LOOP_FLATTEN off
			*(_dst_mat.data + i*(_dst_mat.cols>>(XF_BITSHIFT(NPC))) +j) = out_stream.read();
		}
	}
}
}

#endif//_XF_INTEGRAL_IMAGE_HPP_

