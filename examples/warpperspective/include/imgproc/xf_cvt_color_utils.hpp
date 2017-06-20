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
#ifndef _XF_CVT_COLOR_UTILS_HPP_
#define _XF_CVT_COLOR_UTILS_HPP_

#ifndef _XF_CVT_COLOR_HPP_
#error  This file can not be included independently !
#endif

#include <string.h>
#include "ap_int.h"
#include "common/xf_types.h"
#include "common/xf_structs.h"
#include "common/xf_params.h"
#include "common/xf_utility.h"

/***************************************************************************
 * 	Parameters reqd. for RGB to YUV conversion
 *	   -- Q1.15 format
 *	   -- A2X  A's contribution in calculation of X
 **************************************************************************/
#define R2Y		8422    //0.257
#define G2Y		16516   //0.504
#define B2Y		3212    //0.098
#define R2V		14382   //0.4389
#define G2V		53477		//-0.368
#define B2V		63209		//-0.071
#define R2U		60686		//-0.148
#define G2U		56000		//-0.291
#define B2U		14386		//0.439

/***************************************************************************
 * 	Parameters reqd. for YUV to RGB conversion
 *	   -- Q1.15 format
 *	   -- A2X  A's contribution in calculation of X
 *	   -- Only fractional part is taken for weigths greater
 *	      than 1 and interger part is added as offset
 **************************************************************************/
#define Y2R		5374    //0.164
#define U2R		0       //0
#define V2R		19530   //0.596
#define Y2G		5374    //0.164
#define U2G		52723   //-0.391
#define V2G		38895   //-0.813
#define Y2B		5374    //0.164
#define U2B		590     //0.018
#define V2B		0       //0

#define F_05	16384   // 0.5 in Q1.15 format

//template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int SIZE>
//void auPushIntoMat(XF_SNAME(WORDWIDTH_SRC)* _src, auviz::Mat<ROWS,COLS,DEPTH,NPC,WORDWIDTH_DST>& _dst, int scale)
//{
//	int max_loop = _dst.cols >> scale;
//	if(NPC==AU_NPPC1)
//	{
//		WR_STRM1:
//		for(int j = 0; j < max_loop; j+=2)
//		{
//#pragma HLS pipeline
//#pragma HLS LOOP_TRIPCOUNT min=SIZE max=SIZE
//			_dst.write(_src[j] | ((XF_SNAME(WORDWIDTH_DST))_src[j+1]) << AU_WORDDEPTH(WORDWIDTH_SRC));
//		}
//	}
//	else
//	{
//		WR_STRM:
//		for(int j = 0; j < max_loop; ++j)
//		{
//#pragma HLS pipeline
//#pragma HLS LOOP_TRIPCOUNT min=SIZE max=SIZE
//			_dst.write(_src[j]);
//		}
//	}
//}
//
//template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int SIZE>
//void auPullFromMat(auviz::Mat<ROWS, COLS, DEPTH, NPC, WORDWIDTH_SRC>& _src, XF_SNAME(WORDWIDTH_DST)* _dst)
//{
//	if(NPC==AU_NPPC1)
//	{
//		RD_STRM1:
//		for(int j = 0; j < _src.cols; j+=2)
//		{
//#pragma HLS pipeline
//#pragma HLS LOOP_TRIPCOUNT min=SIZE max=SIZE
//			XF_SNAME(WORDWIDTH_SRC) x = _src.read();
//			_dst[j] = x.range(AU_WORDDEPTH(WORDWIDTH_DST)-1,0);
//			_dst[j+1] = x.range(AU_WORDDEPTH(WORDWIDTH_SRC)-1,AU_WORDDEPTH(WORDWIDTH_DST));
//		}
//	}
//	else
//	{
//		RD_STRM:
//		for(int j = 0; j < _src.cols; ++j)
//		{
//#pragma HLS pipeline
//#pragma HLS LOOP_TRIPCOUNT min=SIZE max=SIZE
//			_dst[j] = _src.read();
//		}
//	}
//}

/**************************************************************************
 * Pack Chroma values into a single variable
 *************************************************************************/
//PackPixels
template<int WORDWIDTH>
XF_SNAME(WORDWIDTH) PackPixels(ap_uint8_t* buf)
{
	XF_SNAME(WORDWIDTH) val;
	for (int k = 0,l = 0; k < XF_WORDDEPTH(WORDWIDTH); k+=8, l++)
	{
#pragma HLS unroll
		// Get bits from certain range of positions.
		val.range(k+7, k) = buf[l];
	}
	return val;
}

/**************************************************************************
 * Extract UYVY Pixels from input stream
 *************************************************************************/
//ExtractUYVYPixels
template<int WORDWIDTH>
void ExtractUYVYPixels(XF_SNAME(WORDWIDTH) pix, ap_uint8_t *buf)
{
	int k;
	XF_SNAME(WORDWIDTH) val;
	int pos = 0;
	val = (XF_SNAME(WORDWIDTH))pix;
	for (k=0; k<(XF_WORDDEPTH(WORDWIDTH)); k+=8)
	{
#pragma HLS unroll
		uint8_t p;
		// Get bits from certain range of positions.
		p = val.range(k+7, k);
		buf[pos++] = p;
	}
}

