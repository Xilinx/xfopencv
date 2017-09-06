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

#ifndef _XF_SCHARR_HPP_
#define _XF_SCHARR_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

typedef unsigned short  uint16_t;

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"


namespace xf{

/********************************************************************
 * xFGradientX : X-Gradient Computation
 *
 * -------------
 * |-3	0   3|
 * |-10	0  10|
 * |-3	0	3|
 * -------------
 ********************************************************************/
template<int DEPTH_SRC, int DEPTH_DST>
XF_PTNAME(DEPTH_DST) xFGradientX(XF_PTNAME(DEPTH_SRC) vt0, XF_PTNAME(DEPTH_SRC) vt1,
		XF_PTNAME(DEPTH_SRC) vt2, XF_PTNAME(DEPTH_SRC) vm0,
		XF_PTNAME(DEPTH_SRC) vm1, XF_PTNAME(DEPTH_SRC) vm2,
		XF_PTNAME(DEPTH_SRC) vb0, XF_PTNAME(DEPTH_SRC) vb1,
		XF_PTNAME(DEPTH_SRC) vb2)
		{
#pragma HLS INLINE off
	XF_PTNAME(DEPTH_DST) temp_g;
	XF_PTNAME(DEPTH_DST) M00 = (XF_PTNAME(DEPTH_DST))vm2 << 3;
	M00 = M00 + vm2 + vm2;
	XF_PTNAME(DEPTH_DST) M01 = (XF_PTNAME(DEPTH_DST))vm0 << 3;
	M01 = M01 + vm0 + vm0;
	XF_PTNAME(DEPTH_DST) A00 = (XF_PTNAME(DEPTH_DST))vt2 << 1;
	A00 = A00 + vt2;
	XF_PTNAME(DEPTH_DST) A01 = (XF_PTNAME(DEPTH_DST))vb2 << 1;
	A01 = A01 + vb2;
	A00 = A00 + A01;
	XF_PTNAME(DEPTH_DST) S00 = (XF_PTNAME(DEPTH_DST))vt0 << 1;
	S00 = S00 + vt0;
	XF_PTNAME(DEPTH_DST) S01 = (XF_PTNAME(DEPTH_DST))vb0 << 1;
	S01 = S01 + vb0;
	S00 = S00 + S01;
	temp_g = M00 - M01;
	temp_g = temp_g + A00;
	temp_g = temp_g - S00;
	return temp_g;
		}


/**********************************************************************
 *  xFGradientY : Y-Gradient Computation
 *
 * -------------
 * |-3	-10 -3|
 * | 0	 0	 0|
 * | 3	 10	 3|
 * -------------
 **********************************************************************/
template<int DEPTH_SRC, int DEPTH_DST>
XF_PTNAME(DEPTH_DST) xFGradientY(XF_PTNAME(DEPTH_SRC) vt0, XF_PTNAME(DEPTH_SRC) vt1,
		XF_PTNAME(DEPTH_SRC) vt2, XF_PTNAME(DEPTH_SRC) vm0,
		XF_PTNAME(DEPTH_SRC) vm1, XF_PTNAME(DEPTH_SRC) vm2,
		XF_PTNAME(DEPTH_SRC) vb0, XF_PTNAME(DEPTH_SRC) vb1,
		XF_PTNAME(DEPTH_SRC) vb2)
		{
#pragma HLS INLINE off
	XF_PTNAME(DEPTH_DST) temp_g;
	XF_PTNAME(DEPTH_DST) M00 = (XF_PTNAME(DEPTH_DST))vb1 << 3;
	M00 = M00 + vb1 + vb1;
	XF_PTNAME(DEPTH_DST) M01 = (XF_PTNAME(DEPTH_DST))vt1 << 3;
	M01 = M01 + vt1 + vt1;
	XF_PTNAME(DEPTH_DST) A00 = (XF_PTNAME(DEPTH_DST))vb0 << 1;
	A00 = A00 + vb0;
	XF_PTNAME(DEPTH_DST) A01 = (XF_PTNAME(DEPTH_DST))vb2 << 1;
	A01 = A01 + vb2;
	A00 = A00 + A01;
	XF_PTNAME(DEPTH_DST) S00 = (XF_PTNAME(DEPTH_DST))vt0 << 1;
	S00 = S00 + vt0;
	XF_PTNAME(DEPTH_DST) S01 = (XF_PTNAME(DEPTH_DST))vt2 << 1;
	S01 = S01 + vt2;
	S00 = S00 + S01;
	temp_g = M00 - M01;
	temp_g = temp_g + A00;
	temp_g = temp_g - S00;
	return temp_g;
		}
/**
 * xFScharr3x3 : Applies the mask and Computes the gradient values
 *
 */
template<int NPC,int DEPTH_SRC,int DEPTH_DST>
void xFScharr3x3(XF_PTNAME(DEPTH_DST) GradientvaluesX[XF_NPIXPERCYCLE(NPC)], XF_PTNAME(DEPTH_DST) GradientvaluesY[XF_NPIXPERCYCLE(NPC)],
		XF_PTNAME(DEPTH_SRC) src_buf1[XF_NPIXPERCYCLE(NPC)+2], XF_PTNAME(DEPTH_SRC) src_buf2[XF_NPIXPERCYCLE(NPC)+2], XF_PTNAME(DEPTH_SRC) src_buf3[XF_NPIXPERCYCLE(NPC)+2])
{
#pragma HLS INLINE off

	Compute_Grad_Loop:
	for(ap_uint<5> j = 0; j < XF_NPIXPERCYCLE(NPC); j++)
	{
#pragma HLS UNROLL
		GradientvaluesX[j] = xFGradientX<DEPTH_SRC, DEPTH_DST>(
				src_buf1[j], src_buf1[j+1], src_buf1[j+2],
				src_buf2[j], src_buf2[j+1], src_buf2[j+2],
				src_buf3[j], src_buf3[j+1],	src_buf3[j+2]);

		GradientvaluesY[j] = xFGradientY<DEPTH_SRC, DEPTH_DST>(
				src_buf1[j], src_buf1[j+1], src_buf1[j+2],
				src_buf2[j], src_buf2[j+1], src_buf2[j+2],
				src_buf3[j], src_buf3[j+1],	src_buf3[j+2]);
	}
}


/**************************************************************************************
 * ProcessScharr3x3 : Computes gradients for the column input data
 **************************************************************************************/
template<int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void ProcessScharr3x3(hls::stream< XF_SNAME(WORDWIDTH_SRC) > & _src_mat,
		hls::stream< XF_SNAME(WORDWIDTH_DST) > & _gradx_mat, hls::stream< XF_SNAME(WORDWIDTH_DST) > & _grady_mat,
		XF_SNAME(WORDWIDTH_SRC) buf[3][(COLS >> XF_BITSHIFT(NPC))], XF_PTNAME(DEPTH_SRC) src_buf1[XF_NPIXPERCYCLE(NPC)+2],
		XF_PTNAME(DEPTH_SRC) src_buf2[XF_NPIXPERCYCLE(NPC)+2], XF_PTNAME(DEPTH_SRC) src_buf3[XF_NPIXPERCYCLE(NPC)+2],
		XF_PTNAME(DEPTH_DST) GradientValuesX[XF_NPIXPERCYCLE(NPC)], XF_PTNAME(DEPTH_DST) GradientValuesY[XF_NPIXPERCYCLE(NPC)],
		XF_SNAME(WORDWIDTH_DST) &P0, XF_SNAME(WORDWIDTH_DST) &P1, uint16_t img_width, uint16_t img_height, ap_uint<13> row_ind, uint16_t &shift_x, uint16_t &shift_y,
		ap_uint<2> tp, ap_uint<2> mid, ap_uint<2> bottom, ap_uint<13> row)
{
#pragma HLS INLINE

	XF_SNAME(WORDWIDTH_SRC) buf0, buf1, buf2;
	uint16_t npc = XF_NPIXPERCYCLE(NPC);
	ap_uint<5> buf_size = XF_NPIXPERCYCLE(NPC) + 2;

	Col_Loop:
	for(ap_uint<13> col = 0; col < img_width; col++)
	{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
		if(row < img_height)
			buf[row_ind][col] = _src_mat.read(); // Read data
		else
			buf[bottom][col] = 0;
		buf0 = buf[tp][col];
		buf1 = buf[mid][col];
		buf2 = buf[bottom][col];

		if(NPC == XF_NPPC8)
		{
			xfExtractPixels<NPC, WORDWIDTH_SRC, DEPTH_SRC>(&src_buf1[2], buf0, 0);
			xfExtractPixels<NPC, WORDWIDTH_SRC, DEPTH_SRC>(&src_buf2[2], buf1, 0);
			xfExtractPixels<NPC, WORDWIDTH_SRC, DEPTH_SRC>(&src_buf3[2], buf2, 0);
		}
		else
		{
			src_buf1[2] = buf0;
			src_buf2[2] = buf1;
			src_buf3[2] = buf2;
		}


		xFScharr3x3<NPC, DEPTH_SRC, DEPTH_DST>(GradientValuesX, GradientValuesY,
				src_buf1, src_buf2, src_buf3);

		if(col == 0)
		{
			shift_x = 0; shift_y = 0;
			P0 = 0; P1 = 0;


			xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesX[0], P0, 1, (npc-1), shift_x);
			xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesY[0], P1, 1, (npc-1), shift_y);

		}
		else
		{


			xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesX[0], P0, 0, 1, shift_x);
			xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesY[0], P1, 0, 1, shift_y);


			_gradx_mat.write(P0);
			_grady_mat.write(P1);

			shift_x = 0; shift_y = 0;
			P0 = 0; P1 = 0;


			xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesX[0], P0, 1, (npc-1), shift_x);
			xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesY[0], P1, 1, (npc-1), shift_y);

		}

		src_buf1[0] = src_buf1[buf_size-2];
		src_buf1[1] = src_buf1[buf_size-1];
		src_buf2[0] = src_buf2[buf_size-2];
		src_buf2[1] = src_buf2[buf_size-1];
		src_buf3[0] = src_buf3[buf_size-2];
		src_buf3[1] = src_buf3[buf_size-1];
	} // Col_Loop
}

