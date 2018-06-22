/***************************************************************************
//Copyright (c) 2016, Xilinx, Inc.
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

#ifndef _XF_DILATION_HPP_
#define _XF_DILATION_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif


typedef unsigned short  uint16_t;

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

namespace xf{



template<int DEPTH>
XF_PTNAME(DEPTH) xfapplydilate3x3(XF_PTNAME(DEPTH) D1, XF_PTNAME(DEPTH) D2, XF_PTNAME(DEPTH) D3,
		XF_PTNAME(DEPTH) D4, XF_PTNAME(DEPTH) D5, XF_PTNAME(DEPTH) D6,
		XF_PTNAME(DEPTH) D7, XF_PTNAME(DEPTH) D8, XF_PTNAME(DEPTH) D9)
		{
#pragma HLS INLINE off
	XF_PTNAME(DEPTH) array[9]={D1, D2, D3, D4, D5, D6, D7, D8, D9};
#pragma HLS ARRAY_PARTITION variable = array complete dim = 1

	XF_PTNAME(DEPTH) max = array[0];

	Compute_Mask:
	for (ap_uint<4> c = 1 ; c < 9; c++)
	{
#pragma HLS unroll
		if(array[c] > max)
		{
			max = array[c];
		}
	}
	return max;
		}

template<int NPC,int DEPTH,int PLANES>
void xfDilate3x3(
		XF_PTNAME(DEPTH) OutputValues[XF_NPIXPERCYCLE(NPC)],
		XF_PTNAME(DEPTH) src_buf1[XF_NPIXPERCYCLE(NPC)+2],
		XF_PTNAME(DEPTH) src_buf2[XF_NPIXPERCYCLE(NPC)+2],
		XF_PTNAME(DEPTH) src_buf3[XF_NPIXPERCYCLE(NPC)+2])
{
#pragma HLS INLINE
	XF_PTNAME(DEPTH) out_val;
	Compute_Grad_Loop:
	for(ap_uint<5> j = 0; j < (XF_NPIXPERCYCLE(NPC)); j++)
	{
//#pragma HLS pipeline
		for(ap_uint<5> c=0,k=0;c<PLANES;c++,k+=8)
		{
#pragma HLS UNROLL

		out_val.range(k+7,k) = xfapplydilate3x3<DEPTH>(src_buf1[j].range(k+7,k), src_buf1[j+1].range(k+7,k), src_buf1[j+2].range(k+7,k),src_buf2[j].range(k+7,k), src_buf2[j+1].range(k+7,k), src_buf2[j+2].range(k+7,k)
				,src_buf3[j].range(k+7,k), src_buf3[j+1].range(k+7,k), src_buf3[j+2].range(k+7,k));
		}
		OutputValues[j]=out_val;
	}
}

template<int SRC_T, int ROWS, int COLS, int PLANES,int DEPTH, int NPC, int WORDWIDTH, int TC>
void ProcessDilate3x3(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst_mat,
		XF_SNAME(WORDWIDTH) buf[3][(COLS >> XF_BITSHIFT(NPC))], XF_PTNAME(DEPTH) src_buf1[XF_NPIXPERCYCLE(NPC)+2],
		XF_PTNAME(DEPTH) src_buf2[XF_NPIXPERCYCLE(NPC)+2], XF_PTNAME(DEPTH) src_buf3[XF_NPIXPERCYCLE(NPC)+2],
		XF_PTNAME(DEPTH) OutputValues[XF_NPIXPERCYCLE(NPC)],
		XF_SNAME(WORDWIDTH) &P0, uint16_t img_width,  uint16_t img_height, uint16_t &shift_x,  ap_uint<2> tp, ap_uint<2> mid, ap_uint<2> bottom, ap_uint<13> row, int &rd_ind, int &wr_ind)
{
#pragma HLS INLINE

	XF_SNAME(WORDWIDTH) buf0, buf1, buf2;
	uint16_t npc = XF_NPIXPERCYCLE(NPC);
	ap_uint<5> buf_size = XF_NPIXPERCYCLE(NPC) + 2;

	Col_Loop:
	for(ap_uint<13> col = 0; col < img_width; col++)
	{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
		if(row < img_height)
			buf[bottom][col] = _src_mat.data[rd_ind++]; // Read data
		else
			buf[bottom][col] = 0;

		buf0 = buf[tp][col];
		buf1 = buf[mid][col];
		buf2 = buf[bottom][col];

		if(NPC == XF_NPPC8)
		{
			xfExtractPixels<NPC, WORDWIDTH, DEPTH>(&src_buf1[2], buf0, 0);
			xfExtractPixels<NPC, WORDWIDTH, DEPTH>(&src_buf2[2], buf1, 0);
			xfExtractPixels<NPC, WORDWIDTH, DEPTH>(&src_buf3[2], buf2, 0);
		}
		else
		{
			src_buf1[2] =  buf0;
			src_buf2[2] =  buf1;
			src_buf3[2] =  buf2;
		}

		xfDilate3x3<NPC, DEPTH,PLANES>(OutputValues,src_buf1, src_buf2, src_buf3);

		if(col == 0)
		{
			shift_x = 0;
			P0 = 0;

			xfPackPixels<NPC, WORDWIDTH, DEPTH>(&OutputValues[0], P0, 1, (npc-1), shift_x);


		}
		else
		{

			xfPackPixels<NPC, WORDWIDTH, DEPTH>(&OutputValues[0], P0, 0, 1, shift_x);



			_dst_mat.data[wr_ind++] = P0;

			shift_x = 0;
			P0 = 0;

			xfPackPixels<NPC, WORDWIDTH, DEPTH>(&OutputValues[0], P0, 1, (npc-1), shift_x);


		}

		src_buf1[0] = src_buf1[buf_size-2];
		src_buf1[1] = src_buf1[buf_size-1];
		src_buf2[0] = src_buf2[buf_size-2];
		src_buf2[1] = src_buf2[buf_size-1];
		src_buf3[0] = src_buf3[buf_size-2];
		src_buf3[1] = src_buf3[buf_size-1];
	} // Col_Loop
}



template<int SRC_T, int ROWS, int COLS,int PLANES, int DEPTH, int NPC, int WORDWIDTH, int TC>
void xfDilation3x3(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst_mat,
		uint16_t img_height, uint16_t img_width)
{
	ap_uint<13> row_ind;
	ap_uint<2> tp, mid, bottom;
	ap_uint<8> buf_size = XF_NPIXPERCYCLE(NPC) + 2;
	uint16_t shift_x = 0;
	ap_uint<13> row, col;
	int rd_ind = 0, wr_ind = 0;

	XF_PTNAME(DEPTH) OutputValues[XF_NPIXPERCYCLE(NPC)];

#pragma HLS ARRAY_PARTITION variable=OutputValues complete dim=1


	XF_PTNAME(DEPTH) src_buf1[XF_NPIXPERCYCLE(NPC)+2], src_buf2[XF_NPIXPERCYCLE(NPC)+2],src_buf3[XF_NPIXPERCYCLE(NPC)+2];
#pragma HLS ARRAY_PARTITION variable=src_buf1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf2 complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf3 complete dim=1

	XF_SNAME(WORDWIDTH) P0;

	XF_SNAME(WORDWIDTH) buf[3][(COLS >> XF_BITSHIFT(NPC))];
#pragma HLS RESOURCE variable=buf core=RAM_S2P_BRAM
#pragma HLS ARRAY_PARTITION variable=buf complete dim=1
	row_ind = 1;

	Clear_Row_Loop:
	for(col = 0; col < img_width; col++)
	{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
		buf[0][col] = 0;
		buf[row_ind][col] = _src_mat.data[rd_ind++];
	}
	row_ind++;

	Row_Loop:
	for(row = 1; row < img_height+1; row++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		if(row_ind == 2)
		{
			tp = 0; mid = 1; bottom = 2;
		}
		else if(row_ind == 0)
		{
			tp = 1; mid = 2; bottom = 0;
		}
		else if(row_ind == 1)
		{
			tp = 2; mid = 0; bottom = 1;
		}

		src_buf1[0] = src_buf1[1] = 0;
		src_buf2[0] = src_buf2[1] = 0;
		src_buf3[0] = src_buf3[1] = 0;


		P0 = 0;
		ProcessDilate3x3<SRC_T,ROWS, COLS,PLANES, DEPTH, NPC, WORDWIDTH, TC>
		(_src_mat, _dst_mat, buf, src_buf1, src_buf2, src_buf3,OutputValues, P0, img_width, img_height, shift_x, tp, mid, bottom, row, rd_ind, wr_ind);


		if((NPC == XF_NPPC8) || (NPC == XF_NPPC16))
		{

			OutputValues[0] = xfapplydilate3x3<DEPTH>(
					src_buf1[buf_size-2],src_buf1[buf_size-1], 0,
					src_buf2[buf_size-2],src_buf2[buf_size-1], 0,
					src_buf3[buf_size-2],src_buf3[buf_size-1], 0);


		}
		else
		{
//			XF_PTNAME(DEPTH) out_val1;
			for(ap_uint<5> i=0,k=0;i<PLANES;i++,k+=8)
			{
//				#pragma HLS LOOP_TRIPCOUNT min=PLANES max=PLANES
				#pragma HLS UNROLL
//				#pragma HLS pipeline
				ap_uint<8> srcbuf10=src_buf1[buf_size-3].range(k+7,k);ap_uint<8> srcbuf11=src_buf1[buf_size-2].range(k+7,k);
				ap_uint<8> srcbuf20=src_buf2[buf_size-3].range(k+7,k);ap_uint<8> srcbuf21=src_buf2[buf_size-2].range(k+7,k);
				ap_uint<8> srcbuf30=src_buf3[buf_size-3].range(k+7,k);ap_uint<8> srcbuf31=src_buf3[buf_size-2].range(k+7,k);

				OutputValues[0].range(k+7,k)=xfapplydilate3x3<DEPTH>(srcbuf10, srcbuf11, 0,srcbuf20, srcbuf21, 0, srcbuf30, srcbuf31, 0);

			}
	//		OutputValues[0]=out_val1;
		}

		xfPackPixels<NPC, WORDWIDTH, DEPTH>(&OutputValues[0], P0, 0, 1, shift_x);


		_dst_mat.data[wr_ind++] = P0;


		shift_x = 0;
		P0 = 0;

		row_ind++;
		if(row_ind == 3)
		{
			row_ind = 0;
		}
	} // Row_Loop
}


#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL, "_dst_mat.data":SEQUENTIAL)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"], "_dst_mat.data"[0:"_dst_mat.size"])
#pragma SDS data mem_attribute("_src_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_dst_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)


template<int BORDER_TYPE, int SRC_T, int ROWS, int COLS,int NPC=1>
void dilate(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst_mat)
{

	assert(((_src_mat.rows <= ROWS ) && (_src_mat.cols <= COLS)) && "ROWS and COLS should be greater than input image");

	uint16_t imgwidth = _src_mat.cols >> XF_BITSHIFT(NPC);
	uint16_t imgheight = _src_mat.rows;

#pragma HLS INLINE OFF

	xfDilation3x3<SRC_T,ROWS,COLS,XF_CHANNELS(SRC_T,NPC),XF_DEPTH(SRC_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC),(COLS>>XF_BITSHIFT(NPC))>
	(_src_mat,_dst_mat,imgheight,imgwidth);

}
}
#endif //_XF_DILATION_HPP_
