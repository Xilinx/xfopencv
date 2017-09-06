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

#ifndef _XF_SOBEL_HPP_
#define _XF_SOBEL_HPP_


typedef unsigned short  uint16_t;

typedef unsigned int  uint32_t;

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

namespace xf{

/*****************************************************************
 * 		                 SobelFilter3x3
 *****************************************************************
 * X-Gradient Computation
 *
 * -------------
 * |-1	0 	1|
 * |-2	0	2|
 * |-1	0	1|
 * -------------
 *****************************************************************/
template<int DEPTH_SRC, int  DEPTH_DST>
XF_PTNAME(DEPTH_DST) xFGradientX3x3(
		XF_PTNAME(DEPTH_SRC) t0, XF_PTNAME(DEPTH_SRC) t1,
		XF_PTNAME(DEPTH_SRC) t2, XF_PTNAME(DEPTH_SRC) m0,
		XF_PTNAME(DEPTH_SRC) m1, XF_PTNAME(DEPTH_SRC) m2,
		XF_PTNAME(DEPTH_SRC) b0, XF_PTNAME(DEPTH_SRC) b1,
		XF_PTNAME(DEPTH_SRC) b2)
		{
#pragma HLS INLINE off

	XF_PTNAME(DEPTH_DST) g_x = 0;
	XF_PTNAME(DEPTH_DST) M00 = ((XF_PTNAME(DEPTH_DST))m0 << 1);
	XF_PTNAME(DEPTH_DST) M01 = ((XF_PTNAME(DEPTH_DST))m2 << 1);
	XF_PTNAME(DEPTH_DST) A00 = (t2 + b2);
	XF_PTNAME(DEPTH_DST) S00 = (t0 + b0);
	g_x = M01 - M00;
	g_x = g_x + A00;
	g_x = g_x - S00;
	return g_x;
		}

/**********************************************************************
 * Y-Gradient Computation
 * -------------
 * | 1	 2 	 1|
 * | 0	 0	 0|
 * |-1	-2	-1|
 * -------------
 **********************************************************************/
template<int DEPTH_SRC, int  DEPTH_DST>
XF_PTNAME(DEPTH_DST) xFGradientY3x3(
		XF_PTNAME(DEPTH_SRC) t0, XF_PTNAME(DEPTH_SRC) t1,
		XF_PTNAME(DEPTH_SRC) t2, XF_PTNAME(DEPTH_SRC) m0,
		XF_PTNAME(DEPTH_SRC) m1, XF_PTNAME(DEPTH_SRC) m2,
		XF_PTNAME(DEPTH_SRC) b0, XF_PTNAME(DEPTH_SRC) b1,
		XF_PTNAME(DEPTH_SRC) b2)
		{
#pragma HLS INLINE off

	XF_PTNAME(DEPTH_DST) g_y = 0;
	XF_PTNAME(DEPTH_DST) M00, M01;
	XF_PTNAME(DEPTH_DST) A00, S00;
	M00 = ((XF_PTNAME(DEPTH_DST))t1 << 1);
	M01 = ((XF_PTNAME(DEPTH_DST))b1 << 1);
	A00 = (b0 + b2);
	S00 = (t0 + t2);
	g_y = (M01 - M00);
	g_y = (g_y + A00);
	g_y = (g_y - S00);
	return g_y;
		}

/**
 * xFSobel3x3 : Applies the mask and Computes the gradient values
 *
 */
template<int NPC,int DEPTH_SRC,int DEPTH_DST>
void xFSobel3x3(
		XF_PTNAME(DEPTH_DST) *GradientvaluesX,
		XF_PTNAME(DEPTH_DST) *GradientvaluesY,
		XF_PTNAME(DEPTH_SRC)  *src_buf1,
		XF_PTNAME(DEPTH_SRC)  *src_buf2,
		XF_PTNAME(DEPTH_SRC)  *src_buf3)
{
#pragma HLS INLINE off

	Compute_Grad_Loop:
	for(ap_uint<5> j = 0; j < XF_NPIXPERCYCLE(NPC); j++)
	{
#pragma HLS UNROLL
		GradientvaluesX[j] = xFGradientX3x3<DEPTH_SRC, DEPTH_DST>(
				src_buf1[j], src_buf1[j+1], src_buf1[j+2],
				src_buf2[j], src_buf2[j+1], src_buf2[j+2],
				src_buf3[j], src_buf3[j+1],	src_buf3[j+2]);

		GradientvaluesY[j] = xFGradientY3x3<DEPTH_SRC, DEPTH_DST>(
				src_buf1[j], src_buf1[j+1], src_buf1[j+2],
				src_buf2[j], src_buf2[j+1], src_buf2[j+2],
				src_buf3[j], src_buf3[j+1],	src_buf3[j+2]);
	}
}


/**************************************************************************************
 * ProcessSobel3x3 : Computes gradients for the column input data
 **************************************************************************************/
template<int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void ProcessSobel3x3(hls::stream< XF_SNAME(WORDWIDTH_SRC) > & _src_mat,
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

		xFSobel3x3<NPC, DEPTH_SRC, DEPTH_DST>(GradientValuesX, GradientValuesY,
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

/**
 * xFSobelFilter3x3 : Computes Sobel gradient of the input image
 *                    for filtersize 3x3
 * _src_mat		: Input image
 * _gradx_mat	: GradientX output
 * _grady_mat	: GradientY output
 */
template<int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void xFSobelFilter3x3(hls::stream< XF_SNAME(WORDWIDTH_SRC) > &_src_mat,
		hls::stream< XF_SNAME(WORDWIDTH_DST) > &_gradx_mat,
		hls::stream< XF_SNAME(WORDWIDTH_DST) > &_grady_mat, uint16_t img_height, uint16_t img_width)
{
	ap_uint<13> row_ind;
	ap_uint<2> tp, mid, bottom;
	ap_uint<8> buf_size = XF_NPIXPERCYCLE(NPC) + 2;
	uint16_t shift_x = 0, shift_y = 0;
	ap_uint<13> row, col;

	XF_PTNAME (DEPTH_DST)
	GradientValuesX[XF_NPIXPERCYCLE(NPC)];										// X-Gradient result buffer
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
		ProcessSobel3x3<ROWS, COLS, DEPTH_SRC, DEPTH_DST, NPC, WORDWIDTH_SRC, WORDWIDTH_DST, TC>
		(_src_mat, _gradx_mat, _grady_mat, buf, src_buf1, src_buf2, src_buf3, GradientValuesX, GradientValuesY, P0, P1, img_width, img_height, row_ind, shift_x, shift_y, tp, mid, bottom, row);

		/*			Last column border care	for RO & PO Case			*/
		if((NPC == XF_NPPC8) || (NPC == XF_NPPC16))
		{
			//	Compute gradient at last column
			GradientValuesX[0] = xFGradientX3x3<DEPTH_SRC, DEPTH_DST>(
					src_buf1[buf_size-2], src_buf1[buf_size-1], 0,
					src_buf2[buf_size-2], src_buf2[buf_size-1], 0,
					src_buf3[buf_size-2], src_buf3[buf_size-1], 0);

			GradientValuesY[0] = xFGradientY3x3<DEPTH_SRC, DEPTH_DST>(
					src_buf1[buf_size-2], src_buf1[buf_size-1], 0,
					src_buf2[buf_size-2], src_buf2[buf_size-1], 0,
					src_buf3[buf_size-2], src_buf3[buf_size-1], 0);
		}
		else							/*			Last column border care	for NO Case			*/
		{
			GradientValuesX[0] = xFGradientX3x3<DEPTH_SRC, DEPTH_DST>(
					src_buf1[buf_size-3], src_buf1[buf_size-2], 0,
					src_buf2[buf_size-3], src_buf2[buf_size-2], 0,
					src_buf3[buf_size-3], src_buf3[buf_size-2], 0);

			GradientValuesY[0] = xFGradientY3x3<DEPTH_SRC, DEPTH_DST>(
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
// xFSobelFilter3x3


/*****************************************************************
 * 		                 SobelFilter5x5
 *****************************************************************/
/**
 *  Sobel Filter X-Gradient used is 5x5
 *
 *       --- ---- ---- ---- ---
 *      | -1 |  -2 | 0 |  2 | 1 |
 *       --- ---- ---- ---- ---
 *      | -4 |  -8 | 0 |  8 | 4 |
 *       --- ---- ---- ---- ---
 *      | -6 | -12 | 0 | 12 | 6 |
 *       --- ---- ---- ---- ---
 *      | -4 |  -8 | 0 |  8 | 4 |
 *       --- ---- ---- ---- ---
 *      | -1 |  -2 | 0 |  2 | 1 |
 *       --- ---- ---- ---- ---
 ****************************************************************/

template<int DEPTH_SRC, int DEPTH_DST>
XF_PTNAME(DEPTH_DST) xFGradientX5x5(XF_PTNAME(DEPTH_SRC) *src_buf1, XF_PTNAME(DEPTH_SRC) *src_buf2,
									XF_PTNAME(DEPTH_SRC) *src_buf3, XF_PTNAME(DEPTH_SRC) *src_buf4,	XF_PTNAME(DEPTH_SRC) *src_buf5)
{
#pragma HLS INLINE off
	XF_PTNAME(DEPTH_DST) g_x = 0;
	XF_PTNAME(DEPTH_DST) M00 = (XF_PTNAME(DEPTH_DST))(src_buf1[1] + src_buf5[1]) << 1;
	XF_PTNAME(DEPTH_DST) M01 = (XF_PTNAME(DEPTH_DST))(src_buf1[4] + src_buf5[4])-(src_buf1[0] + src_buf5[0]);
	XF_PTNAME(DEPTH_DST) A00 = (XF_PTNAME(DEPTH_DST))(src_buf1[3] + src_buf5[3]) << 1;
	XF_PTNAME(DEPTH_DST) M02 = (XF_PTNAME(DEPTH_DST))(src_buf2[0] + src_buf4[0]) << 2;
	XF_PTNAME(DEPTH_DST) M03 = (XF_PTNAME(DEPTH_DST))(src_buf2[1] + src_buf4[1]) << 3;
	XF_PTNAME(DEPTH_DST) A01 = (XF_PTNAME(DEPTH_DST))(src_buf2[3] + src_buf4[3]) << 3;
	XF_PTNAME(DEPTH_DST) A02 = (XF_PTNAME(DEPTH_DST))(src_buf2[4] + src_buf4[4]) << 2;
	XF_PTNAME(DEPTH_DST) M04 = src_buf3[0] * 6;
	XF_PTNAME(DEPTH_DST) M05 = src_buf3[1] * 12;
	XF_PTNAME(DEPTH_DST) A03 = src_buf3[3] * 12;
	XF_PTNAME(DEPTH_DST) A04 = src_buf3[4] * 6;
	XF_PTNAME(DEPTH_DST) S00 = M00 + M02;
	XF_PTNAME(DEPTH_DST) S01 = M03 + M04 + M05;
	XF_PTNAME(DEPTH_DST) A0 = A00 + A01;
	XF_PTNAME(DEPTH_DST) A1 = A02 + A03;
	XF_PTNAME(DEPTH_DST) A2 = A04 + M01;
	XF_PTNAME(DEPTH_DST) FA = A0 + A1 + A2;
	XF_PTNAME(DEPTH_DST) FS = S00 + S01;
	g_x = FA - FS;
	return g_x;
}
/****************************************************************
 * Sobel Filter Y-Gradient used is 5x5
 *
 *       --- ---- ---- ---- ---
 *      | -1 |  -4 |  -6 |  -4 | -1 |
 *       --- ---- ---- ---- ---
 *      | -2 |  -8 | -12 |  -8 | -2 |
 *       --- ---- ---- ---- ---
 *      |  0 |   0 |   0 |   0 |  0 |
 *       --- ---- ---- ---- --- ---
 *      |  2 |   8 |  12 |   8 |  2 |
 *       --- ---- ---- ---- --- ---
 *      |  1 |   4 |   6 |   4 |  1 |
 *       --- ---- ---- ---- --- ---
 ******************************************************************/

template<int DEPTH_SRC, int  DEPTH_DST>
XF_PTNAME(DEPTH_DST) xFGradientY5x5(XF_PTNAME(DEPTH_SRC) *src_buf1, XF_PTNAME(DEPTH_SRC) *src_buf2,
									XF_PTNAME(DEPTH_SRC) *src_buf3, XF_PTNAME(DEPTH_SRC) *src_buf4,	XF_PTNAME(DEPTH_SRC) *src_buf5)
{
#pragma HLS INLINE off
	XF_PTNAME(DEPTH_DST) g_y = 0;
	XF_PTNAME(DEPTH_DST) M00 = (src_buf5[0] + src_buf5[4]) - (src_buf1[0] + src_buf1[4]);
	XF_PTNAME(DEPTH_DST) M01 = (XF_PTNAME(DEPTH_DST))(src_buf1[1] + src_buf1[3]) << 2;
	XF_PTNAME(DEPTH_DST) A00 = (XF_PTNAME(DEPTH_DST))(src_buf5[1] + src_buf5[3]) << 2;
	XF_PTNAME(DEPTH_DST) M02 = (XF_PTNAME(DEPTH_DST))(src_buf2[0] + src_buf2[4]) << 1;
	XF_PTNAME(DEPTH_DST) A01 = (XF_PTNAME(DEPTH_DST))(src_buf4[0] + src_buf4[4]) << 1;
	XF_PTNAME(DEPTH_DST) M03 = (XF_PTNAME(DEPTH_DST))(src_buf2[1] + src_buf2[3]) << 3;
	XF_PTNAME(DEPTH_DST) A02 = (XF_PTNAME(DEPTH_DST))(src_buf4[1] + src_buf4[3]) << 3;
	XF_PTNAME(DEPTH_DST) M04 = src_buf1[2] * 6;
	XF_PTNAME(DEPTH_DST) M05 = src_buf2[2] * 12;
	XF_PTNAME(DEPTH_DST) A03 = src_buf4[2] * 12;
	XF_PTNAME(DEPTH_DST) A04 = src_buf5[2] * 6;
	XF_PTNAME(DEPTH_DST) S00 = M01 + M02 + M03;
	XF_PTNAME(DEPTH_DST) S01 = M04 + M05;
	XF_PTNAME(DEPTH_DST) A0 = A00 + A01;
	XF_PTNAME(DEPTH_DST) A1 = A02 + A03;
	XF_PTNAME(DEPTH_DST) A2 = A04 + M00;
	XF_PTNAME(DEPTH_DST) FA = A0 + A1 + A2;
	XF_PTNAME(DEPTH_DST) FS = S00 + S01;
	g_y = FA - FS;
	return g_y;
}
/**
 * xFSobel5x5 : Applies the mask and Computes the gradient values
 *
 */
template<int NPC, int DEPTH_SRC, int DEPTH_DST>
void xFSobel5x5(
		XF_PTNAME(DEPTH_DST) *GradientvaluesX,
		XF_PTNAME(DEPTH_DST) *GradientvaluesY,
		XF_PTNAME(DEPTH_SRC) *src_buf1,
		XF_PTNAME(DEPTH_SRC) *src_buf2,
		XF_PTNAME(DEPTH_SRC) *src_buf3,
		XF_PTNAME(DEPTH_SRC) *src_buf4,
		XF_PTNAME(DEPTH_SRC) *src_buf5)
{
#pragma HLS INLINE off

	Compute_Grad_Loop:
	for(ap_uint<5> j = 0; j < XF_NPIXPERCYCLE(NPC); j++ )
	{
#pragma HLS LOOP_TRIPCOUNT min=8 max=8
#pragma HLS UNROLL
		GradientvaluesX[j] = xFGradientX5x5<DEPTH_SRC, DEPTH_DST>(&src_buf1[j], &src_buf2[j], &src_buf3[j], &src_buf4[j], &src_buf5[j]);
		GradientvaluesY[j] = xFGradientY5x5<DEPTH_SRC, DEPTH_DST>(&src_buf1[j], &src_buf2[j], &src_buf3[j], &src_buf4[j], &src_buf5[j]);
	}
}


/**************************************************************************************
 * ProcessSobel5x5 : Computes gradients for the column input data
 **************************************************************************************/
template<int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void ProcessSobel5x5(hls::stream< XF_SNAME(WORDWIDTH_SRC) > & _src_mat,
		hls::stream< XF_SNAME(WORDWIDTH_DST) > & _gradx_mat, hls::stream< XF_SNAME(WORDWIDTH_DST) > & _grady_mat,
		XF_SNAME(WORDWIDTH_SRC) buf[5][(COLS >> XF_BITSHIFT(NPC))], XF_PTNAME(DEPTH_SRC) src_buf1[XF_NPIXPERCYCLE(NPC)+4],
		XF_PTNAME(DEPTH_SRC) src_buf2[XF_NPIXPERCYCLE(NPC)+4], XF_PTNAME(DEPTH_SRC) src_buf3[XF_NPIXPERCYCLE(NPC)+4], XF_PTNAME(DEPTH_SRC) src_buf4[XF_NPIXPERCYCLE(NPC)+4], XF_PTNAME(DEPTH_SRC) src_buf5[XF_NPIXPERCYCLE(NPC)+4],
		XF_PTNAME(DEPTH_DST) GradientValuesX[XF_NPIXPERCYCLE(NPC)], XF_PTNAME(DEPTH_DST) GradientValuesY[XF_NPIXPERCYCLE(NPC)],
		XF_SNAME(WORDWIDTH_DST) &inter_valx, XF_SNAME(WORDWIDTH_DST) &inter_valy, uint16_t img_width, uint16_t img_height, ap_uint<13> row_ind, uint16_t &shift_x, uint16_t &shift_y,
		ap_uint<4> tp1, ap_uint<4> tp2, ap_uint<4> mid, ap_uint<4> bottom1, ap_uint<4> bottom2, ap_uint<13> row)
{
#pragma HLS INLINE
	XF_SNAME(WORDWIDTH_SRC)  buf0, buf1, buf2, buf3, buf4;
	ap_uint<8> buf_size = XF_NPIXPERCYCLE(NPC) + 4;
	uint16_t npc = XF_NPIXPERCYCLE(NPC);
	ap_uint<8> max_loop = XF_WORDDEPTH(WORDWIDTH_DST);
	ap_uint<8> step = XF_PIXELDEPTH(DEPTH_DST);

	Col_Loop:
	for(ap_uint<13> col = 0; col < img_width; col++)
	{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
		if(row < img_height)
			buf[row_ind][col] = _src_mat.read();
		else
			buf[bottom2][col] = 0;

		buf0 = buf[tp1][col];
		buf1 = buf[tp2][col];
		buf2 = buf[mid][col];
		buf3 = buf[bottom1][col];
		buf4 = buf[bottom2][col];

		if(NPC == XF_NPPC8)
		{
			xfExtractPixels<NPC, WORDWIDTH_SRC, DEPTH_SRC>(&src_buf1[4], buf0, 0);
			xfExtractPixels<NPC, WORDWIDTH_SRC, DEPTH_SRC>(&src_buf2[4], buf1, 0);
			xfExtractPixels<NPC, WORDWIDTH_SRC, DEPTH_SRC>(&src_buf3[4], buf2, 0);
			xfExtractPixels<NPC, WORDWIDTH_SRC, DEPTH_SRC>(&src_buf4[4], buf3, 0);
			xfExtractPixels<NPC, WORDWIDTH_SRC, DEPTH_SRC>(&src_buf5[4], buf4, 0);
		}
		else
		{
			src_buf1[4] = buf0;
			src_buf2[4] = buf1;
			src_buf3[4] = buf2;
			src_buf4[4] = buf3;
			src_buf5[4] = buf4;

		}
		xFSobel5x5<NPC, DEPTH_SRC, DEPTH_DST>(GradientValuesX, GradientValuesY,
											  src_buf1, src_buf2, src_buf3, src_buf4, src_buf5);

		for(ap_uint<4> i = 0; i < 4; i++)
		{
#pragma HLS unroll
			src_buf1[i] = src_buf1[buf_size-(4 - i)];
			src_buf2[i] = src_buf2[buf_size-(4 - i)];
			src_buf3[i] = src_buf3[buf_size-(4 - i)];
			src_buf4[i] = src_buf4[buf_size-(4 - i)];
			src_buf5[i] = src_buf5[buf_size-(4 - i)];
		}
		if(col == 0)
		{
			shift_x = 0, shift_y = 0;
			inter_valx = 0; inter_valy = 0;

			xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesX[0], inter_valx, 2, (npc-2), shift_x);
			xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesY[0], inter_valy, 2, (npc-2), shift_y);

		}
		else
		{
			if((NPC == XF_NPPC8) || (NPC == XF_NPPC16))
			{
				xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesX[0], inter_valx, 0, 2, shift_x);
				xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesY[0], inter_valy, 0, 2, shift_y);

				_gradx_mat.write(inter_valx);
				_grady_mat.write(inter_valy);

				shift_x = 0; shift_y = 0;
				inter_valx = 0; inter_valy = 0;
				xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesX[0], inter_valx, 2, (npc-2), shift_x);
				xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesY[0], inter_valy, 2, (npc-2), shift_y);
			}
			else
			{
				if(col >= 2)
				{
					inter_valx((max_loop-1), (max_loop-step)) = GradientValuesX[0];
					inter_valy((max_loop-1), (max_loop-step)) = GradientValuesY[0];
					_gradx_mat.write(inter_valx);
					_grady_mat.write(inter_valy);
				}
			}
		}
	} // Col_Loop
}