/****************************************************************************
 * Extract R, G, B, A values into a buffer
 ***************************************************************************/
//ExtractRGBAPixels
template<int WORDDEPTH>
void ExtractRGBAPixels(XF_SNAME(WORDDEPTH) pix, uint8_t *buf)
{
	int k, pos = 0;
	uint8_t p;
	XF_SNAME(WORDDEPTH) val;
	val = (XF_SNAME(WORDDEPTH))pix;
	for (k=0; k< XF_WORDDEPTH(WORDDEPTH); k+=8)
	{
#pragma HLS unroll
		// Get bits from certain range of positions.
		p = val.range(k+7, k);
		buf[pos++] = p;
	}
}

/***********************************************************************
 * 	Pack R,G,B,A values into a single variable
 **********************************************************************/
//PackRGBAPixels
template<int WORDWIDTH>
XF_SNAME(WORDWIDTH) PackRGBAPixels(ap_uint8_t *buf )
{
	XF_SNAME(WORDWIDTH) val;
	for (int k = 0, l = 0; k<(XF_WORDDEPTH(WORDWIDTH)); k += 8, l++)
	{
#pragma HLS unroll
		// Get bits from certain range of positions.
		val.range(k+7, k) = buf[l];
	}
	return val;
}


////auWriteChroma420
//template<int ROWS,int COLS,int NPC,int WORDWIDTH>
//void auWriteChroma420(auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH>& plane,
//		XF_SNAME(WORDWIDTH) *dst, int off)
//{
//	bool flag = 0;
//	XF_SNAME(WORDWIDTH) ping[COLS>>NPC], pong[COLS>>NPC];
//	int nppc = AU_NPIXPERCYCLE(NPC);
//	int wordsize = plane.cols * nppc *(AU_PIXELDEPTH(AU_8UP)>>3);
//
//	int i, dst_off = off*(plane.cols);
//	auReadFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(plane, ping);
//	WRUV420:
//	for( i = 0 ; i < (plane.rows-1); i++, dst_off += plane.cols)
//	{
//#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
//		if(flag == 0)
//		{
//			auReadFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(plane, pong);
//			auCopyMemoryOut<COLS, WORDWIDTH>(ping, dst, dst_off,  wordsize);
//			flag = 1;
//		}
//		else if(flag == 1)
//		{
//			auReadFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(plane, ping);
//			auCopyMemoryOut<COLS, WORDWIDTH>(pong, dst, dst_off,  wordsize);
//			flag = 0;
//		}
//	}
//	if(flag == 1)
//		auCopyMemoryOut<COLS, WORDWIDTH>(pong, dst, dst_off, wordsize);
//	else
//		auCopyMemoryOut<COLS, WORDWIDTH>(ping, dst, dst_off, wordsize);
//}
//
//template <int WORDWIDTH>
//void auReadDummy(XF_SNAME(WORDWIDTH)* src)
//{
//	XF_SNAME(WORDWIDTH) dummy[1];
//	int pixelsize = AU_WORDDEPTH(WORDWIDTH)>>3;
//	memcpy((XF_SNAME(WORDWIDTH)*)dummy , (XF_SNAME(WORDWIDTH)*)src , pixelsize);
//}
//
//template <int WORDWIDTH>
//void auWriteDummy(XF_SNAME(WORDWIDTH)* ptr,XF_SNAME(WORDWIDTH) *dst)
//{
//	int pixelsize = AU_WORDDEPTH(WORDWIDTH)>>3;
//	memcpy((XF_SNAME(WORDWIDTH)*)dst , (XF_SNAME(WORDWIDTH)*)ptr , pixelsize);
//}
//
//template <int WORDWIDTH>
//void auWriteDummy1(XF_SNAME(WORDWIDTH)* ptr,XF_SNAME(WORDWIDTH) *dst)
//{
//	int pixelsize = AU_WORDDEPTH(WORDWIDTH)>>3;
//	memcpy((XF_SNAME(WORDWIDTH)*)dst , (XF_SNAME(WORDWIDTH)*)ptr , pixelsize);
//}
//auWriteUV420
//template<int WORDWIDTH_UV, int WORDWIDTH_DST, int NPC, int ROWS, int COLS>
//void auWriteUV420(auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH_UV>& plane, XF_SNAME(WORDWIDTH_DST)* dst, int off)
//{
//
//	bool flag = 0;
//	XF_SNAME(WORDWIDTH_DST) ping[COLS>>NPC], pong[COLS>>NPC];
//	int nppc = AU_NPIXPERCYCLE(NPC);
//	int wordsize = plane.cols * nppc*(AU_PIXELDEPTH(AU_8UP)>>3);
//	int i;
//
//	int dst_off = off * plane.cols;
//
//	auPullFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH_UV,WORDWIDTH_DST,(COLS>>NPC)>(plane, ping);
//	WRUV420:
//	for( i = 0 ; i < (plane.rows-1); i++)
//	{
//#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
//		if(flag == 0)
//		{
//			auPullFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH_UV,WORDWIDTH_DST,(COLS>>NPC)>(plane, pong);
//			auCopyMemoryOut<COLS, WORDWIDTH_DST>(ping, dst, dst_off,  wordsize);
//			flag = 1;
//		}
//		else if(flag == 1)
//		{
//			auPullFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH_UV,WORDWIDTH_DST,(COLS>>NPC)>(plane, ping);
//			auCopyMemoryOut<COLS, WORDWIDTH_DST>(pong, dst, dst_off,  wordsize);
//			flag = 0;
//		}
//		dst_off += plane.cols;
//	}
//	if(flag == 1)
//		auCopyMemoryOut<COLS, WORDWIDTH_DST>(pong, dst, dst_off,  wordsize);
//	else
//		auCopyMemoryOut<COLS, WORDWIDTH_DST>(ping, dst, dst_off,  wordsize);
//}
//auWriteYuv444
//template<int ROWS, int COLS, int NPC, int WORDWIDTH>
//void auWriteYuv444(
//		auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH>& y_plane,
//		auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH>& u_plane,
//		auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH>& v_plane,
//		XF_SNAME(WORDWIDTH)* dst0,
//		XF_SNAME(WORDWIDTH)* dst1,
//		XF_SNAME(WORDWIDTH)* dst2)
//{
//	bool flag = 0;
//	XF_SNAME(WORDWIDTH) ping1[COLS>>NPC], pong1[COLS>>NPC];
//	XF_SNAME(WORDWIDTH) ping2[COLS>>NPC], pong2[COLS>>NPC];
//	XF_SNAME(WORDWIDTH) ping3[COLS>>NPC], pong3[COLS>>NPC];
//
//	int nppc = AU_NPIXPERCYCLE(NPC);
//	int wordsize = y_plane.cols * nppc *(AU_PIXELDEPTH(AU_8UP)>>3);
//	int i;
//
//	auReadFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(y_plane, ping1);
//	auReadFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(u_plane, ping2);
//	auReadFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(v_plane, ping3);
//
//	WRUV420:
//	for( i = 0 ; i < (y_plane.rows-1); i++)
//	{
//#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
//		if(flag == 0)
//		{
//			auReadFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(y_plane, pong1);
//			auReadFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(u_plane, pong2);
//			auReadFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(v_plane, pong3);
//
//			auCopyMemoryOut<COLS, WORDWIDTH>(ping1, dst0, (i)*y_plane.cols,  wordsize);
//			auCopyMemoryOut<COLS, WORDWIDTH>(ping2, dst1, (i)*u_plane.cols,  wordsize);
//			auCopyMemoryOut<COLS, WORDWIDTH>(ping3, dst2, (i)*v_plane.cols,  wordsize);
//			//auCopyMemoryOut<COLS, WORDWIDTH>(ping2, dst, (i+y_plane.rows)*u_plane.cols,  wordsize);
//			//auCopyMemoryOut<COLS, WORDWIDTH>(ping3, dst, (i+(y_plane.rows<<1))*v_plane.cols,  wordsize);
//			flag = 1;
//		}
//		else if(flag == 1)
//		{
//			auReadFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(y_plane, ping1);
//			auReadFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(u_plane, ping2);
//			auReadFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(v_plane, ping3);
//
//			auCopyMemoryOut<COLS, WORDWIDTH>(pong1, dst0, (i)*y_plane.cols,  wordsize);
//			auCopyMemoryOut<COLS, WORDWIDTH>(pong2, dst1, (i)*u_plane.cols,  wordsize);
//			auCopyMemoryOut<COLS, WORDWIDTH>(pong3, dst2, (i)*v_plane.cols,  wordsize);
//			//auCopyMemoryOut<COLS, WORDWIDTH>(pong2, dst, (i+y_plane.rows)*u_plane.cols,  wordsize);
//			//auCopyMemoryOut<COLS, WORDWIDTH>(pong3, dst, (i+(y_plane.rows<<1))*v_plane.cols,  wordsize);
//			flag = 0;
//		}
//	}
//	if(flag == 1)
//	{
//		auCopyMemoryOut<COLS, WORDWIDTH>(pong1, dst0, (i)*y_plane.cols, wordsize);
//		auCopyMemoryOut<COLS, WORDWIDTH>(pong2, dst1, (i)*u_plane.cols,  wordsize);
//		auCopyMemoryOut<COLS, WORDWIDTH>(pong3, dst2, (i)*v_plane.cols,  wordsize);
//		//auCopyMemoryOut<COLS, WORDWIDTH>(pong2, dst, (i+y_plane.rows)*u_plane.cols,  wordsize);
//		//auCopyMemoryOut<COLS, WORDWIDTH>(pong3, dst, (i+(y_plane.rows<<1))*v_plane.cols,  wordsize);
//	}
//	else
//	{
//		auCopyMemoryOut<COLS, WORDWIDTH>(ping1, dst0, (i)*y_plane.cols, wordsize);
//		auCopyMemoryOut<COLS, WORDWIDTH>(ping2, dst1, (i+y_plane.rows)*u_plane.cols, wordsize);
//		auCopyMemoryOut<COLS, WORDWIDTH>(ping3, dst2, (i+(y_plane.rows<<1))*v_plane.cols, wordsize);
//		//auCopyMemoryOut<COLS, WORDWIDTH>(ping2, dst, (i+y_plane.rows)*u_plane.cols, wordsize);
//		//auCopyMemoryOut<COLS, WORDWIDTH>(ping3, dst, (i+(y_plane.rows<<1))*v_plane.cols, wordsize);
//	}
//}
//auWriteRgba
//template<int ROWS, int COLS,int DEPTH,int NPC,int WORDWIDTH>
//void auWriteRgba(auviz::Mat<ROWS, COLS, AU_32UP, NPC, WORDWIDTH>& plane, XF_SNAME(WORDWIDTH)* dst0, XF_SNAME(WORDWIDTH)* dst1, XF_SNAME(WORDWIDTH)* dst2)
//{
//	XF_SNAME(WORDWIDTH) dummy0[1],dummy1[1];
//	dummy0[0] = 0;dummy1[0] = 0;
//
//	auWriteImage<ROWS, COLS, DEPTH, NPC, WORDWIDTH>(plane, dst0);
//	auWriteDummy<WORDWIDTH>(dummy0,dst1);
//	auWriteDummy1<WORDWIDTH>(dummy1,dst2);
//}
//
//
//template<int WORDWIDTH, int NPC, int ROWS, int COLS>
//void auWriteRgba_in(auviz::Mat<ROWS, COLS, AU_32UP, NPC, WORDWIDTH>& plane, XF_SNAME(WORDWIDTH)* dst0, XF_SNAME(WORDWIDTH)* dst1, XF_SNAME(WORDWIDTH)* dst2)
//{
//	bool flag = 0;
//	XF_SNAME(WORDWIDTH) ping[COLS>>NPC], pong[COLS>>NPC];
//	XF_SNAME(WORDWIDTH) dummy0[1],dummy1[1];
//	dummy0[0] = 0;dummy1[0] = 0;
//
//	int pixelsize = AU_WORDDEPTH(WORDWIDTH);
//	int nppc = AU_NPIXPERCYCLE(NPC);
//	int size = plane.cols * nppc *(AU_PIXELDEPTH(AU_32UP)>>3);
//	int i, offset = 0;
//
//	auReadFromMat<ROWS,COLS,AU_32UP,NPC,WORDWIDTH,(COLS>>NPC)>(plane, ping);
//	WR_Rgba:
//	for( i = 0 ; i < (plane.rows-1); i++)
//	{
//#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
//		if(flag == 0)
//		{
//			auReadFromMat<ROWS,COLS,AU_32UP,NPC,WORDWIDTH,(COLS>>NPC)>(plane, pong);
//			auCopyMemoryOut<COLS*(AU_PIXELDEPTH(AU_32UP)>>3), WORDWIDTH>(ping, dst0, offset, size);
//			flag = 1;
//		}
//		else if(flag == 1)
//		{
//			auReadFromMat<ROWS,COLS,AU_32UP,NPC,WORDWIDTH,(COLS>>NPC)>(plane, ping);
//			auCopyMemoryOut<COLS*(AU_PIXELDEPTH(AU_32UP)>>3), WORDWIDTH>(pong, dst0, offset,  size);
//			flag = 0;
//		}
//		offset += plane.cols;
//	}
//	if(flag == 1)
//		auCopyMemoryOut<COLS*(AU_PIXELDEPTH(AU_32UP)>>3), WORDWIDTH>(pong, dst0, offset,  size);
//	else
//		auCopyMemoryOut<COLS*(AU_PIXELDEPTH(AU_32UP)>>3), WORDWIDTH>(ping, dst0, offset, size);
//
//	memcpy((XF_SNAME(WORDWIDTH)*)dst1, (XF_SNAME(WORDWIDTH)*)dummy0 , pixelsize);
//	memcpy((XF_SNAME(WORDWIDTH)*)dst2, (XF_SNAME(WORDWIDTH)*)dummy0 , pixelsize);
//}
//
//
////auWriteUV444
//template<int TC, int WORDWIDTH, int NPC, int ROWS, int COLS>
//void auWriteUV444(auviz::Mat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH>& src, XF_SNAME(WORDWIDTH)* dst, int Offset)
//{
//	XF_SNAME(WORDWIDTH) ping[COLS>>NPC], pong[COLS>>NPC];
//	bool flag = 0;
//	int nppc = AU_NPIXPERCYCLE(NPC);
//	int wordsize = src.cols*nppc*(AU_PIXELDEPTH(AU_8UP)>>3);
//	int i;
//
//	auReadFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(src, ping);
//	WR_UV444:
//	for(i = 0; i < ((src.rows+1)>>1)-1; i++)
//	{
//#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
//		if(flag == 0)
//		{
//			auReadFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(src, pong);
//			auCopyMemoryOut<COLS, WORDWIDTH>(ping, dst,  ((i<<1)+Offset)*src.cols,  wordsize);
//			auCopyMemoryOut<COLS, WORDWIDTH>(ping, dst,  (((i<<1)+1)+Offset)*src.cols,  wordsize);
//			flag = 1;
//		}
//		else if(flag == 1)
//		{
//			auReadFromMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(src, ping);
//			auCopyMemoryOut<COLS, WORDWIDTH>(pong, dst,  ((i<<1)+Offset)*src.cols,  wordsize);
//			auCopyMemoryOut<COLS, WORDWIDTH>(pong, dst,  (((i<<1)+1)+Offset)*src.cols,  wordsize);
//			flag = 0;
//		}
//	}
//	if(flag == 1)
//	{
//		auCopyMemoryOut<COLS, WORDWIDTH>(pong, dst, ((i<<1)+Offset)*src.cols,  wordsize);
//		auCopyMemoryOut<COLS, WORDWIDTH>(pong, dst, (((i<<1)+1)+Offset)*src.cols,  wordsize);
//	}
//	else if(flag == 0)
//	{
//		auCopyMemoryOut<COLS, WORDWIDTH>(ping, dst, ((i<<1)+Offset)*src.cols,  wordsize);
//		auCopyMemoryOut<COLS, WORDWIDTH>(ping, dst, (((i<<1)+1)+Offset)*src.cols,  wordsize);
//	}
//}
//
//
////auReadUV420
//template<int WORDWIDTH_SRC, int WORDWIDTH_DST, int NPC, int ROWS, int COLS>
//void auReadUV420(XF_SNAME(WORDWIDTH_SRC)* src, auviz::Mat<ROWS, COLS, AU_8UP, NPC,WORDWIDTH_DST>& dst, int Offset)
//{
//	bool flag = 0;
//	XF_SNAME(WORDWIDTH_SRC) ping[COLS>>NPC], pong[COLS>>NPC];
//	int src_off = Offset*(dst.cols);
//	int nppc = AU_NPIXPERCYCLE(NPC);
//	int wordsize = dst.cols*nppc*(AU_PIXELDEPTH(AU_8UP)>>3);
//
//	auCopyMemoryIn<COLS, WORDWIDTH_SRC>(src, ping, src_off, wordsize);
//	RD_UV420:
//	for(int i = 1 ; i < (dst.rows); i++)
//	{
//#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
//		src_off += dst.cols;
//		if(flag == 0)
//		{
//			auCopyMemoryIn<COLS, WORDWIDTH_SRC>(src, pong, src_off, wordsize);
//			auPushIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,(COLS>>NPC)>(ping,dst,0);
//			flag = 1;
//		}
//		else if(flag == 1)
//		{
//			auCopyMemoryIn<COLS, WORDWIDTH_SRC>(src, ping, src_off, wordsize);
//			auPushIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,(COLS>>NPC)>(pong,dst,0);
//			flag = 0;
//		}
//	}
//	if(flag == 1)
//		auPushIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,(COLS>>NPC)>(pong,dst,0);
//	else if(flag == 0)
//		auPushIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,(COLS>>NPC)>(ping,dst,0);
//}