template<int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void xFScharrFilterKernel(hls::stream< XF_SNAME(WORDWIDTH_SRC) > &_src_mat,
		hls::stream< XF_SNAME(WORDWIDTH_DST) > &_gradx_mat,
		hls::stream< XF_SNAME(WORDWIDTH_DST) > &_grady_mat, uint16_t img_height, uint16_t img_width)
{
	ap_uint<13> row_ind;
	ap_uint<2> tp, mid, bottom;
	ap_uint<8> buf_size = XF_NPIXPERCYCLE(NPC) + 2;
	uint16_t shift_x = 0, shift_y = 0;
	ap_uint<13> row, col;

	XF_PTNAME(DEPTH_DST) GradientValuesX[XF_NPIXPERCYCLE(NPC)];										// X-Gradient result buffer
	XF_PTNAME(DEPTH_DST) GradientValuesY[XF_NPIXPERCYCLE(NPC)];										// Y-Gradient result buffer
#pragma HLS ARRAY_PARTITION variable=GradientValuesX complete dim=1
#pragma HLS ARRAY_PARTITION variable=GradientValuesY complete dim=1

	XF_PTNAME(DEPTH_SRC) src_buf1[XF_NPIXPERCYCLE(NPC)+2], src_buf2[XF_NPIXPERCYCLE(NPC)+2],		// Temporary buffers to hold input data for processing
			src_buf3[XF_NPIXPERCYCLE(NPC)+2];
#pragma HLS ARRAY_PARTITION variable=src_buf1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf2 complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf3 complete dim=1

	XF_SNAME(WORDWIDTH_DST) P0, P1;																	// Output data is packed
	// Line buffer to hold image data
	XF_SNAME(WORDWIDTH_SRC) buf[3][(COLS >> XF_BITSHIFT(NPC))];													// Line buffer
#pragma HLS RESOURCE variable=buf core=RAM_S2P_BRAM
#pragma HLS ARRAY_PARTITION variable=buf complete dim=1
	row_ind = 1;

	Clear_Row_Loop:
	for(col = 0; col < img_width; col++)															// Top row border care
	{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
		buf[0][col] = 0;
		buf[row_ind][col] = _src_mat.read(); 														// Read data
	}
	row_ind++;

	Row_Loop:																						// Process complete image
	for(row = 1; row < img_height+1; row++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		if(row_ind == 2)																			// Indexes to hold maintain the row index
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

		/***********		Process complete row			**********/
		P0 = P1 = 0;
		ProcessScharr3x3<ROWS, COLS, DEPTH_SRC, DEPTH_DST, NPC, WORDWIDTH_SRC, WORDWIDTH_DST, TC>
		(_src_mat, _gradx_mat, _grady_mat, buf, src_buf1, src_buf2, src_buf3, GradientValuesX, GradientValuesY, P0, P1, img_width, img_height, row_ind, shift_x, shift_y, tp, mid, bottom, row);

		/*			Last column border care	for RO & PO Case			*/
		if((NPC == XF_NPPC8) || (NPC == XF_NPPC16))
		{
			//	Compute gradient at last column
			GradientValuesX[0] = xFGradientX<DEPTH_SRC, DEPTH_DST>(
					src_buf1[buf_size-2], src_buf1[buf_size-1], 0,
					src_buf2[buf_size-2], src_buf2[buf_size-1], 0,
					src_buf3[buf_size-2], src_buf3[buf_size-1], 0);

			GradientValuesY[0] = xFGradientY<DEPTH_SRC, DEPTH_DST>(
					src_buf1[buf_size-2], src_buf1[buf_size-1], 0,
					src_buf2[buf_size-2], src_buf2[buf_size-1], 0,
					src_buf3[buf_size-2], src_buf3[buf_size-1], 0);
		}
		else							/*			Last column border care	for NO Case			*/
		{
			GradientValuesX[0] = xFGradientX<DEPTH_SRC, DEPTH_DST>(
					src_buf1[buf_size-3], src_buf1[buf_size-2], 0,
					src_buf2[buf_size-3], src_buf2[buf_size-2], 0,
					src_buf3[buf_size-3], src_buf3[buf_size-2], 0);

			GradientValuesY[0] = xFGradientY<DEPTH_SRC, DEPTH_DST>(
					src_buf1[buf_size-3], src_buf1[buf_size-2], 0,
					src_buf2[buf_size-3], src_buf2[buf_size-2], 0,
					src_buf3[buf_size-3], src_buf3[buf_size-2], 0);
		}

		xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesX[0], P0, 0, 1, shift_x);
		xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesY[0], P1, 0, 1, shift_y);

		_gradx_mat.write(P0);
		_grady_mat.write(P1);

		shift_x = 0; shift_y = 0;
		P0 = 0; P1 = 0;

		row_ind++;
		if(row_ind == 3)
		{
			row_ind = 0;
		}
	} // Row_Loop
}
/**
 * xFScharrFilter : This function calls the xFScharrFilterKernel
 * 					depend on requirements
 * 	_gradx : X-Gradient output
 * 	_grady : Y-Gradient output
 *
 */