/**
 * xFSobelFilter5x5 : Computes Sobel gradient of the input image
 *                    for filtersize 5X5
 * _src_mat		: Input image
 * _gradx_mat	: GradientX output
 * _grady_mat	: GradientY output
 */
template<int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void xFSobelFilter5x5(hls::stream< XF_SNAME(WORDWIDTH_SRC) > & _src_mat,
		hls::stream< XF_SNAME(WORDWIDTH_DST) > & _gradx_mat,
		hls::stream< XF_SNAME(WORDWIDTH_DST) > & _grady_mat, uint16_t img_height, uint16_t img_width)
{
	ap_uint<13> row_ind;
	ap_uint<13> row, col;
	ap_uint<4> tp1, tp2, mid, bottom1, bottom2;
	ap_uint<5> i;

	ap_uint<8> buf_size = XF_NPIXPERCYCLE(NPC) + 4;
	ap_uint<9> step = XF_PIXELDEPTH(DEPTH_DST);
	ap_uint<9> max_loop = XF_WORDDEPTH(WORDWIDTH_DST);
	uint16_t shift_x = 0, shift_y = 0;

	XF_PTNAME(DEPTH_DST) GradientValuesX[XF_NPIXPERCYCLE(NPC)];
	XF_PTNAME(DEPTH_DST) GradientValuesY[XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=GradientValuesX complete dim=1
#pragma HLS ARRAY_PARTITION variable=GradientValuesY complete dim=1

	XF_SNAME(WORDWIDTH_SRC)  buf0, buf1, buf2, buf3, buf4;
	// Temporary buffers to hold image data from five rows
	XF_PTNAME(DEPTH_SRC) src_buf1[XF_NPIXPERCYCLE(NPC)+4], src_buf2[XF_NPIXPERCYCLE(NPC)+4],
			src_buf3[XF_NPIXPERCYCLE(NPC)+4], src_buf4[XF_NPIXPERCYCLE(NPC)+4],
			src_buf5[XF_NPIXPERCYCLE(NPC)+4];
#pragma HLS ARRAY_PARTITION variable=src_buf1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf2 complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf3 complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf4 complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf5 complete dim=1

	XF_SNAME(WORDWIDTH_SRC)  tmp_in;
	XF_SNAME(WORDWIDTH_DST) inter_valx = 0, inter_valy = 0;
	// Temporary buffer to hold image data from five rows
	XF_SNAME(WORDWIDTH_SRC)  buf[5][(COLS >> XF_BITSHIFT(NPC))];
#pragma HLS RESOURCE variable=buf core=RAM_S2P_BRAM
#pragma HLS ARRAY_PARTITION variable=buf complete dim=1

	row_ind = 2;

	Clear_Row_Loop:
	for(col = 0; col < img_width; col++)
	{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
		buf[0][col] = 0;
		buf[1][col] = 0;
		buf[row_ind][col] = _src_mat.read();
	}

	row_ind++;

	Read_Row2_Loop:
	for(col = 0; col < img_width; col++)
	{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline

		buf[row_ind][col] = _src_mat.read();
	}
	row_ind++;

	Row_Loop:
	for(row = 2; row < img_height+2; row++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS

		// modify the buffer indices to re use
		if(row_ind == 4)
		{
			tp1 = 0; tp2 = 1; mid = 2; bottom1 = 3; bottom2 = 4;
		}
		else if(row_ind == 0)
		{
			tp1 = 1; tp2 = 2; mid = 3; bottom1 = 4; bottom2 = 0;
		}
		else if(row_ind == 1)
		{
			tp1 = 2; tp2 = 3; mid = 4; bottom1 = 0; bottom2 = 1;
		}
		else if(row_ind == 2)
		{
			tp1 = 3; tp2 = 4; mid = 0; bottom1 = 1; bottom2 = 2;
		}
		else if(row_ind == 3)
		{
			tp1 = 4; tp2 = 0; mid = 1; bottom1 = 2; bottom2 = 3;
		}

		src_buf1[0] = src_buf1[1] = src_buf1[2] = src_buf1[3] = 0;
		src_buf2[0] = src_buf2[1] = src_buf2[2] = src_buf2[3] = 0;
		src_buf3[0] = src_buf3[1] = src_buf3[2] = src_buf3[3] = 0;
		src_buf4[0] = src_buf4[1] = src_buf4[2] = src_buf4[3] = 0;
		src_buf5[0] = src_buf5[1] = src_buf5[2] = src_buf5[3] = 0;

		inter_valx = inter_valy = 0;

		ProcessSobel5x5<ROWS, COLS, DEPTH_SRC, DEPTH_DST, NPC, WORDWIDTH_SRC, WORDWIDTH_DST, TC>( _src_mat, _gradx_mat,  _grady_mat, buf, src_buf1,	src_buf2, src_buf3, src_buf4, src_buf5,	GradientValuesX, GradientValuesY,
				inter_valx, inter_valy, img_width, img_height, row_ind, shift_x, shift_y, tp1, tp2, mid, bottom1, bottom2, row);

		if((NPC == XF_NPPC8) || (NPC == XF_NPPC16))
		{
			for(ap_uint<6> i = 4; i < (XF_NPIXPERCYCLE(NPC)+4); i++)
			{
				src_buf1[i] = 0;
				src_buf2[i] = 0;
				src_buf3[i] = 0;
				src_buf4[i] = 0;
				src_buf5[i] = 0;
			}

			GradientValuesX[0] = xFGradientX5x5<DEPTH_SRC, DEPTH_DST>(&src_buf1[0], &src_buf2[0], &src_buf3[0], &src_buf4[0], &src_buf5[0]);
			GradientValuesX[1] = xFGradientX5x5<DEPTH_SRC, DEPTH_DST>(&src_buf1[1], &src_buf2[1], &src_buf3[1], &src_buf4[1], &src_buf5[1]);
			GradientValuesY[0] = xFGradientY5x5<DEPTH_SRC, DEPTH_DST>(&src_buf1[0], &src_buf2[0], &src_buf3[0], &src_buf4[0], &src_buf5[0]);
			GradientValuesY[1] = xFGradientY5x5<DEPTH_SRC, DEPTH_DST>(&src_buf1[1], &src_buf2[1], &src_buf3[1], &src_buf4[1], &src_buf5[1]);

			xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesX[0], inter_valx, 0, 2, shift_x);
			xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesY[0], inter_valy, 0, 2, shift_y);

			_gradx_mat.write(inter_valx);
			_grady_mat.write(inter_valy);
		}
		else
		{
#pragma HLS ALLOCATION instances=xFGradientX5x5 limit=1 function
#pragma HLS ALLOCATION instances=xFGradientY5x5 limit=1 function

			src_buf1[buf_size-1] = 0;
			src_buf2[buf_size-1] = 0;
			src_buf3[buf_size-1] = 0;
			src_buf4[buf_size-1] = 0;
			src_buf5[buf_size-1] = 0;

			GradientValuesX[0] = xFGradientX5x5<DEPTH_SRC, DEPTH_DST>(&src_buf1[0], &src_buf2[0], &src_buf3[0], &src_buf4[0], &src_buf5[0]);
			GradientValuesY[0] = xFGradientY5x5<DEPTH_SRC, DEPTH_DST>(&src_buf1[0], &src_buf2[0], &src_buf3[0], &src_buf4[0], &src_buf5[0]);
			inter_valx((max_loop-1), (max_loop-step)) = GradientValuesX[0];
			inter_valy((max_loop-1), (max_loop-step)) = GradientValuesY[0];
			_gradx_mat.write(inter_valx);
			_grady_mat.write(inter_valy);

			for(ap_uint<4> i = 0; i < 4; i++)
			{
#pragma HLS unroll
				src_buf1[i] = src_buf1[buf_size-(4 - i)];
				src_buf2[i] = src_buf2[buf_size-(4 - i)];
				src_buf3[i] = src_buf3[buf_size-(4 - i)];
				src_buf4[i] = src_buf4[buf_size-(4 - i)];
				src_buf5[i] = src_buf5[buf_size-(4 - i)];
			}
			src_buf1[buf_size-1] = 0;
			src_buf2[buf_size-1] = 0;
			src_buf3[buf_size-1] = 0;
			src_buf4[buf_size-1] = 0;
			src_buf5[buf_size-1] = 0;

			GradientValuesX[0] = xFGradientX5x5<DEPTH_SRC, DEPTH_DST>(&src_buf1[0], &src_buf2[0], &src_buf3[0], &src_buf4[0], &src_buf5[0]);
			GradientValuesY[0] = xFGradientY5x5<DEPTH_SRC, DEPTH_DST>(&src_buf1[0], &src_buf2[0], &src_buf3[0], &src_buf4[0], &src_buf5[0]);

			inter_valx((max_loop-1), (max_loop-step)) = GradientValuesX[0];
			inter_valy((max_loop-1), (max_loop-step)) = GradientValuesY[0];
			_gradx_mat.write(inter_valx);
			_grady_mat.write(inter_valy);
		}

		row_ind++;

		if(row_ind == 5)
		{
			row_ind = 0;
		}
	} // Row_Loop
}
// xFSobelFilter5x5