//template<int WORDWIDTH, int NPC, int ROWS, int COLS>
//void auReadRgb_plane(XF_SNAME(WORDWIDTH)* src, auviz::Mat<ROWS, COLS, AU_32UP, NPC, WORDWIDTH>& dst)
//{
//	bool flag = 0;
//	XF_SNAME(WORDWIDTH) ping[COLS>>NPC], pong[COLS>>NPC];
//	int nppc = AU_NPIXPERCYCLE(NPC);
//	int size = dst.cols* nppc*(AU_PIXELDEPTH(AU_32UP)>>3);
//	int src_off = 0;
//
//	auCopyMemoryIn<COLS*(AU_PIXELDEPTH(AU_32UP)>>3), WORDWIDTH>(src, ping, 0, size);
//	for(int i = 1 ; i < (dst.rows); i++)
//	{
//#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
//		src_off += dst.cols;
//		if(flag == 0)
//		{
//			auCopyMemoryIn<COLS*(AU_PIXELDEPTH(AU_32UP)>>3), WORDWIDTH>(src, pong, src_off, size);
//			auWriteIntoMat<ROWS,COLS,AU_32UP,NPC,WORDWIDTH,(COLS>>NPC)>(ping, dst);
//			flag = 1;
//		}
//		else if(flag == 1)
//		{
//			auCopyMemoryIn<COLS*(AU_PIXELDEPTH(AU_32UP)>>3), WORDWIDTH>(src, ping, src_off, size);
//			auWriteIntoMat<ROWS,COLS,AU_32UP,NPC,WORDWIDTH,(COLS>>NPC)>(pong, dst);
//			flag = 0;
//		}
//	}
//	if(flag == 1)
//		auWriteIntoMat<ROWS,COLS,AU_32UP,NPC,WORDWIDTH,(COLS>>NPC)>(pong, dst);
//	else if(flag == 0)
//		auWriteIntoMat<ROWS,COLS,AU_32UP,NPC,WORDWIDTH,(COLS>>NPC)>(ping, dst);
//}

