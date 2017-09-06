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

template<int NPC,int DEPTH>
void xfDilate3x3(
		XF_PTNAME(DEPTH) OutputValues[XF_NPIXPERCYCLE(NPC)],
		XF_PTNAME(DEPTH) src_buf1[XF_NPIXPERCYCLE(NPC)+2],
		XF_PTNAME(DEPTH) src_buf2[XF_NPIXPERCYCLE(NPC)+2],
		XF_PTNAME(DEPTH) src_buf3[XF_NPIXPERCYCLE(NPC)+2])
{
#pragma HLS INLINE

	Compute_Grad_Loop:
	for(ap_uint<5> j = 0; j < XF_NPIXPERCYCLE(NPC); j++)
	{
#pragma HLS UNROLL
		OutputValues[j] = xfapplydilate3x3<DEPTH>(
				src_buf1[j], src_buf1[j+1], src_buf1[j+2],
				src_buf2[j], src_buf2[j+1], src_buf2[j+2],
				src_buf3[j], src_buf3[j+1],	src_buf3[j+2]);
	}
}

template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH, int TC>
void ProcessDilate3x3(hls::stream< XF_SNAME(WORDWIDTH) > & _src_mat, 
		hls::stream< XF_SNAME(WORDWIDTH) > & _out_mat,                
		XF_SNAME(WORDWIDTH) buf[3][(COLS >> XF_BITSHIFT(NPC))], XF_PTNAME(DEPTH) src_buf1[XF_NPIXPERCYCLE(NPC)+2],
		XF_PTNAME(DEPTH) src_buf2[XF_NPIXPERCYCLE(NPC)+2], XF_PTNAME(DEPTH) src_buf3[XF_NPIXPERCYCLE(NPC)+2], 
		XF_PTNAME(DEPTH) OutputValues[XF_NPIXPERCYCLE(NPC)], 
		XF_SNAME(WORDWIDTH) &P0, uint16_t img_width,  uint16_t img_height, uint16_t &shift_x,  ap_uint<2> tp, ap_uint<2> mid, ap_uint<2> bottom, ap_uint<13> row)
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
			buf[bottom][col] = _src_mat.read(); // Read data
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

		xfDilate3x3<NPC, DEPTH>(OutputValues,
				src_buf1, src_buf2, src_buf3);

		if(col == 0)
		{
			shift_x = 0;
			P0 = 0;

			xfPackPixels<NPC, WORDWIDTH, DEPTH>(&OutputValues[0], P0, 1, (npc-1), shift_x);


		}
		else
		{

			xfPackPixels<NPC, WORDWIDTH, DEPTH>(&OutputValues[0], P0, 0, 1, shift_x);



			_out_mat.write(P0);

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



template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH, int TC>
void xfDilation3x3(hls::stream< XF_SNAME(WORDWIDTH) > &_src_mat,
		hls::stream< XF_SNAME(WORDWIDTH) > &_out_mat,
		uint16_t img_height, uint16_t img_width)
{
	ap_uint<13> row_ind;
	ap_uint<2> tp, mid, bottom;
	ap_uint<8> buf_size = XF_NPIXPERCYCLE(NPC) + 2;
	uint16_t shift_x = 0;
	ap_uint<13> row, col;

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
		buf[row_ind][col] = _src_mat.read();
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
		ProcessDilate3x3<ROWS, COLS, DEPTH, NPC, WORDWIDTH, TC>
		(_src_mat, _out_mat, buf, src_buf1, src_buf2, src_buf3,OutputValues, P0, img_width, img_height, shift_x, tp, mid, bottom, row);


		if((NPC == XF_NPPC8) || (NPC == XF_NPPC16))
		{

			OutputValues[0] = xfapplydilate3x3<DEPTH>(
					src_buf1[buf_size-2],src_buf1[buf_size-1], 0,
					src_buf2[buf_size-2],src_buf2[buf_size-1], 0,
					src_buf3[buf_size-2],src_buf3[buf_size-1], 0);


		}
		else
		{
			OutputValues[0] = xfapplydilate3x3<DEPTH>(
					src_buf1[buf_size-3], src_buf1[buf_size-2], 0,
					src_buf2[buf_size-3], src_buf2[buf_size-2], 0,
					src_buf3[buf_size-3], src_buf3[buf_size-2], 0);


		}

		xfPackPixels<NPC, WORDWIDTH, DEPTH>(&OutputValues[0], P0, 0, 1, shift_x);


		_out_mat.write(P0);


		shift_x = 0;
		P0 = 0;

		row_ind++;
		if(row_ind == 3)
		{
			row_ind = 0;
		}
	} // Row_Loop
}

template<int ROWS,int COLS,int DEPTH,int NPC,int WORDWIDTH>
void xFDilation( 
		hls::stream< XF_SNAME(WORDWIDTH) > &_src, 
		hls::stream< XF_SNAME(WORDWIDTH) > &_dst, 
		int _border_type,uint16_t imgheight,uint16_t imgwidth)
{


	assert(((imgheight <= ROWS ) && (imgwidth <= COLS)) && "ROWS and COLS should be greater than input image");

	imgwidth = imgwidth >> XF_BITSHIFT(NPC);


	xfDilation3x3<ROWS,COLS,DEPTH,NPC,WORDWIDTH,(COLS>>XF_BITSHIFT(NPC))>(_src, _dst,imgheight,imgwidth);


}


#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL, "_dst_mat.data":SEQUENTIAL)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"], "_dst_mat.data"[0:"_dst_mat.size"])
#pragma SDS data mem_attribute("_src_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_dst_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
//#pragma SDS data data_mover("_src_mat.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst_mat.data":AXIDMA_SIMPLE)

template<int BORDER_TYPE, int SRC_T, int ROWS, int COLS,int NPC=1>
void dilate(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst_mat)
{



	hls::stream< XF_TNAME(SRC_T,NPC)> _src;
	hls::stream< XF_TNAME(SRC_T,NPC)> _dst;
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW

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


	xFDilation<ROWS,COLS,XF_DEPTH(SRC_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC)>(_src,_dst,BORDER_TYPE,_src_mat.rows,_src_mat.cols);

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
}
#endif //_XF_DILATION_HPP_