/*******************************************************************************
 * 			SobelFilter7x7
 *******************************************************************************
 *      SobelFilter X-Gradient used is 7X7
 *
 *       --- ---- ---- ---- ---  ---- ---  ----
 *      |  -1 |  -4 |   -5 | 0 |   5 |  4 |  1 |
 *       --- ---- ---- ---- ---  ---- ---  ----
 *      |  -6 | -24 |  -30 | 0 |  30 | 24 |  6 |
 *       --- ---- ---- ---- ---  ---- ---  ----
 *      | -15 | -60 |  -75 | 0 |  75 | 60 | 15 |
 *       --- ---- ---- ---- ---  ---- ---  ----
 *      | -20 | -80 | -100 | 0 | 100 | 80 | 20 |
 *       --- ---- ---- ---- ---  ---- ---  ----
 *      | -15 | -60 |  -75 | 0 |  75 | 60 | 15 |
 *       --- ---- ---- ---- ---  ---- ---  ----
 *      |  -6 | -24 |  -30 | 0 |  30 | 24 |  6 |
 *       --- ---- ---- ---- ---  ---- ---  ----
 *      |  -1 |  -4 |   -5 | 0 |   5 |  4 |  1 |
 *       --- ---- ---- ---- ---  ---- ---  ----
 ******************************************************************************/