//auReadRgb
//template<int WORDWIDTH, int NPC, int ROWS, int COLS>
//void auReadRgb(
//		XF_SNAME(WORDWIDTH)* src0,
//		XF_SNAME(WORDWIDTH)* src1,
//		XF_SNAME(WORDWIDTH)* src2,
//		auviz::Mat<ROWS, COLS, AU_32UP, NPC, WORDWIDTH>& rgba
//		)
//{
//	auReadImage<ROWS,COLS,AU_32UP,NPC,WORDWIDTH>(src0, rgba);
//	auReadDummy<WORDWIDTH>(src1);
//	auReadDummy<WORDWIDTH>(src2);
//}
//
//
//template<int WORDWIDTH_SRC, int WORDWIDTH_DST, int NPC, int ROWS, int COLS>
//void auReadUyvy_plane(XF_SNAME(WORDWIDTH_SRC)* src, auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH_DST>& dst)
//{
//	bool flag = 0;
//	XF_SNAME(WORDWIDTH_SRC) ping[COLS>>(NPC+1)], pong[COLS>>(NPC+1)];
//	int nppc = AU_NPIXPERCYCLE(NPC);
//	int wordsize = (dst.cols)*nppc*(AU_PIXELDEPTH(AU_8UP)>>3);
//	int offset = (dst.cols>>1);
//
//	auCopyMemoryIn<COLS, WORDWIDTH_SRC>(src, ping, 0, wordsize);
//	for(int i = 1 ; i < (dst.rows); i++)
//	{
//#pragma HLS LOOP TRIPCOUNT min=ROWS max=ROWS
//
//		if(flag == 0)
//		{
//			auCopyMemoryIn<COLS, WORDWIDTH_SRC>(src, pong, offset, wordsize);
//			auPushIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>NPC)>>1)>(ping, dst, 1);
//			flag = 1;
//		}
//		else if(flag == 1)
//		{
//			auCopyMemoryIn<COLS, WORDWIDTH_SRC>(src, ping, offset, wordsize);
//			auPushIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>NPC)>>1)>(pong, dst, 1);
//			flag = 0;
//		}
//		offset += (dst.cols>>1);
//	}
//	if(flag == 1)
//		auPushIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>NPC)>>1)>(pong, dst, 1);
//	else if(flag == 0)
//		auPushIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>NPC)>>1)>(ping, dst, 1);
//}

