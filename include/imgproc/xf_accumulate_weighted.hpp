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

#ifndef _XF_ACCUMULATE_WEIGHTED_HPP_
#define _XF_ACCUMULATE_WEIGHTED_HPP_

#include "hls_stream.h"
#include "common/xf_common.h"

#ifndef XF_IN_STEP
#define XF_IN_STEP  8
#endif
#ifndef XF_OUT_STEP
#define XF_OUT_STEP 16
#endif
namespace xf {
template<int ROWS, int COLS, int NPC, int DEPTH_SRC, int DEPTH_DST, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
int AccumulateWeightedKernel(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _src1,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _src2,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _dst,
		float _alpha,uint16_t height,uint16_t width)
{

	ap_uint<13> i,j,k,l;
	ap_uint<24> temp  = (_alpha * ((1<<23)-1));
	ap_uint<24> temp1 = ((1<<23)-1) - temp + 1;

	XF_SNAME(WORDWIDTH_DST) pxl_pack_out;
	XF_SNAME(WORDWIDTH_SRC)  pxl_pack1, pxl_pack2;
	RowLoop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN OFF
		ColLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline

			pxl_pack1 = (XF_SNAME(WORDWIDTH_SRC))(_src1.read());
			pxl_pack2 = (XF_SNAME(WORDWIDTH_SRC))(_src2.read());
			ProcLoop:
			for( k = 0, l = 0; k < (8<<XF_BITSHIFT(NPC)); k+=XF_IN_STEP, l+=XF_OUT_STEP)
			{
				XF_PTNAME(DEPTH_SRC) pxl1 = pxl_pack1.range(k+7, k);
				XF_PTNAME(DEPTH_SRC) pxl2 = pxl_pack2.range(k+7, k);

				ap_uint<40> firstcmp  = pxl1 * temp;
				ap_uint<40> secondcmp = pxl2 * temp1;

				XF_PTNAME(DEPTH_DST) t = (firstcmp + secondcmp) >> 23;

				pxl_pack_out.range(l+XF_OUT_STEP-1, l) = t;
			}

			_dst.write((XF_SNAME(WORDWIDTH_DST))pxl_pack_out);
		}
	}
	return 0;
}

template<int ROWS, int COLS, int NPC, int DEPTH_SRC, int DEPTH_DST, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void kernel_process(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _src1,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _src2,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _dst,
		float _alpha,uint16_t height,uint16_t width)
{


	width = width  >> XF_BITSHIFT(NPC);
	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			"NPC must be XF_NPPC1 or XF_NPPC8");
	assert(((WORDWIDTH_SRC == XF_8UW) || (WORDWIDTH_SRC == XF_64UW)) &&
			"WORDWIDTH_SRC must be XF_8UW or XF_128UW ");
	assert(((WORDWIDTH_DST == XF_16UW) || (WORDWIDTH_DST == XF_128UW)) &&
			"WORDWIDTH_DST must be XF_16UW or XF_128UW");
	assert((_alpha >= 0.0) && (_alpha <= 1.0) && "Alpha value must be from 0.0 to 1.0");
	assert(((height <= ROWS ) && (width <= COLS)) && "ROWS and COLS should be greater than input image");


	 AccumulateWeightedKernel<ROWS,COLS,NPC,DEPTH_SRC,DEPTH_DST,WORDWIDTH_SRC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>
			 (_src1, _src2, _dst, _alpha, height, width);


}
//#pragma SDS data data_mover("src1.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("src2.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("dst.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("src2.data":SEQUENTIAL)
#pragma SDS data copy("src1.data"[0:"src1.size"])
#pragma SDS data copy("src2.data"[0:"src2.size"])
#pragma SDS data access_pattern("dst.data":SEQUENTIAL)
#pragma SDS data copy("dst.data"[0:"dst.size"])
template< int SRC_T,int DST_T, int ROWS, int COLS, int NPC = 1>
void accumulateWeighted(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & src2, xf::Mat<DST_T, ROWS, COLS, NPC> & dst,float alpha)
{
		hls::stream<XF_TNAME(SRC_T, NPC)> _src1;
		hls::stream<XF_TNAME(SRC_T, NPC)> _src2;
		hls::stream<XF_TNAME(DST_T, NPC)> _dst;
#pragma HLS INLINE OFF
#pragma HLS dataflow
		Read_Loop:
		for(int i=0; i<src1.rows;i++)
		{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
			for(int j=0; j<(src1.cols)>>(XF_BITSHIFT(NPC));j++)
			{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
				#pragma HLS PIPELINE
				#pragma HLS loop_flatten off
				_src1.write( *(src1.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
				_src2.write( *(src2.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
			}
		}


		kernel_process<ROWS, COLS,NPC ,XF_DEPTH(SRC_T,NPC),XF_DEPTH(DST_T,NPC), XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(DST_T,NPC)>(_src1, _src2, _dst,alpha,src1.rows, src1.cols);

		for(int i=0; i<dst.rows;i++)
		{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
			for(int j=0; j<(dst.cols)>>(XF_BITSHIFT(NPC));j++)
			{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
				#pragma HLS PIPELINE
				#pragma HLS loop_flatten off
				*(dst.data + i*(dst.cols>>(XF_BITSHIFT(NPC))) +j) = _dst.read();

			}
		}
}
}
#endif//_XF_ACCUMULATE_WEIGHTED_HPP_