template<int DEPTH_SRC, int DEPTH_DST>
XF_PTNAME(DEPTH_DST) xFGradientX7x7(XF_PTNAME(DEPTH_SRC) *src_buf1, XF_PTNAME(DEPTH_SRC) *src_buf2,
		XF_PTNAME(DEPTH_SRC) *src_buf3, XF_PTNAME(DEPTH_SRC) *src_buf4,
		XF_PTNAME(DEPTH_SRC) *src_buf5, XF_PTNAME(DEPTH_SRC) *src_buf6,
		XF_PTNAME(DEPTH_SRC) *src_buf7)
		{
#pragma HLS INLINE off
#pragma HLS PIPELINE II=1

	XF_PTNAME(DEPTH_DST) g_x = 0;
	XF_PTNAME(DEPTH_DST) Res;
	Res = 0;

	ap_int<20> M00 = (src_buf1[6] + src_buf7[6]) - (src_buf1[0] + src_buf7[0]);
	ap_int<20> M01 = (ap_int<20>)(src_buf1[1] + src_buf7[1]) << 2;
	ap_int<20> A00 = (ap_int<20>)(src_buf1[5] + src_buf7[5]) << 2;
	ap_int<20> M02 = ((ap_int<20>)(src_buf1[2] + src_buf7[2]) << 2) + (src_buf1[2] + src_buf7[2]);		//(src_buf1[2] + src_buf7[2]) * 5;
	ap_int<20> A01 = ((ap_int<20>)(src_buf1[4] + src_buf7[4]) << 2) + (src_buf1[4] + src_buf7[4]);			//(src_buf1[4] + src_buf7[4]) * 5;
	ap_int<20> M03 = ((ap_int<20>)(src_buf2[0] + src_buf6[0]) << 2) + ((ap_int<20>)(src_buf2[0] + src_buf6[0]) << 1) ;	//(src_buf2[0] + src_buf6[0]) * 6;
	ap_int<20> A02 = ((ap_int<20>)(src_buf2[6] + src_buf6[6]) << 2) + ((ap_int<20>)(src_buf2[6] + src_buf6[6]) << 1) ;	//(src_buf2[6] + src_buf6[6]) * 6;
	ap_int<20> M04 = ((ap_int<20>)(src_buf2[1] + src_buf6[1]) << 4) + ((ap_int<20>)(src_buf2[1] + src_buf6[1]) << 3) ;	//(src_buf2[1] + src_buf6[1]) * 24;
	ap_int<20> A03 = ((ap_int<20>)(src_buf2[5] + src_buf6[5]) << 4) + ((ap_int<20>)(src_buf2[5] + src_buf6[5]) << 3) ;	//(src_buf2[5] + src_buf6[5]) * 24;
	ap_int<20> M05 = ((ap_int<20>)(src_buf2[2] + src_buf6[2]) << 5) - ((ap_int<20>)(src_buf2[2] + src_buf6[2]) << 1) ;	//(src_buf2[2] + src_buf6[2]) * 30;
	ap_int<20> A04 = ((ap_int<20>)(src_buf2[4] + src_buf6[4]) << 5) - ((ap_int<20>)(src_buf2[4] + src_buf6[4]) << 1) ;	//(src_buf2[4] + src_buf6[4]) * 30;
	ap_int<20> M06 = ((ap_int<20>)(src_buf3[0] + src_buf5[0]) << 4) - (src_buf3[0] + src_buf5[0]) ;			//(src_buf3[0] + src_buf5[0]) * 15;
	ap_int<20> A05 = ((ap_int<20>)(src_buf3[6] + src_buf5[6]) << 4) - (src_buf3[6] + src_buf5[6]) ;			//(src_buf3[6] + src_buf5[6]) * 15;
	ap_int<20> M07 = ((ap_int<20>)(src_buf3[1] + src_buf5[1]) << 6) - ((ap_int<20>)(src_buf3[1] + src_buf5[1]) << 2);		//(src_buf3[1] + src_buf5[1]) * 60;
	ap_int<20> A06 = ((ap_int<20>)(src_buf3[5] + src_buf5[5]) << 6) - ((ap_int<20>)(src_buf3[5] + src_buf5[5]) << 2);		//(src_buf3[5] + src_buf5[5]) * 60;
	ap_int<20> M08 = ((ap_int<20>)(src_buf3[2] + src_buf5[2]) << 6) + ((ap_int<20>)(src_buf3[2] + src_buf5[2]) << 3) + ((ap_int<20>)(src_buf3[2] + src_buf5[2]) << 1) + (src_buf3[2] + src_buf5[2]);//(src_buf3[2] + src_buf5[2]) * 75;
	ap_int<20> A07 = ((ap_int<20>)(src_buf3[4] + src_buf5[4]) << 6) + ((ap_int<20>)(src_buf3[4] + src_buf5[4]) << 3) + ((ap_int<20>)(src_buf3[4] + src_buf5[4]) << 1) + (src_buf3[4] + src_buf5[4]);//(src_buf3[4] + src_buf5[4]) * 75;
	ap_int<20> M09 = ((ap_int<20>)(src_buf4[6] - src_buf4[0]) << 4) + ((ap_int<20>)(src_buf4[6] - src_buf4[0]) << 2);		//(src_buf4[6] - src_buf4[0]) * 20;
	ap_int<20> M10 = ((ap_int<20>)(src_buf4[5] - src_buf4[1]) << 6) + ((ap_int<20>)(src_buf4[5] - src_buf4[1]) << 4) ;					//(src_buf4[5] - src_buf4[1]) * 80;
	ap_int<20> M11 = ((ap_int<20>)(src_buf4[4] - src_buf4[2]) << 6) + ((ap_int<20>)(src_buf4[4] - src_buf4[2]) << 5) + ((ap_int<20>)(src_buf4[4] - src_buf4[2]) << 2);//(src_buf4[4] - src_buf4[2]) * 100;
	ap_int<20> FS00 = M01 + M02 + M03;
	ap_int<20> FS01 = M04 + M05;
	ap_int<20> FS02 = M06 + M07 + M08;
	ap_int<20> FA00 = A00 + A01;
	ap_int<20> FA01 = A02 + A03;
	ap_int<20> FA02 = A04 + A05;
	ap_int<20> FA03 = A06 + A07;
	ap_int<20> FA04 = M09 + M10 + M11;
	ap_int<20> FS0 = FS00 + FS01 + FS02;
	ap_int<20> FA0 = M00 + FA00 + FA01;
	ap_int<20> FA1 = FA02 + FA03 + FA04;
	Res = (FA0 + FA1) - (FS0);
	g_x = Res ;
	return g_x;
		}