//auReadUyvy
//template<int WORDWIDTH_SRC, int WORDWIDTH_DST, int NPC, int ROWS, int COLS>
//void auReadUyvy(
//		XF_SNAME(WORDWIDTH_SRC)* src0,XF_SNAME(WORDWIDTH_SRC)* src1,XF_SNAME(WORDWIDTH_SRC)* src2,
//		 auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH_DST>& in_uyvy)
//{
//	auReadUyvy_plane<WORDWIDTH_SRC>(src0, in_uyvy);
//	auReadDummy<WORDWIDTH_SRC>(src1);
//	auReadDummy<WORDWIDTH_SRC>(src2);
//}
/*
 template<int WORDWIDTH_SRC, int WORDWIDTH_DST, int NPC, int ROWS, int COLS>
 void auReadUyvy(XF_SNAME(WORDWIDTH_SRC)* src, auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH_DST>& dst)
 {




 bool flag = 0;
 XF_SNAME(WORDWIDTH_SRC) ping[COLS>>(NPC+1)], pong[COLS>>(NPC+1)];
 int nppc = AU_NPIXPERCYCLE(NPC);
 int wordsize = (dst.cols)*nppc*(AU_PIXELDEPTH(AU_8UP)>>3);
 int offset = (dst.cols>>1);

 auCopyMemoryIn<COLS, WORDWIDTH_SRC>(src, ping, 0, wordsize);
 for(int i = 1 ; i < (dst.rows); i++)
 {
 #pragma HLS LOOP TRIPCOUNT min=ROWS max=ROWS

 if(flag == 0)
 {
 auCopyMemoryIn<COLS, WORDWIDTH_SRC>(src, pong, offset, wordsize);
 auPushIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>NPC)>>1)>(ping, dst, 1);
 flag = 1;
 }
 else if(flag == 1)
 {
 auCopyMemoryIn<COLS, WORDWIDTH_SRC>(src, ping, offset, wordsize);
 auPushIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>NPC)>>1)>(pong, dst, 1);
 flag = 0;
 }
 offset += (dst.cols>>1);
 }
 if(flag == 1)
 auPushIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>NPC)>>1)>(pong, dst, 1);
 else if(flag == 0)
 auPushIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>NPC)>>1)>(ping, dst, 1);
 }
 */