template<int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFScharrFilter(hls::stream<XF_SNAME(WORDWIDTH_SRC)>&   _src,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _gradx,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _grady,
		int _border_type,uint16_t _height,uint16_t _width)
{

#pragma HLS inline

	_width = _width >> XF_BITSHIFT(NPC);


	assert(((_height <= ROWS ) && (_width <= COLS)) && "ROWS and COLS should be greater than input image");

		xFScharrFilterKernel<ROWS, COLS, DEPTH_SRC, DEPTH_DST,
		NPC, WORDWIDTH_SRC, WORDWIDTH_DST, (COLS>>XF_BITSHIFT(NPC))>(_src, _gradx, _grady,_height,_width);

}


#pragma SDS data mem_attribute("_src_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_dst_matx.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_dst_maty.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL, "_dst_matx.data":SEQUENTIAL,"_dst_maty.data":SEQUENTIAL)
//#pragma SDS data data_mover("_src_mat.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst_matx.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst_maty.data":AXIDMA_SIMPLE)
#pragma SDS data sys_port("_src_mat.data":HP)
#pragma SDS data sys_port("_dst_matx.data":HP)
#pragma SDS data sys_port("_dst_maty.data":HP)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"], "_dst_matx.data"[0:"_dst_matx.size"],"_dst_maty.data"[0:"_dst_maty.size"])

template<int BORDER_TYPE, int SRC_T,int DST_T, int ROWS, int COLS,int NPC>
void Scharr(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst_matx,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst_maty)
{
	

	hls::stream< XF_TNAME(SRC_T,NPC)> _src;
	hls::stream< XF_TNAME(DST_T,NPC)> _dstx;
	hls::stream< XF_TNAME(DST_T,NPC)> _dsty;
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



	xFScharrFilter<ROWS,COLS,XF_DEPTH(SRC_T,NPC),XF_DEPTH(DST_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(_src,_dstx,_dsty,BORDER_TYPE,_src_mat.rows,_src_mat.cols);

	for(int i=0; i<_dst_matx.rows;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst_matx.cols)>>(XF_BITSHIFT(NPC));j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
#pragma HLS PIPELINE
#pragma HLS LOOP_FLATTEN off
			*(_dst_matx.data + i*(_dst_matx.cols>>(XF_BITSHIFT(NPC))) +j) = _dstx.read();
			*(_dst_maty.data + i*(_dst_maty.cols>>(XF_BITSHIFT(NPC))) +j) = _dsty.read();
		}
	}


}
}
#endif  // _XF_SCHARR_HPP_