/********************************************************************
 *     SobelFilter Y-Gradient used is 7X7
 *
 *       --- ---- ---- ---- ---  ---- ---  ----
 *      | -1 |  -6 | -15 | -20 | -15 | -6 | -1 |
 *       --- ---- ---- ---- ---  ---- ---  ----
 *      | -4 | -24 | -60 | -80 | -60 |-24 | -4 |
 *       --- ---- ---- ---- ---  ---- ---  ----
 *      | -5 | -30 | -75 |-100 | -75 |-30 | -5 |
 *       --- ---- ---- ---- ---  ---- ---  ----
 *      |  0 |   0 |   0 |   0 |   0 |  0 |  0 |
 *       --- ---- ---- ---- ---  ---- ---  ----
 *      |  5 |  30 |  75 | 100 |  75 | 30 |  5 |
 *       --- ---- ---- ---- ---  ---- ---  ----
 *      |  4 |  24 |  60 |  80 |  60 | 24 |  4 |
 *       --- ---- ---- ---- ---  ---- ---  ----
 *      |  1 |   6 |  15 |  20 |  15 |  6 |  1 |
 *       --- ---- ---- ---- ---  ---- ---  ----
 ******************************************************************/
template<int DEPTH_SRC, int DEPTH_DST>
XF_PTNAME(DEPTH_DST) xFGradientY7x7(XF_PTNAME(DEPTH_SRC) *src_buf1, XF_PTNAME(DEPTH_SRC) *src_buf2,
		XF_PTNAME(DEPTH_SRC) *src_buf3, XF_PTNAME(DEPTH_SRC) *src_buf4,
		XF_PTNAME(DEPTH_SRC) *src_buf5, XF_PTNAME(DEPTH_SRC) *src_buf6,
		XF_PTNAME(DEPTH_SRC) *src_buf7)
		{
#pragma HLS INLINE off
#pragma HLS PIPELINE II=1
	XF_PTNAME(DEPTH_DST) g_y = 0;
	XF_PTNAME(DEPTH_DST) Res;
	Res = 0;
	ap_int<20> M00 = (src_buf7[0] + src_buf7[6]) - (src_buf1[0] + src_buf1[6]);
	ap_int<20> M01 = ((ap_int<20>)(src_buf1[1] + src_buf1[5]) << 2) + ((ap_int<20>)(src_buf1[1] + src_buf1[5]) << 1);//(src_buf1[1] + src_buf1[5]) * 6;
	ap_int<20> A00 = ((ap_int<20>)(src_buf7[1] + src_buf7[5]) << 2) + ((ap_int<20>)(src_buf7[1] + src_buf7[5]) << 1);//(src_buf7[1] + src_buf7[5]) * 6;
	ap_int<20> M02 = ((ap_int<20>)(src_buf1[2] + src_buf1[4]) << 4) - (src_buf1[2] + src_buf1[4]) ;							   // (src_buf1[2] + src_buf1[4]) * 15;
	ap_int<20> A01 = ((ap_int<20>)(src_buf7[2] + src_buf7[4]) << 4) - (src_buf7[2] + src_buf7[4]) ;							   //(src_buf7[2] + src_buf7[4]) * 15;
	ap_int<20> M03 = (ap_int<20>)(src_buf2[0] + src_buf2[6]) << 2;
	ap_int<20> A02 = (ap_int<20>)(src_buf6[0] + src_buf6[6]) << 2;
	ap_int<20> M04 = ((ap_int<20>)(src_buf2[1] + src_buf2[5]) << 4) + ((ap_int<20>)(src_buf2[1] + src_buf2[5]) << 3);//(src_buf2[1] + src_buf2[5]) * 24;
	ap_int<20> A03 = ((ap_int<20>)(src_buf6[1] + src_buf6[5]) << 4) + ((ap_int<20>)(src_buf6[1] + src_buf6[5]) << 3);//(src_buf6[1] + src_buf6[5]) * 24;
	ap_int<20> M05 = ((ap_int<20>)(src_buf2[2] + src_buf2[4]) << 6) - ((ap_int<20>)(src_buf2[2] + src_buf2[4]) << 2);//(src_buf2[2] + src_buf2[4]) * 60;
	ap_int<20> A04 = ((ap_int<20>)(src_buf6[2] + src_buf6[4]) << 6) - ((ap_int<20>)(src_buf6[2] + src_buf6[4]) << 2);//(src_buf6[2] + src_buf6[4]) * 60;
	ap_int<20> M06 = ((ap_int<20>)(src_buf3[0] + src_buf3[6]) << 2) + (src_buf3[0] + src_buf3[6]);//(src_buf3[0] + src_buf3[6]) * 5;
	ap_int<20> A05 = ((ap_int<20>)(src_buf5[0] + src_buf5[6]) << 2) + (src_buf5[0] + src_buf5[6]);//(src_buf5[0] + src_buf5[6]) * 5;
	ap_int<20> M07 = ((ap_int<20>)(src_buf3[1] + src_buf3[5]) << 5) - ((ap_int<20>)(src_buf3[1] + src_buf3[5]) << 1);//(src_buf3[1] + src_buf3[5]) * 30;
	ap_int<20> A06 = ((ap_int<20>)(src_buf5[1] + src_buf5[5]) << 5) - ((ap_int<20>)(src_buf5[1] + src_buf5[5]) << 1);//(src_buf5[1] + src_buf5[5]) * 30;

	ap_int<20> M08 = ((ap_int<20>)(src_buf3[2] + src_buf3[4]) << 6) + ((ap_int<20>)(src_buf3[2] + src_buf3[4]) << 3) + ((ap_int<20>)(src_buf3[2] + src_buf3[4]) << 1) + (src_buf3[2] + src_buf3[4]);//(src_buf3[2] + src_buf3[4]) * 75;
	ap_int<20> A07 = ((ap_int<20>)(src_buf5[2] + src_buf5[4]) << 6) + ((ap_int<20>)(src_buf5[2] + src_buf5[4]) << 3) + ((ap_int<20>)(src_buf5[2] + src_buf5[4]) << 1) + (src_buf5[2] + src_buf5[4]);//(src_buf5[2] + src_buf5[4]) * 75;
	ap_int<20> M09 = ((ap_int<20>)(src_buf7[3] - src_buf1[3]) << 4) + ((ap_int<20>)(src_buf7[3] - src_buf1[3]) << 2);//(src_buf7[3] - src_buf1[3]) * 20;
	ap_int<20> M10 = ((ap_int<20>)(src_buf6[3] - src_buf2[3]) << 6) + ((ap_int<20>)(src_buf6[3] - src_buf2[3]) << 4);//(src_buf6[3] - src_buf2[3]) * 80;
	ap_int<20> M11 = ((ap_int<20>)(src_buf5[3] - src_buf3[3]) << 6) + ((ap_int<20>)(src_buf5[3] - src_buf3[3]) << 5) + ((ap_int<20>)(src_buf5[3] - src_buf3[3]) << 2);	//(src_buf5[3] - src_buf3[3]) * 100;
	ap_int<20> FS00 = M01 + M02 + M03;
	ap_int<20> FS01 = M04 + M05;
	ap_int<20> FS02 = M06 + M07 + M08;
	ap_int<20> FA00 = A00 + A01;
	ap_int<20> FA01 = A02 + A03;
	ap_int<20> FA02 = A04 + A05;
	ap_int<20> FA03 = A06 + A07;
	ap_int<20> FA04 = M09 + M10 + M11;
	ap_int<20> FS0 = FS00 + FS01 + FS02;
	ap_int<20> FA0 = M00 + FA00 + FA01;
	ap_int<20> FA1 = FA02 + FA03 + FA04;
	Res = (FA0 + FA1) - (FS0);
	g_y = Res ;
	return g_y;
		}