//auReadChroma420
//template<int WORDWIDTH, int NPC, int ROWS, int COLS>
//void auReadChroma420(
//		XF_SNAME(WORDWIDTH)* src,
//		auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH>& dst,
//		int Offset)
//{
//	bool flag = 0;
//	XF_SNAME(WORDWIDTH) ping[COLS>>NPC], pong[COLS>>NPC];
//	int nppc = AU_NPIXPERCYCLE(NPC);
//	int wordsize = dst.cols*nppc*(AU_PIXELDEPTH(AU_8UP)>>3);
//	int src_off = Offset*(dst.cols);
//
//	auCopyMemoryIn<COLS, WORDWIDTH>(src, ping, src_off, wordsize);
//	for(int i = 1 ; i < (dst.rows); i++)
//	{
//#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
//		src_off += dst.cols;
//		if(flag == 0)
//		{
//			auCopyMemoryIn<COLS, WORDWIDTH>(src, pong, src_off, wordsize);
//			auWriteIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(ping, dst);
//			flag = 1;
//		}
//		else if(flag == 1)
//		{
//			auCopyMemoryIn<COLS, WORDWIDTH>(src, ping, src_off, wordsize);
//			auWriteIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(pong, dst);
//			flag = 0;
//		}
//
//	}
//	if(flag == 1)
//		auWriteIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(pong, dst);
//	else if(flag == 0)
//		auWriteIntoMat<ROWS,COLS,AU_8UP,NPC,WORDWIDTH,(COLS>>NPC)>(ping, dst);
//}

