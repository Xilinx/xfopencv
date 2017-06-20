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

#ifndef _XF_ACCUMULATE_SQUARED_HPP_
#define _XF_ACCUMULATE_SQUARED_HPP_

#include "hls_stream.h"
#include "common/xf_common.h"


#ifndef XF_IN_STEP
#define XF_IN_STEP  8
#endif
#ifndef XF_OUT_STEP
#define XF_OUT_STEP 16
#endif

template<int ROWS, int COLS, int NPC, int DEPTH_SRC, int DEPTH_DST, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
int AcuumulateSquaredKernel(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _src1,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _src2,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _dst,uint16_t height,uint16_t width)
{

	ap_uint<13> i,j,k,l;
	#pragma HLS DATAFLOW
	XF_SNAME(WORDWIDTH_DST) pxl_pack_out;
	XF_SNAME(WORDWIDTH_SRC) pxl_pack1, pxl_pack2;
	RowLoop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
			int y;
			pxl_pack1 = (XF_SNAME(WORDWIDTH_SRC))(_src1.read());
			pxl_pack2 = (XF_SNAME(WORDWIDTH_SRC))(_src2.read());
			ProcLoop:
			for( k = 0, l = 0; k < (8<<XF_BITSHIFT(NPC)); k+=XF_IN_STEP, l+=XF_OUT_STEP)
			{
#pragma HLS UNROLL
				XF_PTNAME(DEPTH_SRC) pxl1 = pxl_pack1.range(k+7, k);
				XF_PTNAME(DEPTH_SRC) pxl2 = pxl_pack2.range(k+7, k);

				XF_PTNAME(DEPTH_DST) t = (pxl1 * pxl1);
				pxl_pack_out.range(l+XF_OUT_STEP-1, l) = t + pxl2;
			}

			_dst.write((XF_SNAME(WORDWIDTH_DST))pxl_pack_out);
		}
	}
	return 0;
}

template<int ROWS, int COLS, int NPC, int DEPTH_SRC, int DEPTH_DST, int WORDWIDTH_SRC, int WORDWIDTH_DST>
int xFAccumulateSquared(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>&  _src1,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>&  _src2,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>&  _accum, uint16_t height, uint16_t width)
{

//#pragma HLS license key=IPAUVIZ_CV_BASIC
	assert(DEPTH_SRC == XF_8UP && "Input Depth must be XF_8UP");
	assert(DEPTH_DST == XF_16UP && "Input Depth must be XF_16UP");
	assert(((height <= ROWS ) && (width <= COLS)) && "ROWS and COLS should be greater than input image");
	width=width>>XF_BITSHIFT(NPC);

	    AcuumulateSquaredKernel<ROWS,COLS,NPC,DEPTH_SRC,DEPTH_DST,WORDWIDTH_SRC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(_src1, _src2, _accum, height,width);
}
#pragma SDS data data_mover("src1.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("src2.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("dst.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("src2.data":SEQUENTIAL)
#pragma SDS data copy("src1.data"[0:"src1.size"])
#pragma SDS data copy("src2.data"[0:"src2.size"])
#pragma SDS data access_pattern("dst.data":SEQUENTIAL)
#pragma SDS data copy("dst.data"[0:"dst.size"])
template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC=1>
void xFaccumulateSquare(xF::Mat<SRC_T, ROWS, COLS, NPC> & src1, xF::Mat<SRC_T, ROWS, COLS, NPC> & src2, xF::Mat<DST_T, ROWS, COLS, NPC> & dst)
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

		xFAccumulateSquared<ROWS,COLS,NPC,XF_DEPTH(SRC_T,NPC),XF_DEPTH(DST_T,NPC),XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(_src1,_src2,_dst,src1.rows,src1.cols);

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
#endif//_AU_ACCUMULATE_SQUARED_HPP_