/**
 * xFSobel7x7 : Applies the mask and Computes the gradient values
 *              for filtersize 7x7
 */
template<int NPC, int DEPTH_SRC, int DEPTH_DST>
void xFSobel7x7(XF_PTNAME(DEPTH_DST) *GradientvaluesX, XF_PTNAME(DEPTH_DST) *GradientvaluesY,
		XF_PTNAME(DEPTH_SRC) src_buf1[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_SRC) src_buf2[XF_NPIXPERCYCLE(NPC)+6],
		XF_PTNAME(DEPTH_SRC) src_buf3[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_SRC) src_buf4[XF_NPIXPERCYCLE(NPC)+6],
		XF_PTNAME(DEPTH_SRC) src_buf5[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_SRC) src_buf6[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_SRC) src_buf7[XF_NPIXPERCYCLE(NPC)+6])
{
#pragma HLS INLINE
	for(ap_uint<9> j = 0; j < XF_NPIXPERCYCLE(NPC); j++)
	{
#pragma HLS LOOP_TRIPCOUNT min=8 max=8
#pragma HLS UNROLL
		GradientvaluesX[j] = xFGradientX7x7<DEPTH_SRC, DEPTH_DST>(&src_buf1[j], &src_buf2[j],
				&src_buf3[j], &src_buf4[j], &src_buf5[j], &src_buf6[j],
				&src_buf7[j]);

		GradientvaluesY[j] = xFGradientY7x7<DEPTH_SRC, DEPTH_DST>(&src_buf1[j], &src_buf2[j],
				&src_buf3[j], &src_buf4[j], &src_buf5[j], &src_buf6[j],
				&src_buf7[j]);
	}
}




/**************************************************************************************
 * ProcessSobel7x7 : Computes gradients for the column input data
 **************************************************************************************/