//auWriteIyuv
//template<int ROWS,int COLS,int NPC,int WORDWIDTH>
//void auWriteIyuv(
//		auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH>& y_plane,
//		auviz::Mat<(ROWS>>2), COLS, AU_8UP, NPC, WORDWIDTH>& u_plane,
//		auviz::Mat<(ROWS>>2), COLS, AU_8UP, NPC, WORDWIDTH>& v_plane,
//		XF_SNAME(WORDWIDTH) *dst0,XF_SNAME(WORDWIDTH) *dst1,XF_SNAME(WORDWIDTH) *dst2)
//{
//	auWriteImage<ROWS, COLS, AU_8UP, NPC, WORDWIDTH>(y_plane, dst0);
//	auWriteChroma420<(ROWS>>2),COLS,NPC,WORDWIDTH>(u_plane, dst1, 0);
//	auWriteChroma420<(ROWS>>2),COLS,NPC,WORDWIDTH>(v_plane, dst2, 0);
//}
//auWriteNV12
//template<int WORDWIDTH_Y, int WORDWIDTH_UV, int NPC, int ROWS, int COLS>
//void auWriteNV12(
//		auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH_Y>& y_plane,
//		auviz::Mat<((ROWS+1)>>1), COLS, AU_8UP, NPC, WORDWIDTH_UV>& uv_plane,
//		XF_SNAME(WORDWIDTH_Y)* dst0,XF_SNAME(WORDWIDTH_Y)* dst1,XF_SNAME(WORDWIDTH_Y)* dst2)
//{
//	XF_SNAME(WORDWIDTH_Y) dummy[1];
//	dummy[0] = 0;
//	auWriteImage<ROWS, COLS, AU_8UP, NPC, WORDWIDTH_Y>(y_plane, dst0);
//	auWriteUV420<WORDWIDTH_UV, WORDWIDTH_Y, NPC>(uv_plane, dst1, 0);
//	auWriteDummy<WORDWIDTH_Y>(dummy,dst2);
//}

//auReadIyuv
//template<int WORDWIDTH, int NPC, int ROWS, int COLS>
//void auReadIyuv(
//		XF_SNAME(WORDWIDTH)* src0,
//		XF_SNAME(WORDWIDTH)* src1,
//		XF_SNAME(WORDWIDTH)* src2,
//		auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH>& y_plane,
//		auviz::Mat<(ROWS>>2), COLS, AU_8UP, NPC, WORDWIDTH>& u_plane,
//		auviz::Mat<(ROWS>>2), COLS, AU_8UP, NPC, WORDWIDTH>& v_plane)
//{
//	int off = y_plane.rows & 0x3 ? (y_plane.rows>>2)+1 : (y_plane.rows>>2);
//	auReadImage(src0, y_plane);
//	auReadChroma420<WORDWIDTH, NPC>(src1, u_plane, 0);
//	auReadChroma420<WORDWIDTH, NPC>(src2, v_plane, 0);
//}

//auReadNV12
//template<int WORDWIDTH_Y, int WORDWIDTH_UV, int NPC, int ROWS, int COLS>
//void auReadNV12(
//		XF_SNAME(WORDWIDTH_Y)* src0,XF_SNAME(WORDWIDTH_Y)* src1,XF_SNAME(WORDWIDTH_Y)* src2,
//		auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH_Y>& in_y,
//		auviz::Mat<((ROWS+1)>>1), COLS, AU_8UP, NPC, WORDWIDTH_UV>& in_uv)
//{
//	auReadImage(src0, in_y);
//	auReadUV420<WORDWIDTH_Y,WORDWIDTH_UV,NPC,((ROWS+1)>>1),COLS>(src1, in_uv,0);
//	auReadDummy<WORDWIDTH_Y>(src2);
//}