template<int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void ProcessSobel7x7(hls::stream< XF_SNAME(WORDWIDTH_SRC) > & _src_mat,
		hls::stream< XF_SNAME(WORDWIDTH_DST) > & _gradx_mat, hls::stream< XF_SNAME(WORDWIDTH_DST) > & _grady_mat,
		XF_SNAME(WORDWIDTH_SRC) buf[7][(COLS >> XF_BITSHIFT(NPC))], XF_PTNAME(DEPTH_SRC) src_buf1[XF_NPIXPERCYCLE(NPC)+6],
		XF_PTNAME(DEPTH_SRC) src_buf2[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_SRC) src_buf3[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_SRC) src_buf4[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_SRC) src_buf5[XF_NPIXPERCYCLE(NPC)+6],
		XF_PTNAME(DEPTH_SRC) src_buf6[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_SRC) src_buf7[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_DST) GradientValuesX[XF_NPIXPERCYCLE(NPC)], XF_PTNAME(DEPTH_DST) GradientValuesY[XF_NPIXPERCYCLE(NPC)],
		XF_SNAME(WORDWIDTH_DST) &inter_valx, XF_SNAME(WORDWIDTH_DST) &inter_valy, uint16_t img_width, uint16_t img_height, ap_uint<13> row_ind, uint16_t &shiftx, uint16_t &shifty,
		ap_uint<4> tp1, ap_uint<4> tp2, ap_uint<4> tp3, ap_uint<4> mid, ap_uint<4> bottom1, ap_uint<4> bottom2, ap_uint<4> bottom3, ap_uint<13> row)
{
#pragma HLS INLINE
	XF_SNAME(WORDWIDTH_SRC) buf0, buf1, buf2, buf3, buf4, buf5, buf6;
	uint16_t npc = XF_NPIXPERCYCLE(NPC);
	ap_uint<10> max_loop = XF_WORDDEPTH(WORDWIDTH_DST);

	Col_Loop:
	for(ap_uint<13> col = 0; col < img_width; col++)
	{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
		if(row < img_height)
			buf[row_ind][col] = _src_mat.read();
		else
			buf[bottom3][col] = 0;
		buf0 = buf[tp1][col];
		buf1 = buf[tp2][col];
		buf2 = buf[tp3][col];
		buf3 = buf[mid][col];
		buf4 = buf[bottom1][col];
		buf5 = buf[bottom2][col];
		buf6 = buf[bottom3][col];


		if(NPC == XF_NPPC8)
		{
			xfExtractData<NPC, WORDWIDTH_SRC, DEPTH_SRC>(src_buf1, src_buf2, src_buf3, src_buf4,
					src_buf5, src_buf6, src_buf7, buf0, buf1, buf2, buf3, buf4, buf5, buf6);
		}
		else
		{
			src_buf1[6] = buf0;
			src_buf2[6] = buf1;
			src_buf3[6] = buf2;
			src_buf4[6] = buf3;
			src_buf5[6] = buf4;
			src_buf6[6] = buf5;
			src_buf7[6] = buf6;
		}
		xFSobel7x7<NPC, DEPTH_SRC, DEPTH_DST>(GradientValuesX, GradientValuesY,
				src_buf1, src_buf2, src_buf3, src_buf4,
				src_buf5, src_buf6, src_buf7);

		xfCopyData<NPC, DEPTH_SRC>(src_buf1, src_buf2, src_buf3, src_buf4,
				src_buf5, src_buf6, src_buf7);

		if(col == 0)
		{
			shiftx = 0;
			shifty = 0;
			inter_valx = 0;
			inter_valy = 0;

			xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesX[0], inter_valx, 3, (npc-3), shiftx);
			xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesY[0], inter_valy, 3, (npc-3), shifty);

		}
		else
		{
			if((NPC == XF_NPPC8) || (NPC == XF_NPPC16))
			{
				xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesX[0], inter_valx, 0, 3, shiftx);
				xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesY[0], inter_valy, 0, 3, shifty);

				_gradx_mat.write(inter_valx);
				_grady_mat.write(inter_valy);
				shiftx = 0;
				shifty = 0;
				inter_valx = 0;
				inter_valy = 0;

				xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesX[0], inter_valx, 3, (npc-3), shiftx);
				xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesY[0], inter_valy, 3, (npc-3), shifty);

			}
			else
			{
				if(col >=3 )
				{
					inter_valx((max_loop-1), (max_loop-32)) = GradientValuesX[0];
					inter_valy((max_loop-1), (max_loop-32)) = GradientValuesY[0];
					_gradx_mat.write(inter_valx);
					_grady_mat.write(inter_valy);
				}
			}
		}
	}// Col_Loop
}

template<int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void RightBorder7x7(hls::stream< XF_SNAME(WORDWIDTH_SRC) > & _src_mat,
		hls::stream< XF_SNAME(WORDWIDTH_DST) > & _gradx_mat, hls::stream< XF_SNAME(WORDWIDTH_DST) > & _grady_mat,
		XF_PTNAME(DEPTH_SRC) src_buf1[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_SRC) src_buf2[XF_NPIXPERCYCLE(NPC)+6],
		XF_PTNAME(DEPTH_SRC) src_buf3[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_SRC) src_buf4[XF_NPIXPERCYCLE(NPC)+6],
		XF_PTNAME(DEPTH_SRC) src_buf5[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_SRC) src_buf6[XF_NPIXPERCYCLE(NPC)+6],
		XF_PTNAME(DEPTH_SRC) src_buf7[XF_NPIXPERCYCLE(NPC)+6], XF_PTNAME(DEPTH_DST) GradientValuesX[XF_NPIXPERCYCLE(NPC)],
		XF_PTNAME(DEPTH_DST) GradientValuesY[XF_NPIXPERCYCLE(NPC)],	XF_SNAME(WORDWIDTH_DST) &inter_valx, XF_SNAME(WORDWIDTH_DST) &inter_valy, uint16_t &shiftx, uint16_t &shifty)
{
#pragma HLS INLINE off
	ap_uint<4> i = 0;
	ap_uint<5> buf_size = (XF_NPIXPERCYCLE(NPC)+6);
	ap_uint<10> max_loop = XF_WORDDEPTH(WORDWIDTH_DST);

	if((NPC == XF_NPPC8) || (NPC == XF_NPPC16))
	{
		for(i = 0; i < 8; i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=8 max=8
#pragma HLS unroll
			src_buf1[buf_size+i-(XF_NPIXPERCYCLE(NPC))] = 0;
			src_buf2[buf_size+i-(XF_NPIXPERCYCLE(NPC))] = 0;
			src_buf3[buf_size+i-(XF_NPIXPERCYCLE(NPC))] = 0;
			src_buf4[buf_size+i-(XF_NPIXPERCYCLE(NPC))] = 0;
			src_buf5[buf_size+i-(XF_NPIXPERCYCLE(NPC))] = 0;
			src_buf6[buf_size+i-(XF_NPIXPERCYCLE(NPC))] = 0;
			src_buf7[buf_size+i-(XF_NPIXPERCYCLE(NPC))] = 0;
		}
		for(i = 0; i < 3; i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=3 max=3
#pragma HLS unroll

			GradientValuesX[i] = xFGradientX7x7<DEPTH_SRC, DEPTH_DST>(&src_buf1[i], &src_buf2[i], &src_buf3[i],
					&src_buf4[i], &src_buf5[i], &src_buf6[i],
					&src_buf7[i]);

			GradientValuesY[i] = xFGradientY7x7<DEPTH_SRC, DEPTH_DST>(&src_buf1[i], &src_buf2[i], &src_buf3[i],
					&src_buf4[i], &src_buf5[i], &src_buf6[i],
					&src_buf7[i]);

		}
		xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesX[0], inter_valx, 0, 3, shiftx);
		xfPackPixels<NPC, WORDWIDTH_DST, DEPTH_DST>(&GradientValuesY[0], inter_valy, 0, 3, shifty);

		_gradx_mat.write(inter_valx);
		_grady_mat.write(inter_valy);
		shiftx = 0;
		shifty = 0;
		inter_valx = 0;
		inter_valy = 0;
	}
	else
	{
		src_buf1[6] = 0;
		src_buf2[6] = 0;
		src_buf3[6] = 0;
		src_buf4[6] = 0;
		src_buf5[6] = 0;
		src_buf6[6] = 0;
		src_buf7[6] = 0;

		for(ap_uint<5> k = 0; k < 3; k++)
		{
#pragma HLS LOOP_TRIPCOUNT min=3 max=3
#pragma HLS ALLOCATION instances=xFGradientX7x7 limit=1 function
#pragma HLS ALLOCATION instances=xFGradientY7x7 limit=1 function

			GradientValuesX[0] = xFGradientX7x7<DEPTH_SRC, DEPTH_DST>(&src_buf1[0], &src_buf2[0], &src_buf3[0],
					&src_buf4[0], &src_buf5[0], &src_buf6[0],
					&src_buf7[0]);

			GradientValuesY[0] = xFGradientY7x7<DEPTH_SRC, DEPTH_DST>(&src_buf1[0], &src_buf2[0], &src_buf3[0],
					&src_buf4[0], &src_buf5[0], &src_buf6[0],
					&src_buf7[0]);

			xfCopyData<NPC, DEPTH_SRC>(src_buf1, src_buf2, src_buf3, src_buf4,
					src_buf5, src_buf6, src_buf7);
			inter_valx((max_loop-1), (max_loop-32)) = GradientValuesX[0];
			inter_valy((max_loop-1), (max_loop-32)) = GradientValuesY[0];
			_gradx_mat.write(inter_valx);
			_grady_mat.write(inter_valy);
		}
	}

}
/**
 * xFSobelFilter7x7 : Computes Sobel gradient of the input image
 *                    for filter size 7x7
 * _src_mat		: Input image
 * _gradx_mat	: GradientX output
 * _grady_mat	: GradientY output
 */
template<int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void xFSobelFilter7x7(hls::stream< XF_SNAME(WORDWIDTH_SRC) > & _src_mat,
		hls::stream< XF_SNAME(WORDWIDTH_DST) > & _gradx_mat,
		hls::stream< XF_SNAME(WORDWIDTH_DST) > & _grady_mat, uint16_t img_height, uint16_t img_width)
{
	ap_uint<13> row_ind, row, col;
	ap_uint<4> tp1,tp2, tp3, mid, bottom1, bottom2, bottom3;
	ap_uint<5> i;

	// Gradient output values stored in these buffer
	XF_PTNAME(DEPTH_DST) GradientValuesX[XF_NPIXPERCYCLE(NPC)];
	XF_PTNAME(DEPTH_DST) GradientValuesY[XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=GradientValuesX complete dim=1
#pragma HLS ARRAY_PARTITION variable=GradientValuesY complete dim=1

	// Temporary buffers to hold image data from three rows.
	XF_PTNAME(DEPTH_SRC) src_buf1[XF_NPIXPERCYCLE(NPC) + 6], src_buf2[XF_NPIXPERCYCLE(NPC) + 6],
			src_buf3[XF_NPIXPERCYCLE(NPC) + 6], src_buf4[XF_NPIXPERCYCLE(NPC) + 6], src_buf5[XF_NPIXPERCYCLE(NPC) + 6];
	XF_PTNAME(DEPTH_SRC) src_buf6[XF_NPIXPERCYCLE(NPC) + 6], src_buf7[XF_NPIXPERCYCLE(NPC) + 6];
#pragma HLS ARRAY_PARTITION variable=src_buf1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf2 complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf3 complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf4 complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf5 complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf6 complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf7 complete dim=1

	XF_SNAME(WORDWIDTH_DST) inter_valx = 0, inter_valy = 0;
	uint16_t shiftx = 0, shifty = 0;

	XF_SNAME(WORDWIDTH_SRC) buf[7][(COLS >> XF_BITSHIFT(NPC))];
#pragma HLS RESOURCE variable=buf core=RAM_S2P_BRAM
#pragma HLS ARRAY_PARTITION variable=buf complete dim=1

	row_ind = 3;
	Clear_Row_Loop:
	for(col = 0; col < img_width; col++)
	{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
		buf[0][col] = 0;
		buf[1][col] = 0;
		buf[2][col] = 0;
		buf[row_ind][col] = _src_mat.read();
	}
	row_ind++;

	Read_Row1_Loop:
	for(col = 0; col < img_width; col++)
	{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline

		buf[row_ind][col] = _src_mat.read();
	}
	row_ind++;

	Read_Row2_Loop:
	for(col = 0; col < img_width; col++)
	{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline

		buf[row_ind][col] = _src_mat.read();
	}
	row_ind++;

	Row_Loop:
	for(row = 3; row < img_height+3; row++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		// modify the buffer indices to re use
		if(row_ind == 0)
		{
			tp1 = 1; tp2 = 2; tp3 = 3; mid = 4; bottom1 = 5; bottom2 = 6; bottom3 = 0;
		}
		else if(row_ind == 1)
		{
			tp1 = 2; tp2 = 3; tp3 = 4; mid = 5; bottom1 = 6; bottom2 = 0; bottom3 = 1;
		}
		else if(row_ind == 2)
		{
			tp1 = 3; tp2 = 4; tp3 = 5; mid = 6; bottom1 = 0; bottom2 = 1; bottom3 = 2;
		}
		else if(row_ind == 3)
		{
			tp1 = 4; tp2 = 5; tp3 = 6; mid = 0; bottom1 = 1; bottom2 = 2; bottom3 = 3;
		}
		else if(row_ind == 4)
		{
			tp1 = 5; tp2 = 6; tp3 = 0; mid = 1; bottom1 = 2; bottom2 = 3; bottom3 = 4;
		}
		else if(row_ind == 5)
		{
			tp1 = 6; tp2 = 0; tp3 = 1; mid = 2; bottom1 = 3; bottom2 = 4; bottom3 = 5;
		}
		else if(row_ind == 6)
		{
			tp1 = 0; tp2 = 1; tp3 = 2; mid = 3; bottom1 = 4; bottom2 = 5; bottom3 = 6;
		}

		for(i = 0; i < 6; i++)
		{
#pragma HLS unroll
			src_buf1[i] = 0;
			src_buf2[i] = 0;
			src_buf3[i] = 0;
			src_buf4[i] = 0;
			src_buf5[i] = 0;
			src_buf6[i] = 0;
			src_buf7[i] = 0;
		}
		inter_valx = inter_valy = 0;
		/***********		Process complete row			**********/
		ProcessSobel7x7<ROWS, COLS, DEPTH_SRC, DEPTH_DST, NPC, WORDWIDTH_SRC, WORDWIDTH_DST, TC>( _src_mat, _gradx_mat, _grady_mat, buf,  src_buf1,	src_buf2, src_buf3, src_buf4, src_buf5,	src_buf6, src_buf7,	GradientValuesX, GradientValuesY,
				inter_valx, inter_valy, img_width, img_height, row_ind, shiftx, shifty, tp1, tp2, tp3, mid, bottom1, bottom2, bottom3, row);

		RightBorder7x7<ROWS, COLS, DEPTH_SRC, DEPTH_DST, NPC, WORDWIDTH_SRC, WORDWIDTH_DST, TC>( _src_mat, _gradx_mat, _grady_mat, src_buf1,	src_buf2, src_buf3, src_buf4, src_buf5,	src_buf6, src_buf7,	GradientValuesX, GradientValuesY,
				inter_valx, inter_valy, shiftx, shifty);

		row_ind++;
		if(row_ind == 7)
		{
			row_ind = 0;
		}
	}//Row_Loop ends here
}
// xFSobelFilter7x7


/*********************************************************************
 * xFSobelFilter : Calls the Main Function depend on Requirements
 *********************************************************************/
template<int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFSobelFilter(hls::stream<XF_SNAME(WORDWIDTH_SRC)> &   _src,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _gradx,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _grady,
		int _filter_width,
		int _border_type,short int height,short int width)
{


#pragma HLS inline

	width = width >> XF_BITSHIFT(NPC);

	assert(((_filter_width == XF_FILTER_3X3) || (_filter_width == XF_FILTER_5X5) ||
			(_filter_width == XF_FILTER_7X7)) && " Filter width must be XF_FILTER_3X3, XF_FILTER_5X5 or XF_FILTER_7X7 ");

	assert((DEPTH_SRC == XF_8UP) && " Input image must be of type XF_8UP ");

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8) || (NPC == XF_NPPC16))
			&& "NPC must be XF_NPPC1, XF_NPPC8 or XF_NPPC16 ");

	assert((_border_type == XF_BORDER_CONSTANT) && "Border type must be XF_BORDER_CONSTANT ");

	assert(((height <= ROWS ) && (width <= COLS)) && "ROWS and COLS should be greater than input image");


	if(_filter_width == XF_FILTER_3X3)
	{
		xFSobelFilter3x3<ROWS, COLS, DEPTH_SRC, DEPTH_DST, NPC,
		WORDWIDTH_SRC, WORDWIDTH_DST,(COLS >> XF_BITSHIFT(NPC))>(_src, _gradx, _grady,height,width);
	}

	else if(_filter_width == XF_FILTER_5X5)
	{
		xFSobelFilter5x5<ROWS, COLS, DEPTH_SRC, DEPTH_DST, NPC,
		WORDWIDTH_SRC, WORDWIDTH_DST,(COLS >> XF_BITSHIFT(NPC))>(_src, _gradx, _grady,height,width);
	}

	else if(_filter_width == XF_FILTER_7X7)
	{
		xFSobelFilter7x7<ROWS, COLS, DEPTH_SRC, DEPTH_DST, NPC,
		WORDWIDTH_SRC, WORDWIDTH_DST,(COLS >> XF_BITSHIFT(NPC))>(_src, _gradx, _grady,height,width);
	}

}

#pragma SDS data mem_attribute("_src_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_dst_matx.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_dst_maty.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL, "_dst_matx.data":SEQUENTIAL,"_dst_maty.data":SEQUENTIAL)
//#pragma SDS data data_mover("_src_mat.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst_matx.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst_maty.data":AXIDMA_SIMPLE)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"], "_dst_matx.data"[0:"_dst_matx.size"],"_dst_maty.data"[0:"_dst_maty.size"])

template<int BORDER_TYPE,int FILTER_TYPE, int SRC_T,int DST_T, int ROWS, int COLS,int NPC>
void Sobel(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst_matx,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst_maty)
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



	xFSobelFilter<ROWS,COLS,XF_DEPTH(SRC_T,NPC),XF_DEPTH(DST_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(_src,_dstx,_dsty,FILTER_TYPE,BORDER_TYPE,_src_mat.rows,_src_mat.cols);

	
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
// xFSobelFilter
#endif // _XF_SOBEL_HPP_