//auWriteYuv4
//template<int WORDWIDTH, int NPC, int ROWS, int COLS>
//void auWriteYuv4(
//		auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH>& y_plane,
//		auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH>& u_plane,
//		auviz::Mat<ROWS, COLS, AU_8UP, NPC, WORDWIDTH>& v_plane,
//		XF_SNAME(WORDWIDTH)* dst0,XF_SNAME(WORDWIDTH)* dst1,XF_SNAME(WORDWIDTH)* dst2)
//{
//	auWriteImage(y_plane, dst0);
//	auWriteUV444<((ROWS+1)>>1)-1>(u_plane, dst1, 0);
//	auWriteUV444<((ROWS+1)>>1)-1>(v_plane, dst2, 0);
//}

/****************************************************************************
 * 	Function to add the offset and check the saturation
 ***************************************************************************/
static uint8_t saturate_cast(int32_t Value, int32_t offset) {
	// Right shifting Value 15 times to get the integer part
	int Value_int = (Value >> 15) + offset;
	unsigned char Value_uchar = 0;
	if (Value_int > 255)
		Value_uchar = 255;
	else if (Value_int < 0)
		Value_uchar = 0;
	else
		Value_uchar = (uint8_t) Value_int;

	return Value_uchar;
}

/****************************************************************************
 * 	CalculateY - calculates the Y(luma) component using R,G,B values
 * 	Y = (0.257 * R) + (0.504 * G) + (0.098 * B) + 16
 * 	An offset of 16 is added to the resultant value
 ***************************************************************************/
static uint8_t CalculateY(uint8_t R, uint8_t G, uint8_t B) {
#pragma HLS INLINE
	// 1.15 * 8.0 = 9.15
	int32_t Y = ((short int) R2Y * R) + ((short int) G2Y * G)
			+ ((short int) B2Y * B) + F_05;
	uint8_t Yvalue = saturate_cast(Y, 16);
	return Yvalue;
}

/***********************************************************************
 * CalculateU - calculates the U(Chroma) component using R,G,B values
 * U = -(0.148 * R) - (0.291 * G) + (0.439 * B) + 128
 * an offset of 128 is added to the resultant value
 **********************************************************************/
static uint8_t CalculateU(uint8_t R, uint8_t G, uint8_t B) {
#pragma HLS INLINE
	int32_t U = ((short int) R2U * R) + ((short int) G2U * G)
			+ ((short int) B2U * B) + F_05;
	uint8_t Uvalue = saturate_cast(U, 128);
	return Uvalue;
}

/***********************************************************************
 * CalculateV - calculates the V(Chroma) component using R,G,B values
 * V = (0.439 * R) - (0.368 * G) - (0.071 * B) + 128
 * an offset of 128 is added to the resultant value
 **********************************************************************/
static uint8_t CalculateV(uint8_t R, uint8_t G, uint8_t B) {
#pragma HLS INLINE
	int32_t V = ((short int) R2V * R) + ((short int) G2V * G)
			+ ((short int) B2V * B) + F_05;
	uint8_t Vvalue = saturate_cast(V, 128);
	return Vvalue;
}

/***********************************************************************
 * CalculateR - calculates the R(Red) component using Y & V values
 * R = 1.164*Y + 1.596*V = 0.164*Y + 0.596*V + Y + V
 **********************************************************************/
static uint8_t CalculateR(uint8_t Y, int32_t V2Rtemp, int8_t V) {
#pragma HLS INLINE
	int32_t R = (short int) Y2R * Y + V2Rtemp + F_05;
	uint8_t Rvalue = saturate_cast(R, V + Y);
	return (Rvalue);
}

/***********************************************************************
 * CalculateG - calculates the G(Green) component using Y, U & V values
 * G = 1.164*Y - 0.813*V - 0.391*U = 0.164*Y - 0.813*V - 0.391*U + Y
 **********************************************************************/
static uint8_t CalculateG(uint8_t Y, int32_t U2Gtemp, int32_t V2Gtemp) {
#pragma HLS INLINE
	int32_t G = (short int) Y2G * Y + U2Gtemp + V2Gtemp + F_05;
	uint8_t Gvalue = saturate_cast(G, Y);
	return (Gvalue);
}

/***********************************************************************
 * CalculateB - calculates the B(Blue) component using Y & U values
 * B = 1.164*Y + 2.018*U = 0.164*Y + Y + 0.018*U + 2*U
 **********************************************************************/
static uint8_t CalculateB(uint8_t Y, int32_t U2Btemp, int8_t U) {
#pragma HLS INLINE
	int32_t B = (short int) Y2B * Y + U2Btemp + F_05;
	uint8_t Bvalue = saturate_cast(B, 2 * U + Y);
	return (Bvalue);
}

#endif // _XF_CVT_COLOR_UTILS_H_
