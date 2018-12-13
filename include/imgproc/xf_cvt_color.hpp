
/***************************************************************************
Copyright (c) 2018, Xilinx, Inc.
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
#ifndef _XF_CVT_COLOR_HPP_
#define _XF_CVT_COLOR_HPP_

#include "hls_stream.h"
#include "common/xf_common.h"
#include "xf_cvt_color_1.hpp"
#include "xf_cvt_color_utils.hpp"
#include <assert.h>

namespace xf {
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC,int TC>
void write_y(hls::stream< XF_SNAME(WORDWIDTH_SRC) >& src0,hls::stream< XF_SNAME(WORDWIDTH_SRC) >& dst0,uint16_t height,uint16_t width)
{
XF_SNAME(WORDWIDTH_SRC) tmp;
for(int i=0;i<height;i++)
{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
	for(int i=0;i<width>>XF_BITSHIFT(NPC);i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
		tmp=src0.read();
		dst0.write(tmp);
	}
}
}

//KernRgba2Yuv4
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernRgba2Yuv4(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& src,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& dst1,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& dst2,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& dst3,uint16_t height,uint16_t width)
{
//	width=width>>NPC;
	XF_PTNAME(XF_8UP) Y0[16], U[16], V[16];
	uint8_t RGB[64];
#pragma HLS ARRAY_PARTITION variable=Y0 complete
#pragma HLS ARRAY_PARTITION variable=U complete
#pragma HLS ARRAY_PARTITION variable=V complete
#pragma HLS ARRAY_PARTITION variable=RGB complete

	XF_SNAME(WORDWIDTH_SRC)  PackedPixels;
	XF_SNAME(WORDWIDTH_DST) YPacked, UPacked, VPacked;
	uint8_t offset;

	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			PackedPixels = src.read();
			ExtractRGBAPixels<WORDWIDTH_SRC>(PackedPixels, RGB);
			//	Converting from RGBA to YUV4
			//		Y =  (0.257 * R) + (0.504 * G) + (0.098 * B) + 16
			//		U = -(0.148 * R) - (0.291 * G) + (0.439 * B) + 128
			//		V =  (0.439 * R) - (0.368 * G) - (0.071 * B) + 128
			for(int l = 0; l < (1<<NPC)>>1; l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
				//#pragma HLS unroll
				offset = l << 3;
				Y0[(l<<1)]   = CalculateY(RGB[offset + 0], RGB[offset + 1], RGB[offset + 2]);
				Y0[(l<<1)+1] = CalculateY(RGB[offset + 4], RGB[offset + 5], RGB[offset + 6]);

				U[(l<<1)]   = CalculateU(RGB[offset + 0], RGB[offset + 1], RGB[offset + 2]);
				U[(l<<1)+1] = CalculateU(RGB[offset + 4], RGB[offset + 5], RGB[offset + 6]);

				V[(l<<1)]   = CalculateV(RGB[offset + 0], RGB[offset + 1], RGB[offset + 2]);
				V[(l<<1)+1] = CalculateV(RGB[offset + 4], RGB[offset + 5], RGB[offset + 6]);
			}
			YPacked = PackPixels<WORDWIDTH_DST>(Y0);
			UPacked = PackPixels<WORDWIDTH_DST>(U);
			VPacked = PackPixels<WORDWIDTH_DST>(V);

			dst1.write(YPacked);
			dst2.write(UPacked);
			dst3.write(VPacked);
		}
	}
}

//KernRgba2Iyuv
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int ROWS_U,int ROWS_V,int TC,int iTC>
void KernRgba2Iyuv(
		hls::stream < XF_SNAME(WORDWIDTH_SRC) >& rgba,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& y_plane,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& u_plane,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& v_plane,uint16_t height,uint16_t width)
{

	ap_uint8_t Y0[16], U[16], V[16]; uint8_t RGB[64];
#pragma HLS ARRAY_PARTITION variable=Y0  complete
#pragma HLS ARRAY_PARTITION variable=U   complete
#pragma HLS ARRAY_PARTITION variable=V   complete
#pragma HLS ARRAY_PARTITION variable=RGB complete

	XF_SNAME(WORDWIDTH_SRC)  PackedPixels;
	XF_SNAME(WORDWIDTH_DST) YPacked, UPacked, VPacked;

	uint8_t Ycount = 0, UVcount = 0;
	int  offset;
	uchar_t UVoffset_ind,l;
    ap_uint<13> i,j;
	UVoffset_ind = (1<<XF_BITSHIFT(NPC))>>1;

	bool evenRow = true, evenBlock = true;
	rowloop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for( j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			PackedPixels = rgba.read();
			ExtractRGBAPixels<WORDWIDTH_SRC>(PackedPixels, RGB);
			for( l = 0; l < (1<<XF_BITSHIFT(NPC))>>1; l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				offset = l << 3;
				Y0[(l<<1)]   = CalculateY(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
				Y0[(l<<1)+1] = CalculateY(RGB[offset+4], RGB[offset+5], RGB[offset+6]);
				if(evenRow)	//As Sampling rate is 2, Calculating U and V components only for even rows
				{
					/* 128 is added to U and V values to make them always positive and in studio range 16-240 */
					if(evenBlock)
					{
						U[l] = CalculateU(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
						V[l] = CalculateV(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
					}
					else
					{
						U[UVoffset_ind + l] = CalculateU(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
						V[UVoffset_ind + l] = CalculateV(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
					}
				}
			}
			YPacked = PackPixels<WORDWIDTH_DST>(Y0);
			y_plane.write(YPacked);
			if(evenRow & !evenBlock)
			{
				UPacked = PackPixels<WORDWIDTH_DST>(U);
				VPacked = PackPixels<WORDWIDTH_DST>(V);
				u_plane.write(UPacked);
				v_plane.write(VPacked);
			}
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
	if(((ROWS+1)>>1) & 0x1)
	{	// Filling the empty region with zeros, when the height is multiple of 2 but not a multiple of 4
		for( i = 0; i < width; i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			u_plane.write(0);
			v_plane.write(0);
		}
	}
}

//KernRgba2Nv12
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV, int TC, int iTC>
void KernRgba2Nv12(
		hls::stream< XF_SNAME(WORDWIDTH_SRC) >& rgba,
		hls::stream< XF_SNAME(WORDWIDTH_Y) >& y_plane,
		hls::stream< XF_SNAME(WORDWIDTH_UV) >& uv_plane,uint16_t height,uint16_t width)
{
	//width=width>>NPC;
	XF_PTNAME(XF_8UP) Y0[16], UV[16];
	uint8_t RGB[64];
#pragma HLS ARRAY_PARTITION variable=Y0  complete
#pragma HLS ARRAY_PARTITION variable=UV  complete
#pragma HLS ARRAY_PARTITION variable=RGB complete
	XF_SNAME(WORDWIDTH_SRC)  PackedPixels;
	XF_SNAME(WORDWIDTH_Y) YPacked, UVPacked;
	uint8_t offset;
	bool evenRow = true;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			PackedPixels = rgba.read();
			ExtractRGBAPixels<WORDWIDTH_SRC>(PackedPixels, RGB);
			for(int l = 0; l < (1<<XF_BITSHIFT(NPC))>>1; l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				offset = l << 3;
				Y0[(l<<1)]   = CalculateY(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
				Y0[(l<<1)+1] = CalculateY(RGB[offset+4], RGB[offset+5], RGB[offset+6]);
				if(evenRow)
				{
					UV[l<<1]     = CalculateU(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
					UV[(l<<1)+1] = CalculateV(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
				}
			}
			YPacked = PackPixels<WORDWIDTH_Y>(Y0);
			y_plane.write(YPacked);
			if(evenRow)
			{
				UVPacked = PackPixels<WORDWIDTH_Y>(UV);
				uv_plane.write(UVPacked);
			}
		}
		evenRow = evenRow ? false : true;
	}
}

//KernRgba2Nv21
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_Y,int WORDWIDTH_VU, int TC, int iTC>
void KernRgba2Nv21(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& rgba,
		hls::stream<XF_SNAME(WORDWIDTH_Y)>& y_plane,
		hls::stream<XF_SNAME(WORDWIDTH_VU)>& vu_plane,uint16_t height,uint16_t width)
{
	//width=width>>NPC;
	uint16_t i, j, k, l;
	ap_uint8_t Y0[16], VU[16];
	uint8_t RGB[64];
#pragma HLS ARRAY_PARTITION variable=Y0 complete
#pragma HLS ARRAY_PARTITION variable=VU complete
#pragma HLS ARRAY_PARTITION variable=RGB complete
	XF_SNAME(WORDWIDTH_SRC) PackedPixels;
	XF_SNAME(WORDWIDTH_Y) YPacked, VUPacked;
	uint8_t offset;
	bool evenRow = true;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			PackedPixels = (XF_SNAME(WORDWIDTH_SRC))rgba.read();
			ExtractRGBAPixels<WORDWIDTH_SRC>(PackedPixels, RGB);
			for(int l = 0; l < (1<<NPC)>>1; l++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				offset = l << 3;
				Y0[(l<<1)]   = CalculateY(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
				Y0[(l<<1)+1] = CalculateY(RGB[offset+4], RGB[offset+5], RGB[offset+6]);
				if(evenRow)
				{
					VU[(l<<1)]   = CalculateV(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
					VU[(l<<1)+1] = CalculateU(RGB[offset+0], RGB[offset+1], RGB[offset+2]);
				}
			}
			YPacked = PackPixels<WORDWIDTH_Y>(Y0);
			y_plane.write(YPacked);
			if(evenRow)
			{
				VUPacked = PackPixels<WORDWIDTH_Y>(VU);
				vu_plane.write(VUPacked);
			}
		}
		evenRow = evenRow ? false : true;
	}
}



//KernIyuv2Rgba
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernIyuv2Rgba(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& in_y,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& in_u,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& in_v,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& rgba,uint16_t height,uint16_t width)
{
	//width=width>>NPC;
//	ap_uint<13> i,j,k;
//	uchar_t k;
	XF_PTNAME(XF_8UP) RGB[64], Ybuf[16], Ubuf[16], Vbuf[16];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=Ubuf complete
#pragma HLS ARRAY_PARTITION variable=Vbuf complete

	hls::stream<XF_SNAME(WORDWIDTH_SRC)> UStream, VStream;
#pragma HLS STREAM variable=&UStream  depth=COLS
#pragma HLS STREAM variable=&VStream  depth=COLS

	XF_SNAME(WORDWIDTH_SRC)  YPacked, UPacked, VPacked;
	XF_SNAME(WORDWIDTH_DST) PackedPixels;

	uint8_t Y00, Y01;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t U, V;
	uint8_t offset;
	bool evenRow = true, evenBlock = true;

	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YPacked = in_y.read();

			xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(Ybuf, YPacked, 0);
			if(evenBlock)
			{
				if(evenRow)
				{
					UPacked = in_u.read();
					UStream.write(UPacked);
					VPacked = in_v.read();
					VStream.write(VPacked);
				}
				else
				{
					/* Copy of the U and V values are pushed into stream to be used for next row */
					UPacked = UStream.read();
					VPacked = VStream.read();
				}
				xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(Ubuf, UPacked, 0);
				xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(Vbuf, VPacked, 0);
				offset = 0;
			}
			else
			{
				offset = (1 << NPC) >> 1;
			}
			for(int k = 0; k < (1<<NPC)>>1; k++)
			{	// Y00 and Y01 have a U and V values in common
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				//Y00 = (Ybuf[k<<1] > 16) ? (Ybuf[k<<1]-16) : 0;
				//Y01 = (Ybuf[(k<<1) + 1] > 16) ? (Ybuf[(k<<1)+1]-16) : 0;

				if((Ybuf[k<<1] > 16))
				{
					Y00 = (Ybuf[k<<1]-16);
				}
				else
				{
					Y00 = 0;
				}

				if((Ybuf[(k<<1) + 1] > 16))
				{
					Y01 = (Ybuf[(k<<1)+1]-16);
				}
				else
				{
					Y01 = 0;
				}

				U = Ubuf[k + offset] - 128;
				V = Vbuf[k + offset] - 128;

				V2Rtemp = V * (short int)V2R;
				U2Gtemp = (short int)U2G * U;
				V2Gtemp = (short int)V2G * V;
				U2Btemp = U * (short int)U2B;

				// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
				// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
				// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
				RGB[(k<<3)]     = CalculateR(Y00,V2Rtemp,V);		//R0
				RGB[(k<<3) + 1] = CalculateG(Y00,U2Gtemp,V2Gtemp);	//G0
				RGB[(k<<3) + 2] = CalculateB(Y00,U2Btemp,U);		//B0
				RGB[(k<<3) + 3] = 255;					//A
				RGB[(k<<3) + 4] = CalculateR(Y01,V2Rtemp,V);		//R1
				RGB[(k<<3) + 5] = CalculateG(Y01,U2Gtemp,V2Gtemp);	//G1
				RGB[(k<<3) + 6] = CalculateB(Y01,U2Btemp,U);		//B1
				RGB[(k<<3) + 7] = 255;					//A
			}
			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			rgba.write(PackedPixels);
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false: true;
	}
}

//KernIyuv2Nv12
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_UV, int rTC, int cTC, int iTC>
void KernIyuv2Nv12(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& in_u,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& in_v,
		hls::stream<XF_SNAME(WORDWIDTH_UV)>& out_uv,uint16_t height,uint16_t width)
{
	ap_uint<13> i,j;
	XF_PTNAME(XF_8UP) U[16], V[16];
#pragma HLS ARRAY_PARTITION variable=U complete
#pragma HLS ARRAY_PARTITION variable=V complete

	XF_SNAME(WORDWIDTH_SRC) UVPacked0, UVPacked1, UPacked, VPacked;
	rowloop:
	for( i = 0; i < height>>1; i++)
	{
		/*
		 * Reading the plane interleaved U and V data from streams and packing them in pixel interleaved
		 * and writing out to UV stream
		 */
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=rTC max=rTC
		columnloop:
		for( j = 0; j < (width>>(1+XF_BITSHIFT(NPC))); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=cTC max=cTC
			UPacked = in_u.read();
			VPacked = in_v.read();

			xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(U, UPacked, 0);
			xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(V, VPacked, 0);
			// Packing with alternative U and V values for Pixel interleaving
#define AU_CVT_STEP 16
			ap_uint<4> off = (1 << XF_BITSHIFT(NPC)) >> 1;
			ap_uint<4> k;
			int l;
			for( k = 0,  l = 0; k < ((1<<XF_BITSHIFT(NPC))>>1); k++, l+=AU_CVT_STEP)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS UNROLL
				UVPacked0.range(l+AU_CVT_STEP-1, l) = (U[k]) | ((ap_uint<16>)V[k] << (8));
				UVPacked1.range(l+AU_CVT_STEP-1, l) = (U[k+off]) | ((ap_uint<16>)V[k+off] << (8));
			}
			out_uv.write(UVPacked0);
			out_uv.write(UVPacked1);
		}
	}
}

//KernIyuv2Yuv4
template<int ROWS, int COLS, int NPC, int WORDWIDTH, int rTC, int cTC, int iTC>
void KernIyuv2Yuv4(
		hls::stream<XF_SNAME(WORDWIDTH)>& in_u,
		hls::stream<XF_SNAME(WORDWIDTH)>& in_v,
		hls::stream<XF_SNAME(WORDWIDTH)>& out_u,
		hls::stream<XF_SNAME(WORDWIDTH)>& out_v,uint16_t height,uint16_t width)
{

	XF_PTNAME(XF_8UP) U[16], V[16];
#pragma HLS ARRAY_PARTITION variable=U complete
#pragma HLS ARRAY_PARTITION variable=V complete

	XF_SNAME(WORDWIDTH) IUPacked, IVPacked, UPacked0,
			VPacked0, UPacked1, VPacked1;
	rowloop:
	for(int i = 0; i < ((height>>2)<<1); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=rTC max=rTC
		columnloop:
		for(int j = 0; j < ((width>>XF_BITSHIFT(NPC))>>1); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=cTC max=cTC
			IUPacked = in_u.read();
			IVPacked = in_v.read();

			xfExtractPixels<NPC, WORDWIDTH, XF_8UP>(U, IUPacked, 0);
			xfExtractPixels<NPC, WORDWIDTH, XF_8UP>(V, IVPacked, 0);
#define AU_CVT_STEP 16
			int off = 1 << (2); // (1 << NPC) >> 1;
			for(int k = 0, l = 0; k <(1<<(2)); k++, l += AU_CVT_STEP)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS UNROLL
				UPacked0.range(l+AU_CVT_STEP-1, l) = (U[k]) | ((ap_uint<16>)U[k] << (8));
				VPacked0.range(l+AU_CVT_STEP-1, l) = (V[k]) | ((ap_uint<16>)V[k] << (8));
				UPacked1.range(l+AU_CVT_STEP-1, l) = (U[k+off]) | ((ap_uint<16>)U[k+off] << (8));
				VPacked1.range(l+AU_CVT_STEP-1, l) = (V[k+off]) | ((ap_uint<16>)V[k+off] << (8));
			}
			out_u.write(UPacked0);
			out_v.write(VPacked0);
			out_u.write(UPacked1);
			out_v.write(VPacked1);
		}
	}
}

//KernNv122Iyuv
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernNv122Iyuv(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& in_uv,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& u_out,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& v_out,uint16_t height,uint16_t width)
{
	XF_PTNAME(XF_8UP) UV0[16], UV1[16];
#pragma HLS ARRAY_PARTITION variable=UV0 complete
#pragma HLS ARRAY_PARTITION variable=UV1 complete

	XF_SNAME(WORDWIDTH_DST) UPacked, VPacked;
	XF_SNAME(WORDWIDTH_SRC) UVPacked0, UVPacked1;
ap_uint<13> i,j;
	rowloop:
	for( i = 0; i < (height>>1); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for( j = 0; j < ((width>>XF_BITSHIFT(NPC))>>1); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			UVPacked0 = in_uv.read();
			UVPacked1 = in_uv.read();

			xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(UV0, UVPacked0, 0);
			xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(UV1, UVPacked1, 0);
			// Packing the U and V by picking even indeces for U and odd indeces for V
#define AU_CVT_STEP 16
			int sft = 1 << (XF_BITSHIFT(NPC)+2);
			int l;
			ap_uint<9> k;
			for(int k = 0, l = 0; k < (1<<(XF_BITSHIFT(NPC))); k+=4, l+=AU_CVT_STEP)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS UNROLL
				VPacked.range(l+AU_CVT_STEP-1, l) = (UV0[k+1]) | ((ap_uint<16>)UV0[k+3] << (8));
				UPacked.range(l+AU_CVT_STEP-1, l) =  (UV0[k])  | ((ap_uint<16>)UV0[k+2] << (8));

				VPacked.range(l+sft+AU_CVT_STEP-1, l+sft) = (UV1[k+1]) | ((ap_uint<16>)UV1[k+3] << (8));
				UPacked.range(l+sft+AU_CVT_STEP-1, l+sft) = (UV1[k])   | ((ap_uint<16>)UV1[k+2] << (8));
			}
			u_out.write(UPacked);
			v_out.write(VPacked);
		}
	}
}

//KernNv122Rgba
template<int ROWS, int COLS, int NPC, int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST, int TC, int iTC>
void KernNv122Rgba(
		hls::stream<XF_SNAME(WORDWIDTH_Y)>& in_y,
		hls::stream<XF_SNAME(WORDWIDTH_UV)>& in_uv,
		hls::stream< XF_SNAME(WORDWIDTH_DST) >& rgba,uint16_t height,uint16_t width)
{
	//width=width>>NPC;
	XF_PTNAME(XF_8UP) RGB[64], Ybuf[16], UVbuf[16];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=UVbuf complete

	hls::stream<XF_SNAME(WORDWIDTH_UV)> UVStream;
#pragma HLS STREAM variable=&UVStream  depth=COLS
	XF_SNAME(WORDWIDTH_Y) YPacked; XF_SNAME(WORDWIDTH_UV) UVPacked;
	XF_SNAME(WORDWIDTH_DST) PackedPixels;
	uint8_t Y00, Y01;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t U, V;
	bool evenRow = true;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS

		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC

			YPacked = in_y.read();
			xfExtractPixels<NPC, WORDWIDTH_Y, XF_8UP>(Ybuf, YPacked, 0);
			if(evenRow)
			{
				UVPacked = in_uv.read();
				UVStream.write(UVPacked);
			}
			else // Keep a copy of UV row data in stream to use for oddrow
				UVPacked = UVStream.read();

			xfExtractPixels<NPC, WORDWIDTH_UV, XF_8UP>(UVbuf, UVPacked, 0);
			for(int k = 0; k < (1<<NPC)>>1; k++)
			{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				//Y00 = (Ybuf[k<<1] > 16) ? (Ybuf[k<<1]-16) : 0;
				//Y01 = (Ybuf[(k<<1)+1] > 16) ? (Ybuf[(k<<1)+1] - 16) : 0;

				if((Ybuf[k<<1] > 16))
				{
					Y00 = (Ybuf[k<<1]-16);
				}
				else
				{
					Y00 = 0;
				}

				if((Ybuf[(k<<1)+1] > 16))
				{
					Y01 = (Ybuf[(k<<1)+1] - 16);
				}
				else
				{
					Y01 = 0;
				}

				U = UVbuf[k<<1] - 128;
				V = UVbuf[(k<<1)+1] - 128;

				V2Rtemp = V * (short int)V2R;
				U2Gtemp = (short int)U2G * U;
				V2Gtemp = (short int)V2G * V;
				U2Btemp = U * (short int)U2B;

				// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
				// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
				// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
				RGB[(k<<3) + 0] = CalculateR(Y00, V2Rtemp, V);	     //R0
				RGB[(k<<3) + 1] = CalculateG(Y00, U2Gtemp, V2Gtemp); //G0
				RGB[(k<<3) + 2] = CalculateB(Y00, U2Btemp, U);	     //B0
				RGB[(k<<3) + 3] = 255;				     //A
				RGB[(k<<3) + 4] = CalculateR(Y01, V2Rtemp, V);	     //R1
				RGB[(k<<3) + 5] = CalculateG(Y01, U2Gtemp, V2Gtemp); //G1
				RGB[(k<<3) + 6] = CalculateB(Y01, U2Btemp, U);	     //B0
				RGB[(k<<3) + 7] = 255;				     //A
			}
			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			rgba.write(PackedPixels);
		}
		evenRow = evenRow ? false : true;
	}
	if(height & 1)
	{
		for(int i = 0; i < (width>>NPC); i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			UVStream.read();
		}
	}
}

//KernNv122Yuv4
template<int ROWS, int COLS, int NPC, int WORDWIDTH_UV, int WORDWIDTH_DST, int TC, int iTC>
void KernNv122Yuv4(
		hls::stream<XF_SNAME(WORDWIDTH_UV)>& in_uv,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& u_out,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& v_out,uint16_t height,uint16_t width)
{
	XF_PTNAME(XF_8UP) UV[16];
#pragma HLS ARRAY_PARTITION variable=UV complete
ap_uint<13> i,j;
	XF_SNAME(WORDWIDTH_UV) UPacked;
	XF_SNAME(WORDWIDTH_DST) VPacked, UVPacked;
	rowloop:
	for( i = 0; i < (height>>1); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for( j = 0; j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			UVPacked = in_uv.read();
			xfExtractPixels<NPC, WORDWIDTH_DST, XF_8UP>(UV, UVPacked, 0);
#define AU_CVT_STEP 16
			for(int k = 0, l = 0; k < (1<<(XF_BITSHIFT(NPC))); k+=2, l+=AU_CVT_STEP)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS UNROLL
				VPacked.range(l+AU_CVT_STEP-1, l) = (UV[k+1]) | ((ap_uint<16>)UV[k+1] << (8));
				UPacked.range(l+AU_CVT_STEP-1, l) = (UV[k])   | ((ap_uint<16>)UV[k] << (8));
			}
			u_out.write(UPacked);
			v_out.write(VPacked);
		}
	}
}

//KernNv212Iyuv
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernNv212Iyuv(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& in_uv,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& u_out,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& v_out,uint16_t height,uint16_t width)
{
	XF_PTNAME(XF_8UP) VU0[16], VU1[16];
#pragma HLS ARRAY_PARTITION variable=VU0 complete
#pragma HLS ARRAY_PARTITION variable=VU1 complete
ap_uint<13> i,j;
	XF_SNAME(WORDWIDTH_DST) UPacked, VPacked;
	XF_SNAME(WORDWIDTH_SRC) VUPacked0, VUPacked1;
int l;
ap_uint<4> k;
	rowloop:
	for( i = 0; i < (height>>1); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for( j = 0; j < ((width>>XF_BITSHIFT(NPC))>>1); j++)
		{	// reading UV pixel interleaved data and writing them into UStream and VStream
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			VUPacked0 = in_uv.read();
			VUPacked1 = in_uv.read();

			xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(VU0, VUPacked0, 0);
			xfExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(VU1, VUPacked1, 0);

#define AU_CVT_STEP 16
			int sft = 1 << (XF_BITSHIFT(NPC)+2);
			for( k = 0, l = 0; k < (1<<(XF_BITSHIFT(NPC))); k+=4, l+=AU_CVT_STEP)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS UNROLL
				UPacked.range(l+AU_CVT_STEP-1, l) = (VU0[k+1]) | ((ap_uint<16>)VU0[k+3] << (8));
				VPacked.range(l+AU_CVT_STEP-1, l) = (VU0[k])   | ((ap_uint<16>)VU0[k+2] << (8));

				UPacked.range(l+sft+AU_CVT_STEP-1, l+sft) = (VU1[k+1]) | ((ap_uint<16>)VU1[k+3] << (8));
				VPacked.range(l+sft+AU_CVT_STEP-1, l+sft) = (VU1[k])   | ((ap_uint<16>)VU1[k+2] << (8));
			}
			u_out.write(UPacked);
			v_out.write(VPacked);
		}
	}
	if((height>>1)& 0x1)
	{
		// Writing 0's to fill the stream if the UV plane width is odd
		for(int i = 0; i < ((width>>XF_BITSHIFT(NPC))>>1); i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			u_out.write(0);
			v_out.write(0);
		}
	}
}

////KernNv212Rgba
template<int ROWS, int COLS, int NPC, int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST, int TC, int iTC>
void KernNv212Rgba(
		hls::stream<XF_SNAME(WORDWIDTH_Y)>& in_y,
		hls::stream<XF_SNAME(WORDWIDTH_UV)>& in_uv,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& rgba,uint16_t height,uint16_t width)
{
	XF_PTNAME(XF_8UP) RGB[64],Ybuf[16],UVbuf[16];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=UVbuf complete
ap_uint<13> i,j;
int k;
	hls::stream<XF_SNAME(WORDWIDTH_UV)> UVStream;
#pragma HLS STREAM variable=&UVStream  depth=COLS
	XF_SNAME(WORDWIDTH_Y) YPacked; XF_SNAME(WORDWIDTH_UV) UVPacked;
	XF_SNAME(WORDWIDTH_DST) PackedPixels;
	uint8_t Y00, Y01;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t U, V;
	bool evenRow = true;
	rowloop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for( j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YPacked = in_y.read();
			xfExtractPixels<NPC, WORDWIDTH_Y, XF_8UP>(Ybuf, YPacked, 0);
			if(evenRow)
			{
				UVPacked = in_uv.read();
				UVStream.write(UVPacked);
			}
			else // Keep a copy of UV row data in stream to use for oddrow
				UVPacked = UVStream.read();

			xfExtractPixels<NPC, WORDWIDTH_UV, XF_8UP>(UVbuf, UVPacked, 0);
			for( k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				//Y00 = (Ybuf[k<<1] > 16) ? (Ybuf[k<<1]-16) : 0;
				//Y01 = (Ybuf[(k<<1)+1] > 16) ? (Ybuf[(k<<1)+1]-16) : 0;

				if((Ybuf[k<<1] > 16))
				{
					Y00 = (Ybuf[k<<1]-16);
				}
				else
				{
					Y00 = 0;
				}

				if((Ybuf[(k<<1)+1] > 16))
				{
					Y01 = (Ybuf[(k<<1)+1]-16);
				}
				else
				{
					Y01 = 0;
				}

				V = UVbuf[k<<1] - 128;
				U = UVbuf[(k<<1)+1] - 128;

				V2Rtemp = V * (short int)V2R;
				U2Gtemp = (short int)U2G * U;
				V2Gtemp = (short int)V2G * V;
				U2Btemp = U * (short int)U2B;

				// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
				// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
				// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
				RGB[(k<<3) + 0] = CalculateR(Y00,V2Rtemp,V);		//R0
				RGB[(k<<3) + 1] = CalculateG(Y00,U2Gtemp,V2Gtemp);	//G0
				RGB[(k<<3) + 2] = CalculateB(Y00,U2Btemp,U);		//B0
				RGB[(k<<3) + 3] = 255;					//A
				RGB[(k<<3) + 4] = CalculateR(Y01,V2Rtemp,V);		//R1
				RGB[(k<<3) + 5] = CalculateG(Y01,U2Gtemp,V2Gtemp);	//G1
				RGB[(k<<3) + 6] = CalculateB(Y01,U2Btemp,U);		//B0
				RGB[(k<<3) + 7] = 255;					//A
			}
			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			rgba.write(PackedPixels);
		}
		evenRow = evenRow ? false : true;
	}
	if(height & 1)
	{
		for( i = 0; i < (width>>XF_BITSHIFT(NPC)); i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			UVStream.read();
		}
	}
}

//KernNv212Yuv4
template<int ROWS, int COLS, int NPC, int WORDWIDTH_VU, int WORDWIDTH_DST, int TC, int iTC>
void KernNv212Yuv4(
		hls::stream<XF_SNAME(WORDWIDTH_VU)>& in_vu,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& u_out,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& v_out,uint16_t height,uint16_t width)
{

	XF_PTNAME(XF_8UP) VUbuf[16];
#pragma HLS ARRAY_PARTITION variable=VUbuf complete
	XF_SNAME(WORDWIDTH_DST) UPacked,VPacked; XF_SNAME(WORDWIDTH_VU) VUPacked;
	ap_uint<13> i,j;
	ap_uint<4> k;
	int l;
	rowloop:
	for( i = 0; i < (height>>1); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for( j = 0;j < (width>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			VUPacked = in_vu.read();
			xfExtractPixels<NPC, WORDWIDTH_VU, XF_8UP>(VUbuf, VUPacked, 0);
#define AU_CVT_STEP 16
			for( k = 0, l = 0; k < (1<<(XF_BITSHIFT(NPC))); k+=2, l+=AU_CVT_STEP)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS UNROLL
				UPacked.range(l+AU_CVT_STEP-1, l) = (VUbuf[k+1]) | ((ap_uint<16>)VUbuf[k+1] << (8));
				VPacked.range(l+AU_CVT_STEP-1, l) = (VUbuf[k]) | ((ap_uint<16>)VUbuf[k] << (8));
			}
			u_out.write(UPacked);
			v_out.write(VPacked);
		}
	}
}

//KernYuyv2Rgba
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernYuyv2Rgba(		hls::stream < XF_SNAME(WORDWIDTH_SRC) >& yuyv,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& rgba, uint16_t height, uint16_t width)
{
	ap_uint8_t RGB[64];
	XF_PTNAME(XF_8UP) YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete

	XF_SNAME(WORDWIDTH_DST) PackedPixels;
	XF_SNAME(WORDWIDTH_SRC)  YUVPacked;
	uint8_t Y00, Y01;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t U, V;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YUVPacked = yuyv.read();
			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked, YUVbuf);
			for(int k = 0; k < (1<<(2)); k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
				//Y00 = (YUVbuf[(k<<2)] > 16) ? (YUVbuf[(k<<2)]-16) : 0;
				if(YUVbuf[(k<<2)] > 16)
				{
					Y00 = (YUVbuf[(k<<2)]-16);
				}
				else
				{
					Y00 = 0;
				}
				U =  YUVbuf[(k<<2)+1] - 128;

				//Y01 = (YUVbuf[(k<<2)+2] > 16) ? (YUVbuf[(k<<2)+2]-16) : 0;
				if(YUVbuf[(k<<2)+2] > 16)
				{
					Y01 = YUVbuf[(k<<2)+2]-16;
				}
				else
				{
					Y01 = 0;
				}
				V =  YUVbuf[(k<<2)+3] - 128;

				V2Rtemp = V * (short int)V2R;
				U2Gtemp = (short int)U2G * U;
				V2Gtemp = (short int)V2G * V;
				U2Btemp = U * (short int)U2B;

				RGB[(k<<3)]   = CalculateR(Y00,V2Rtemp,V);		 //R0
				RGB[(k<<3)+1] = CalculateG(Y00,U2Gtemp,V2Gtemp); //G0
				RGB[(k<<3)+2] = CalculateB(Y00,U2Btemp,U);		 //B0
				RGB[(k<<3)+3] = 255;							 //A
				RGB[(k<<3)+4] = CalculateR(Y01,V2Rtemp,V);		 //R1
				RGB[(k<<3)+5] = CalculateG(Y01,U2Gtemp,V2Gtemp); //G1
				RGB[(k<<3)+6] = CalculateB(Y01,U2Btemp,U);		 //B0
				RGB[(k<<3)+7] = 255;							 //A
			}
			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			rgba.write(PackedPixels);
		}
	}
}


////KernYuyvNv12
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV, int TC, int iTC>
void KernYuyv2Nv12(hls::stream < XF_SNAME(WORDWIDTH_SRC) >& yuyv,
		hls::stream < XF_SNAME(WORDWIDTH_Y) >& y_plane,
		hls::stream < XF_SNAME(WORDWIDTH_UV) >& uv_plane, uint16_t height, uint16_t width)
{
	XF_PTNAME(XF_8UP) Ybuf[16], UVbuf[16], YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=UVbuf complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete
	XF_SNAME(WORDWIDTH_SRC) YUVPacked;
	XF_SNAME(WORDWIDTH_Y)  YPacked, UVPacked;

	bool evenRow = true;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YUVPacked = yuyv.read();
			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked, YUVbuf);

			for(int k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{	// filling the Ybuf and UVbuf in the format required for NV12
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				Ybuf[(k<<1)]   = YUVbuf[(k<<2)];
				Ybuf[(k<<1)+1] = YUVbuf[(k<<2)+2];
				if(evenRow)
				{
					UVbuf[(k<<1)] = YUVbuf[(k<<2)+1];
					UVbuf[(k<<1)+1] = YUVbuf[(k<<2)+3];
				}
			}
			YPacked = PackPixels<WORDWIDTH_Y>(Ybuf);
			y_plane.write(YPacked);
			if(evenRow)
			{
				UVPacked = PackPixels<WORDWIDTH_UV>(UVbuf);
				uv_plane.write(UVPacked);
			}
		}
		evenRow = evenRow ? false : true;
	}
}

////KernYuyv2Iyuv
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernYuyv2Iyuv(	hls::stream < XF_SNAME(WORDWIDTH_SRC) >& yuyv,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& y_plane,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& u_plane,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& v_plane, uint16_t height, uint16_t width)
{
	uint16_t i, j, k, l;
	ap_uint8_t Ybuf[16], Ubuf[16], Vbuf[16], YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=Ybuf   complete
#pragma HLS ARRAY_PARTITION variable=Ubuf   complete
#pragma HLS ARRAY_PARTITION variable=Vbuf   complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete

	XF_SNAME(WORDWIDTH_SRC)  YUVPacked;
	XF_SNAME(WORDWIDTH_DST) YPacked0, UPacked, VPacked;
	uint8_t offset;
	bool evenRow = true, evenBlock = true;
	offset = (1 << XF_BITSHIFT(NPC)) >> 1;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YUVPacked = yuyv.read();
			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked, YUVbuf);
			for(int k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				Ybuf[(k<<1)]   = YUVbuf[(k<<2)];
				Ybuf[(k<<1)+1] = YUVbuf[(k<<2)+2];
				if(evenRow)
				{
					if(evenBlock)
					{
						Ubuf[k] = YUVbuf[(k<<2)+1];
						Vbuf[k] = YUVbuf[(k<<2)+3];
					}
					else
					{
						Ubuf[k+offset] = YUVbuf[(k<<2)+1];
						Vbuf[k+offset] = YUVbuf[(k<<2)+3];
					}
				}
			}
			YPacked0 = PackPixels<WORDWIDTH_DST>(Ybuf);
			y_plane.write(YPacked0);
			if(evenRow & !evenBlock)
			{
				UPacked = PackPixels<WORDWIDTH_DST>(Ubuf);
				VPacked = PackPixels<WORDWIDTH_DST>(Vbuf);
				u_plane.write(UPacked);
				v_plane.write(VPacked);
			}
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
}

//KernUyvy2Iyuv
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernUyvy2Iyuv(		hls::stream < XF_SNAME(WORDWIDTH_SRC) >& _uyvy,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& _y,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& _u,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& _v, uint16_t height, uint16_t width)
{

	ap_uint8_t Ybuf[16], Ubuf[16], Vbuf[16], YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=Ubuf complete
#pragma HLS ARRAY_PARTITION variable=Vbuf complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete

	XF_SNAME(WORDWIDTH_SRC)  YUVPacked;
	XF_SNAME(WORDWIDTH_DST) YPacked0, UPacked, VPacked;
	uint8_t offset;
	bool evenRow = true, evenBlock = true;

	offset = (1<<XF_BITSHIFT(NPC))>>1;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YUVPacked = _uyvy.read();

			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked, YUVbuf);
			for(int k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				Ybuf[(k<<1)]   = YUVbuf[(k<<2) + 1];
				Ybuf[(k<<1)+1] = YUVbuf[(k<<2) + 3];
				if(evenRow)
				{
					if(evenBlock)
					{
						Ubuf[k] = YUVbuf[(k<<2)];
						Vbuf[k] = YUVbuf[(k<<2)+2];
					}
					else
					{
						Ubuf[k + offset] = YUVbuf[(k<<2)];
						Vbuf[k + offset] = YUVbuf[(k<<2)+2];
					}
				}
			}
			YPacked0 = PackPixels<WORDWIDTH_DST>(Ybuf);
			_y.write(YPacked0);
			if(evenRow & !evenBlock)
			{
				UPacked = PackPixels<WORDWIDTH_DST>(Ubuf);
				VPacked = PackPixels<WORDWIDTH_DST>(Vbuf);
				_u.write(UPacked);
				_v.write(VPacked);
			}
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
}

////KernUyvy2Nv12
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV, int TC, int iTC>
void KernUyvy2Nv12(		hls::stream < XF_SNAME(WORDWIDTH_SRC) >& _uyvy,
		hls::stream < XF_SNAME(WORDWIDTH_Y) >& _y,
		hls::stream < XF_SNAME(WORDWIDTH_UV) >& _uv, uint16_t height, uint16_t width)
{
	ap_uint8_t Ybuf[16], UVbuf[16], YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=Ybuf complete
#pragma HLS ARRAY_PARTITION variable=UVbuf complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete
	XF_SNAME(WORDWIDTH_SRC)  YUVPacked;
	XF_SNAME(WORDWIDTH_Y) YPacked, UVPacked;
	bool evenRow = true;
	rowloop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		columnloop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			YUVPacked = _uyvy.read();
			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked,YUVbuf);
			// filling the Ybuf and UVbuf in the format required for NV12
			for(int k = 0; k < (1<<XF_BITSHIFT(NPC))>>1; k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				Ybuf[(k<<1)]   = YUVbuf[(k<<2)+1];
				Ybuf[(k<<1)+1] = YUVbuf[(k<<2)+3];
				if(evenRow)
				{
					UVbuf[(k<<1)] = YUVbuf[(k<<2)];
					UVbuf[(k<<1)+1] = YUVbuf[(k<<2)+2];
				}
			}
			YPacked = PackPixels<WORDWIDTH_Y>(Ybuf);
			_y.write(YPacked);
			if(evenRow)
			{
				UVPacked = PackPixels<WORDWIDTH_Y>(UVbuf);
				_uv.write(UVPacked);
			}
		}
		evenRow = evenRow ? false : true;
	}
}

////KernUyvy2Rgba
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC, int iTC>
void KernUyvy2Rgba(
		hls::stream < XF_SNAME(WORDWIDTH_SRC) > & uyvy,

		hls::stream < XF_SNAME(WORDWIDTH_DST) >& rgba, uint16_t height,uint16_t width)
{
	uint16_t i,j,k;
	XF_PTNAME(XF_8UP) RGB[64],YUVbuf[32];
#pragma HLS ARRAY_PARTITION variable=RGB complete
#pragma HLS ARRAY_PARTITION variable=YUVbuf complete

	XF_SNAME(WORDWIDTH_DST) PackedPixels;
	XF_SNAME(WORDWIDTH_SRC) YUVPacked;
	uint8_t Y00, Y01;
	int32_t V2Rtemp,U2Gtemp,V2Gtemp,U2Btemp;
	int8_t U,V;

	rowloop:
	for(i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		columnloop:
		for(j = 0;j < width;j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
			YUVPacked = uyvy.read();
			ExtractUYVYPixels<WORDWIDTH_SRC>(YUVPacked,YUVbuf);
			for(k = 0; k < (1<<NPC)>>1; k++)
			{
#pragma HLS LOOP_TRIPCOUNT min=iTC max=iTC
#pragma HLS unroll
				U = YUVbuf[(k<<2)] - 128;
				//Y00 = (YUVbuf[(k<<2) + 1] > 16) ? (YUVbuf[(k<<2) + 1] - 16):0;
				if(YUVbuf[(k<<2) + 1] > 16)
				{
					Y00 = (YUVbuf[(k<<2) + 1] - 16);
				}
				else
				{
					Y00 = 0;
				}
				V = YUVbuf[(k<<2) + 2] - 128;
				//Y01 = (YUVbuf[(k<<2) + 3] > 16) ? (YUVbuf[(k<<2) + 3] - 16):0;
				if((YUVbuf[(k<<2) + 3] > 16))
				{
					Y01 = (YUVbuf[(k<<2) + 3] - 16);
				}
				else
				{
					Y01 = 0;
				}

				V2Rtemp = V * (short int)V2R;
				U2Gtemp = (short int)U2G * U;
				V2Gtemp = (short int)V2G * V;
				U2Btemp = U * (short int)U2B;

				RGB[(k<<3)] = CalculateR(Y00,V2Rtemp,V);		//R0
				RGB[(k<<3) + 1] = CalculateG(Y00,U2Gtemp,V2Gtemp);	//G0
				RGB[(k<<3) + 2] = CalculateB(Y00,U2Btemp,U);		//B0
				RGB[(k<<3) + 3] = 255;					//A
				RGB[(k<<3) + 4] = CalculateR(Y01,V2Rtemp,V);		//R1
				RGB[(k<<3) + 5] = CalculateG(Y01,U2Gtemp,V2Gtemp);	//G1
				RGB[(k<<3) + 6] = CalculateB(Y01,U2Btemp,U);		//B0
				RGB[(k<<3) + 7] = 255;					//A
			}
			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			rgba.write(PackedPixels);
		}
	}
}

/********************************************************************************
 * Color Conversion APIs
 *******************************************************************************/

template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFRgba2Yuv4(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& rgba,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& y_plane,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& u_plane,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& v_plane,uint16_t height,uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
//	assert(( (rgba.cols == y_plane.cols) && (rgba.rows == y_plane.rows))
//			&& "RGBA and Y plane dimensions mismatch");
//	assert(( (rgba.cols == u_plane.cols) && (rgba.rows == u_plane.rows))
//			&& "RGBA and U plane dimensions mismatch");
//	assert(( (rgba.cols == v_plane.cols) && (rgba.rows == v_plane.rows))
//			&& "RGBA and V plane dimensions mismatch");


		KernRgba2Yuv4<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST>(rgba, y_plane, u_plane, v_plane,height,width);

}
/*#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_y_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_u_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_v_image.data":AXIDMA_SIMPLE)*/
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
#pragma SDS data copy("_src.data"[0:"_src.size"])
template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC=1>
void rgba2yuv4(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<DST_T, ROWS, COLS, NPC> & _y_image, xf::Mat<DST_T, ROWS, COLS, NPC> & _u_image, xf::Mat<DST_T, ROWS, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<XF_TNAME(SRC_T, NPC)> src;
	hls::stream<XF_TNAME(DST_T, NPC)> y_image;
	hls::stream<XF_TNAME(DST_T, NPC)> u_image;
	hls::stream<XF_TNAME(DST_T, NPC)> v_image;

	for(int i=0; i<_src.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
	#pragma HLS loop_flatten off
			src.write( *(_src.data + i*(_src.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}


	xFRgba2Yuv4< ROWS, COLS, NPC, XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(src, y_image, u_image, v_image, _src.rows,_src.cols);

	for(int i=0; i<_y_image.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_y_image.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
#pragma HLS loop_flatten off
			*(_y_image.data + i*(_y_image.cols>>(XF_BITSHIFT(NPC))) +j) = y_image.read();
			*(_u_image.data + i*(_u_image.cols>>(XF_BITSHIFT(NPC))) +j) = u_image.read();
			*(_v_image.data + i*(_v_image.cols>>(XF_BITSHIFT(NPC))) +j) = v_image.read();
		}
	}


}
////auRgba2Yuv4
//
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int ROWS_U,int ROWS_V>
void xFRgba2Iyuv(
		hls::stream < XF_SNAME(WORDWIDTH_SRC) >& rgba,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& y_plane,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& u_plane,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& v_plane,uint16_t height,uint16_t width)
{

//	assert(( (rgba.cols == y_plane.cols) && (rgba.rows == y_plane.rows))
//			&& "RGBA and Y plane dimensions mismatch");
//	assert(( (rgba.cols == u_plane.cols) && (rgba.rows == (u_plane.rows<<2)))
//			&& "RGBA and U plane dimensions mismatch");
//	assert(( (rgba.cols == v_plane.cols) && (rgba.rows == (v_plane.rows<<2)))
//			&& "RGBA and V plane dimensions mismatch");

	if(NPC == XF_NPPC8)
	{

		KernRgba2Iyuv<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,ROWS_U,ROWS_V,(COLS>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(rgba, y_plane, u_plane, v_plane,height,width);

	}
	else
	{
		KernRgba2Iyuv<ROWS, COLS,  NPC, WORDWIDTH_SRC,  WORDWIDTH_DST, ROWS_U,ROWS_V>(rgba, y_plane, u_plane, v_plane,height,width);

	}
}

/*#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_y_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_u_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_v_image.data":AXIDMA_SIMPLE)*/
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
template <int SRC_T, int DST_T, int ROWS, int COLS, int NPC=0>
void rgba2iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<DST_T, ROWS, COLS, NPC> & _y_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _u_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<XF_TNAME(SRC_T, NPC)> src;
	hls::stream<XF_TNAME(DST_T, NPC)> y;
	hls::stream<XF_TNAME(DST_T, NPC)> u;
	hls::stream<XF_TNAME(DST_T, NPC)> v;

	for(int i=0; i<_src.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
	#pragma HLS loop_flatten off
			src.write( *(_src.data + i*(_src.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFRgba2Iyuv< ROWS, COLS, NPC, XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),  ROWS/4, ROWS/4>(src, y, u, v, _src.rows,_src.cols);

	for(int i=0; i<_y_image.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_y_image.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
#pragma HLS loop_flatten off
			*(_y_image.data + i*(_y_image.cols>>(XF_BITSHIFT(NPC))) +j) = y.read();
		}
	}

	for(int i=0; i<_u_image.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_u_image.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
#pragma HLS loop_flatten off
			*(_u_image.data + i*(_u_image.cols>>(XF_BITSHIFT(NPC))) +j) = u.read();
			*(_v_image.data + i*(_u_image.cols>>(XF_BITSHIFT(NPC))) +j) = v.read();
		}
	}



}
//auRgba2Iyuv

template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void xFRgba2Nv12(
		hls::stream < XF_SNAME(WORDWIDTH_SRC) >& rgba,
		hls::stream < XF_SNAME(WORDWIDTH_Y) >& y_plane,
		hls::stream < XF_SNAME(WORDWIDTH_UV) >& uv_plane,
		uint16_t height,uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
//	assert(( (rgba.cols == y_plane.cols) && (rgba.rows == y_plane.rows))
//			&& "RGBA and Y plane dimensions mismatch");
//	assert(( (rgba.cols == uv_plane.cols) && (rgba.rows == (uv_plane.rows<<1)))
//			&& "RGBA and UV plane dimensions mismatch");
	width=width>>XF_BITSHIFT(NPC);

		KernRgba2Nv12<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV>(rgba, y_plane, uv_plane,height,width);

}
////auRgba2Nv12
//
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void xFRgba2Nv21(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& rgba,
		hls::stream<XF_SNAME(WORDWIDTH_Y)>& y_plane,
		hls::stream< XF_SNAME(WORDWIDTH_UV) >& vu_plane,uint16_t height ,uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
//	assert(( (rgba.cols == y_plane.cols) && (rgba.rows == y_plane.rows))
//			&& "RGBA and Y plane dimensions mismatch");
//	assert(( (rgba.cols == vu_plane.cols) && (rgba.rows == (vu_plane.rows<<1)))
//			&& "RGBA and VU plane dimensions mismatch");

	width=width>>XF_BITSHIFT(NPC);

		KernRgba2Nv21<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV>(rgba, y_plane, vu_plane,height,width);


}
/*#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_y.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_uv.data":AXIDMA_SIMPLE)*/
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y.data"[0:"_y.size"])
#pragma SDS data copy("_uv.data"[0:"_uv.size"])
template <int SRC_T, int Y_T, int UV_T, int ROWS, int COLS, int NPC=1>
void rgba2nv21(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC> & _uv)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<XF_TNAME(SRC_T, NPC)> src;
	hls::stream<XF_TNAME(Y_T, NPC)> y;
	hls::stream<XF_TNAME(UV_T, NPC)> uv;

	for(int i=0; i<_src.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src.write( *(_src.data + i*(_src.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFRgba2Nv21<ROWS,COLS,NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(Y_T,NPC),XF_WORDWIDTH(UV_T,NPC)>(src,y,uv,_src.rows,_src.cols);

	for(int i=0; i<_y.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_y.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_y.data + i*(_y.cols>>(XF_BITSHIFT(NPC))) +j) = y.read();

		}
	}

	for(int i=0; i<_uv.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_uv.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_uv.data + i*(_uv.cols>>(XF_BITSHIFT(NPC))) +j) = uv.read();

		}
	}

}
/*#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_y.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_uv.data":AXIDMA_SIMPLE)*/
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y.data"[0:"_y.size"])
#pragma SDS data copy("_uv.data"[0:"_uv.size"])
template <int SRC_T, int Y_T, int UV_T, int ROWS, int COLS, int NPC=1>
void rgba2nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<Y_T, ROWS, COLS, NPC> & _y, xf::Mat<UV_T, ROWS/2, COLS/2, NPC> & _uv)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW

	hls::stream<XF_TNAME(SRC_T, NPC)> src;
	hls::stream<XF_TNAME(Y_T, NPC)> y;
	hls::stream<XF_TNAME(UV_T, NPC)> uv;

	for(int i=0; i<_src.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src.write( *(_src.data + i*(_src.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFRgba2Nv12<ROWS,COLS,NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(Y_T,NPC),XF_WORDWIDTH(UV_T,NPC)>(src,y,uv,_src.rows,_src.cols);

	for(int i=0; i<_y.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_y.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_y.data + i*(_y.cols>>(XF_BITSHIFT(NPC))) +j) = y.read();

		}
	}

	for(int i=0; i<_uv.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_uv.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_uv.data + i*(_uv.cols>>(XF_BITSHIFT(NPC))) +j) = uv.read();

		}
	}

}
//auRgba2Nv21


template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFIyuv2Rgba(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& in_y,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& in_u,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& in_v,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& rgba,uint16_t height,uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
//	assert(( (in_y.cols == (rgba.cols)) && (in_y.rows == rgba.rows))
//			&& "Y plane and RGBA dimensions mismatch");
//	assert(( (in_u.cols == (rgba.cols)) && (in_u.rows == (rgba.rows>>2)))
//			&& "U plane and RGBA dimensions mismatch");
//	assert(( (in_v.cols == (rgba.cols)) && (in_v.rows == (rgba.rows>>2)))
//			&& "V plane and RGBA dimensions mismatch");
	width=width>>XF_BITSHIFT(NPC);
//if((NPC == XF_NPPC8))// && (FLAG == AU_STANDALONE))
//	{
////#pragma HLS INLINE
////		hls::stream< ap_uint<256> > tmp;
////#pragma HLS DATAFLOW
////		KernIyuv2Rgba<ROWS,COLS,NPC,WORDWIDTH_SRC,AU_256UW,(COLS>>NPC),(1<<(NPC+1))>
////		(in_y, in_u, in_v, tmp,height,width);
//
//		Convert256To64<ROWS,COLS,NPC,AU_256UW,WORDWIDTH_DST,(COLS>>NPC)>(tmp, rgba,height,width);
//
//	}
//else
//{
	KernIyuv2Rgba<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(in_y, in_u, in_v, rgba,height,width);

//}

}
/*#pragma SDS data data_mover("src_y.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("src_u.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("src_v.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_dst0.data":AXIDMA_SIMPLE)*/
#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_u.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_v.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst0.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_u.data"[0:"src_u.size"])
#pragma SDS data copy("src_v.data"[0:"src_v.size"])
#pragma SDS data copy("_dst0.data"[0:"_dst0.size"])
template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC=1>
void iyuv2rgba(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y, xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_v,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<XF_TNAME(SRC_T, NPC)> src0;
	hls::stream<XF_TNAME(SRC_T, NPC)> src1;
	hls::stream<XF_TNAME(SRC_T, NPC)> src2;
	hls::stream<XF_TNAME(DST_T, NPC)> dst0;

	Read_Mat_Loop:
	for(int i=0; i<src_y.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_y.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src0.write( *(src_y.data + i*(src_y.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	Read_uv_Loop:
	for(int i=0; i<src_u.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_u.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src1.write( *(src_u.data + i*(src_u.cols>>(XF_BITSHIFT(NPC))) +j) );
			src2.write( *(src_v.data + i*(src_v.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFIyuv2Rgba<ROWS,COLS,NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(src0,src1,src2,dst0,src_y.rows,src_y.cols);

	for(int i=0; i<_dst0.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst0.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_dst0.data + i*(_dst0.cols>>(XF_BITSHIFT(NPC))) +j) = dst0.read();

		}
	}

}
//auIyuv2Rgba

template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_UV>
void xFIyuv2Nv12(hls::stream<XF_SNAME(WORDWIDTH_SRC)>& src0,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& in_u,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>&  in_v,
		hls::stream< XF_SNAME(WORDWIDTH_SRC) >& dst0,
		hls::stream< XF_SNAME(WORDWIDTH_UV) >& out_uv,uint16_t height,uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
//	assert(( (in_u.cols == (out_uv.cols)) && (in_u.rows == (out_uv.rows>>1)))
//			&& "U plane and UV dimensions mismatch");
//	assert(( (in_v.cols == (out_uv.cols)) && (in_v.rows == (out_uv.rows>>1)))
//			&& "V plane and UV dimensions mismatch");

if(NPC==XF_NPPC8)
{
#pragma HLS DATAFLOW
	KernIyuv2Nv12<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_UV,(ROWS>>1),((COLS>>XF_BITSHIFT(NPC))>>1),((1<<XF_BITSHIFT(NPC))>>1)>
	(in_u, in_v, out_uv,height,width);
	write_y<ROWS,COLS,NPC,WORDWIDTH_SRC,(COLS>>XF_BITSHIFT(NPC))>(src0,dst0,height,width);
}
else
{
#pragma HLS DATAFLOW
	KernIyuv2Nv12<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_UV,(ROWS>>1),((COLS>>XF_BITSHIFT(NPC))>>1)>
		(in_u, in_v, out_uv,height,width);

	write_y<ROWS,COLS,NPC,WORDWIDTH_SRC,(COLS>>XF_BITSHIFT(NPC)),(ROWS>>1)>(src0,dst0,height,width);
}
}
//auIyuv2Nv12

template<int ROWS, int COLS, int NPC, int WORDWIDTH>
void xFIyuv2Yuv4(hls::stream<XF_SNAME(WORDWIDTH)>& src0,
		hls::stream<XF_SNAME(WORDWIDTH)>& in_u,
		hls::stream<XF_SNAME(WORDWIDTH)>& in_v,
		hls::stream<XF_SNAME(WORDWIDTH)>& dst0,
		hls::stream<XF_SNAME(WORDWIDTH)>& out_u,
		hls::stream<XF_SNAME(WORDWIDTH)>& out_v,uint16_t height,uint16_t width)
{

//	assert(( (in_u.cols == (out_u.cols)) && (in_u.rows == (out_u.rows>>2)))
//			&& "Input U plane and Output U Plane dimensions mismatch");
//	assert(( (in_v.cols == (out_v.cols)) && (in_v.rows == (out_v.rows>>2)))
//			&& "Input V plane and Output V Plane dimensions mismatch");

	if(NPC == XF_NPPC8)
	{
#pragma HLS DATAFLOW
		KernIyuv2Yuv4<ROWS,COLS,NPC,WORDWIDTH,(ROWS<<1),((COLS>>XF_BITSHIFT(NPC))>>1),((1<<XF_BITSHIFT(NPC))>>1)>(in_u, in_v, out_u, out_v,height,width);
		write_y<ROWS,COLS,NPC,WORDWIDTH,(COLS>>XF_BITSHIFT(NPC))>(src0,dst0,height,width);
	}
	else if (NPC == XF_NPPC1)
	{
#pragma HLS DATAFLOW
		KernIyuv2Yuv4<ROWS,COLS,NPC,WORDWIDTH,(ROWS>>1),((COLS>>XF_BITSHIFT(NPC))>>1)>(in_u, in_v, out_u, out_v,height,width);
		write_y<ROWS,COLS,NPC,WORDWIDTH,(COLS>>XF_BITSHIFT(NPC)),(ROWS>>1)>(src0,dst0,height,width);
	}
}
/*#pragma SDS data data_mover("src_y.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("src_u.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("src_v.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_y_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_u_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_v_image.data":AXIDMA_SIMPLE)*/
#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_u.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_v.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_u.data"[0:"src_u.size"])
#pragma SDS data copy("src_v.data"[0:"src_v.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
template<int SRC_T,int ROWS, int COLS, int NPC=1 >
void iyuv2yuv4(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y, xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_v,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image, xf::Mat<SRC_T, ROWS, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
			hls::stream<XF_TNAME(SRC_T, NPC)> src0;
			hls::stream<XF_TNAME(SRC_T, NPC)> src1;
			hls::stream<XF_TNAME(SRC_T, NPC)> src2;
			hls::stream<XF_TNAME(SRC_T, NPC)> y_image;
			hls::stream<XF_TNAME(SRC_T, NPC)> u_image;
			hls::stream<XF_TNAME(SRC_T, NPC)> v_image;

			Read_Mat_Loop:
			for(int i=0; i<src_y.rows;i++)
			{
			#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
				for(int j=0; j<(src_y.cols)>>(XF_BITSHIFT(NPC));j++)
				{
			#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
					#pragma HLS PIPELINE
					#pragma HLS loop_flatten off
					src0.write( *(src_y.data + i*(src_y.cols>>(XF_BITSHIFT(NPC))) +j) );
				}
			}

			Read_uv_Loop:
			for(int i=0; i<src_u.rows;i++)
			{
			#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
				for(int j=0; j<(src_u.cols)>>(XF_BITSHIFT(NPC));j++)
				{
			#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
					#pragma HLS PIPELINE
					#pragma HLS loop_flatten off
					src1.write( *(src_u.data + i*(src_u.cols>>(XF_BITSHIFT(NPC))) +j) );
					src2.write( *(src_v.data + i*(src_v.cols>>(XF_BITSHIFT(NPC))) +j) );
				}
			}

			xFIyuv2Yuv4<(ROWS>>2),COLS,NPC,XF_WORDWIDTH(SRC_T,NPC)>(src0,src1,src2,y_image,u_image,v_image,src_y.rows,src_y.cols);

			for(int i=0; i<_y_image.rows;i++)
			{
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
				for(int j=0; j<(_y_image.cols)>>(XF_BITSHIFT(NPC));j++)
				{
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
					#pragma HLS PIPELINE
					#pragma HLS loop_flatten off
					*(_y_image.data + i*(_y_image.cols>>(XF_BITSHIFT(NPC))) +j) = y_image.read();

				}
			}

		XF_TNAME(SRC_T, NPC) arr[COLS >>XF_BITSHIFT(NPC)];
		XF_TNAME(SRC_T, NPC) arr1[COLS >>XF_BITSHIFT(NPC)];
	for(int i = 0; i < (_u_image.rows/2); i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j = 0; j < (_u_image.cols>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
#pragma HLS PIPELINE
#pragma HLS loop_flatten off
			XF_TNAME(SRC_T, NPC) pix= u_image.read();
			XF_TNAME(SRC_T, NPC) pix1= v_image.read();
			arr[j]=pix;
			arr1[j]=pix1;
			_u_image.data[((i*2)*(_u_image.cols>>XF_BITSHIFT(NPC)))+j] =pix;
			_v_image.data[((i*2)*(_u_image.cols>>XF_BITSHIFT(NPC)))+j] = pix1;
		}
		for(int j = 0; j < (_u_image.cols>>XF_BITSHIFT(NPC)); j++)
		{
			_u_image.data[(((i*2)+1)*(_u_image.cols>>XF_BITSHIFT(NPC)))+j] =arr[j];
			_v_image.data[(((i*2)+1)*(_u_image.cols>>XF_BITSHIFT(NPC)))+j] =arr1[j];
		}
	}
}

/*#pragma SDS data data_mover("src_y.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("src_u.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("src_v.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_y_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_uv_image.data":AXIDMA_SIMPLE)*/
#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_u.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_v.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv_image.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_u.data"[0:"src_u.size"])
#pragma SDS data copy("src_v.data"[0:"src_v.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_uv_image.data"[0:"_uv_image.size"])
template<int SRC_T,int UV_T,int ROWS, int COLS, int NPC=1 ,int NPC_UV=1>
void iyuv2nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y, xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_u,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & src_v,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image, xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv_image)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<XF_TNAME(SRC_T, NPC)> src0;
	hls::stream<XF_TNAME(SRC_T, NPC)> src1;
	hls::stream<XF_TNAME(SRC_T, NPC)> src2;
	hls::stream<XF_TNAME(SRC_T, NPC)> y_image;
	hls::stream<XF_TNAME(UV_T, NPC_UV)> uv_image;


	Read_Mat_Loop:
	for(int i=0; i<src_y.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_y.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src0.write( *(src_y.data + i*(src_y.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	Read_uv_Loop:
	for(int i=0; i<src_u.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_u.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src1.write( *(src_u.data + i*(src_u.cols>>(XF_BITSHIFT(NPC))) +j) );
			src2.write( *(src_v.data + i*(src_v.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFIyuv2Nv12<ROWS,COLS,NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV)>(src0,src1,src2,y_image,uv_image,src_y.rows,src_y.cols);


	for(int i=0; i<_y_image.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_y_image.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_y_image.data + i*(_y_image.cols>>(XF_BITSHIFT(NPC))) +j) = y_image.read();

		}
	}

	for(int i=0; i<_uv_image.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_uv_image.cols)>>(XF_BITSHIFT(NPC_UV));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_uv_image.data + i*(_uv_image.cols>>(XF_BITSHIFT(NPC_UV))) +j) = uv_image.read();

		}
	}


}
////auIyuv2Yuv4

template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFNv122Iyuv(hls::stream< XF_SNAME(WORDWIDTH_DST) >& src0,
		hls::stream< XF_SNAME(WORDWIDTH_SRC) >& in_uv,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& dst0,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& u_out,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& v_out,uint16_t height,uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
//	assert(( (in_uv.cols == (u_out.cols)) && (in_uv.rows == (u_out.rows<<1)))
//			&& "UV plane and U plane dimensions mismatch");
//	assert(( (in_uv.cols == (v_out.cols)) && (in_uv.rows == (v_out.rows<<1)))
//			&& "UV plane and V plane dimensions mismatch");
	if(NPC == XF_NPPC8)
	{
#pragma HLS DATAFLOW
		KernNv122Iyuv<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>XF_BITSHIFT(NPC))>>1),((1<<XF_BITSHIFT(NPC))>>2)>(in_uv, u_out, v_out,height,width);
		write_y<ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(src0,dst0,height,width);

	}
	else
	{
#pragma HLS DATAFLOW
		KernNv122Iyuv<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>XF_BITSHIFT(NPC))>>1)>(in_uv, u_out, v_out,height,width);
		write_y<ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC)),(ROWS>>1)>(src0,dst0,height,width);
	}
}
//Nv122Iyuv
/*#pragma SDS data data_mover("src_y.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("src_uv.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_y_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_u_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_v_image.data":AXIDMA_SIMPLE)*/
#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
template<int SRC_T,int UV_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv122iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<XF_TNAME(SRC_T, NPC)> src0;
	hls::stream<XF_TNAME(UV_T, NPC_UV)> src1;
	hls::stream<XF_TNAME(SRC_T, NPC)> dst0;
	hls::stream<XF_TNAME(SRC_T, NPC)> dst1;
	hls::stream<XF_TNAME(SRC_T, NPC)> dst2;


	Read_y_Loop:
	for(int i=0; i<src_y.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_y.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src0.write( *(src_y.data + i*(src_y.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	Read_uv_Loop:
	for(int i=0; i<src_uv.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_uv.cols)>>(XF_BITSHIFT(NPC_UV));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src1.write( *(src_uv.data + i*(src_uv.cols>>(XF_BITSHIFT(NPC_UV))) +j) );

		}
	}


		xFNv122Iyuv<(ROWS>>1),COLS,NPC,XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(SRC_T,NPC)>(src0,src1,dst0,dst1,dst2,src_y.rows,src_y.cols);

		for(int i=0; i<_y_image.rows;i++)
		{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
			for(int j=0; j<(_y_image.cols)>>(XF_BITSHIFT(NPC));j++)
			{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
				#pragma HLS PIPELINE
	#pragma HLS loop_flatten off
				*(_y_image.data + i*(_y_image.cols>>(XF_BITSHIFT(NPC))) +j) = dst0.read();
			}
		}

		for(int i=0; i<_u_image.rows;i++)
		{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
			for(int j=0; j<(_u_image.cols)>>(XF_BITSHIFT(NPC));j++)
			{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
				#pragma HLS PIPELINE
	#pragma HLS loop_flatten off
				*(_u_image.data + i*(_u_image.cols>>(XF_BITSHIFT(NPC))) +j) = dst1.read();
				*(_v_image.data + i*(_u_image.cols>>(XF_BITSHIFT(NPC))) +j) = dst2.read();
			}
		}


}
template<int ROWS, int COLS, int NPC, int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST>
void xFNv122Rgba(hls::stream<XF_SNAME(WORDWIDTH_Y)>& in_y,hls::stream< XF_SNAME(WORDWIDTH_UV) >& in_uv,hls::stream< XF_SNAME(WORDWIDTH_DST) >& rgba,uint16_t height,uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
//	assert(( (in_y.cols == (rgba.cols)) && (in_y.rows == rgba.rows))
//			&& "Y plane and RGBA dimensions mismatch");
//	assert(( (in_uv.cols == (rgba.cols)) && (in_uv.rows == (rgba.rows>>1)))
//			&& "UV plane and RGBA dimensions mismatch");
	width=width>>XF_BITSHIFT(NPC);

		KernNv122Rgba<ROWS,COLS,NPC,WORDWIDTH_Y,WORDWIDTH_UV,WORDWIDTH_DST>(in_y, in_uv, rgba,height,width);

}
//Nv122Rgba
/*#pragma SDS data data_mover("src_y.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("src_uv.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_dst1.data":AXIDMA_SIMPLE)*/
#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst0.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst2.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_dst0.data"[0:"_dst0.size"])
template<int SRC_T,int UV_T,int DST_T,int ROWS,int COLS,int NPC=1>
void nv122rgba(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC> & src_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW

	hls::stream<XF_TNAME(SRC_T, NPC)> src0;
	hls::stream<XF_TNAME(UV_T, NPC)> src1;
	hls::stream<XF_TNAME(DST_T, NPC)> dst0;


	Read_y_Loop:
	for(int i=0; i<src_y.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_y.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src0.write( *(src_y.data + i*(src_y.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	Read_uv_Loop:
	for(int i=0; i<src_uv.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_uv.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src1.write( *(src_uv.data + i*(src_uv.cols>>(XF_BITSHIFT(NPC))) +j) );

		}
	}


	xFNv122Rgba<ROWS,COLS,NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(UV_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(src0,src1,dst0,src_y.rows,src_y.cols);

	for(int i=0; i<_dst0.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst0.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_dst0.data + i*(_dst0.cols>>(XF_BITSHIFT(NPC))) +j) = dst0.read();

		}
	}

}
template<int ROWS, int COLS, int NPC, int WORDWIDTH_UV, int WORDWIDTH_DST>
void xFNv122Yuv4(hls::stream<XF_SNAME(WORDWIDTH_DST) >& src0,
		hls::stream<XF_SNAME(WORDWIDTH_UV) >& in_uv,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& dst0,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& u_out,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& v_out,
		uint16_t height,uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
//	assert(( (in_uv.cols == (u_out.cols)) && (in_uv.rows == (u_out.rows>>1)))
//			&& "UV plane and U plane dimensions mismatch");
//	assert(( (in_uv.cols == (v_out.cols)) && (in_uv.rows == (v_out.rows>>1)))
//			&& "UV plane and V plane dimensions mismatch");
	if(NPC == XF_NPPC8 )
	{
#pragma HLS DATAFLOW
		KernNv122Yuv4<ROWS,COLS,NPC,WORDWIDTH_UV,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC)),((1<<(XF_BITSHIFT(NPC)))>>1)>
		(in_uv, u_out, v_out,height,width);
		write_y<ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(src0,dst0,height,width);


	}
	else
	{
#pragma HLS DATAFLOW
		KernNv122Yuv4<ROWS,COLS,NPC,WORDWIDTH_UV,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>
		(in_uv, u_out, v_out,height,width);
		write_y<ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC)),(ROWS>>1)>(src0,dst0,height,width);


	}
}
//auNv122Yuv4

/*#pragma SDS data data_mover("src_y.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("src_uv.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_y_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_u_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_v_image.data":AXIDMA_SIMPLE)*/
#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
template<int SRC_T,int UV_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv122yuv4(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<XF_TNAME(SRC_T, NPC)> src0;
	hls::stream<XF_TNAME(UV_T, NPC_UV)> src1;
	hls::stream<XF_TNAME(SRC_T, NPC)> y_image;
	hls::stream<XF_TNAME(SRC_T, NPC)> u_image;
	hls::stream<XF_TNAME(SRC_T, NPC)> v_image;


	Read_y_Loop:
	for(int i=0; i<src_y.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_y.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src0.write( *(src_y.data + i*(src_y.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	Read_uv_Loop:
	for(int i=0; i<src_uv.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_uv.cols)>>(XF_BITSHIFT(NPC_UV));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src1.write( *(src_uv.data + i*(src_uv.cols>>(XF_BITSHIFT(NPC_UV))) +j) );

		}
	}

	xFNv122Yuv4<(ROWS>>1),COLS,NPC,XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(SRC_T,NPC)>(src0,src1,y_image,u_image,v_image,src_y.rows,src_y.cols);

	write_y_Loop:
	for(int i=0; i<_y_image.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_y_image.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_y_image.data + i*(_y_image.cols>>(XF_BITSHIFT(NPC))) +j) = y_image.read();

		}
	}
	write_uv_Loop:

	XF_TNAME(SRC_T, NPC) arr[COLS >> XF_BITSHIFT(NPC)];
	XF_TNAME(SRC_T, NPC) arr1[COLS >> XF_BITSHIFT(NPC)];

	for(int i = 0; i < (_u_image.rows/2); i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j = 0; j < (_u_image.cols>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
#pragma HLS PIPELINE
#pragma HLS loop_flatten off
			XF_TNAME(SRC_T, NPC) pix= u_image.read();
			XF_TNAME(SRC_T, NPC) pix1= v_image.read();
			arr[j]=pix;
			arr1[j]=pix1;
			_u_image.data[((i*2)*(_u_image.cols>>XF_BITSHIFT(NPC)))+j] =pix;
			_v_image.data[((i*2)*(_u_image.cols>>XF_BITSHIFT(NPC)))+j] = pix1;
		}
		for(int j = 0; j < (_u_image.cols>>XF_BITSHIFT(NPC)); j++)
		{
			_u_image.data[(((i*2)+1)*(_u_image.cols>>XF_BITSHIFT(NPC)))+j] =arr[j];
			_v_image.data[(((i*2)+1)*(_u_image.cols>>XF_BITSHIFT(NPC)))+j] =arr1[j];
		}
	}


}
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFNv212Iyuv(hls::stream< XF_SNAME(WORDWIDTH_DST) >& src0,
		hls::stream< XF_SNAME(WORDWIDTH_SRC) >& in_vu,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& dst0,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& u_out,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& v_out,uint16_t height,uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
//	assert(( (in_vu.cols == (u_out.cols)) && (in_vu.rows == (u_out.rows<<1)))
//			&& "VU plane and U plane dimensions mismatch")
//	assert(( (in_vu.cols == (v_out.cols)) && (in_vu.rows == (v_out.rows<<1)))
//			&& "VU plane and V plane dimensions mismatch");
	if(NPC == XF_NPPC8)
	{
#pragma HLS DATAFLOW
		KernNv212Iyuv<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>XF_BITSHIFT(NPC))>>1),((1<<XF_BITSHIFT(NPC))>>2)>
		(in_vu, u_out, v_out,height,width);
		write_y<ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(src0,dst0,height,width);

	}
	else
	{
#pragma HLS DATAFLOW
		KernNv212Iyuv<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>XF_BITSHIFT(NPC))>>1)>(in_vu, u_out, v_out,height,width);
		write_y<ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC)),(ROWS>>1)>(src0,dst0,height,width);

	}

}



//Nv212Iyuv
/*#pragma SDS data data_mover("src_y.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("src_uv.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_y_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_u_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_v_image.data":AXIDMA_SIMPLE)*/
#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
template<int SRC_T,int UV_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv212iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS/4, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<XF_TNAME(SRC_T, NPC)> src0;
	hls::stream<XF_TNAME(UV_T, NPC_UV)> src1;
	hls::stream<XF_TNAME(SRC_T, NPC)> dst0;
	hls::stream<XF_TNAME(SRC_T, NPC)> dst1;
	hls::stream<XF_TNAME(SRC_T, NPC)> dst2;

	Read_y_Loop:
	for(int i=0; i<src_y.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_y.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src0.write( *(src_y.data + i*(src_y.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	Read_uv_Loop:
	for(int i=0; i<src_uv.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_uv.cols)>>(XF_BITSHIFT(NPC_UV));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src1.write( *(src_uv.data + i*(src_uv.cols>>(XF_BITSHIFT(NPC_UV))) +j) );

		}
	}

		xFNv212Iyuv<(ROWS>>1),COLS,NPC,XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(SRC_T,NPC)>(src0,src1,dst0,dst1,dst2,src_y.rows,src_y.cols);

		for(int i=0; i<_y_image.rows;i++)
		{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
			for(int j=0; j<(_y_image.cols)>>(XF_BITSHIFT(NPC));j++)
			{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
				#pragma HLS PIPELINE
	#pragma HLS loop_flatten off
				*(_y_image.data + i*(_y_image.cols>>(XF_BITSHIFT(NPC))) +j) = dst0.read();
			}
		}

		for(int i=0; i<_u_image.rows;i++)
		{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
			for(int j=0; j<(_u_image.cols)>>(XF_BITSHIFT(NPC));j++)
			{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
				#pragma HLS PIPELINE
	#pragma HLS loop_flatten off
				*(_u_image.data + i*(_u_image.cols>>(XF_BITSHIFT(NPC))) +j) = dst1.read();
				*(_v_image.data + i*(_u_image.cols>>(XF_BITSHIFT(NPC))) +j) = dst2.read();
			}
		}


}
template<int ROWS, int COLS, int NPC, int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST>
void xFNv212Rgba(
		hls::stream<XF_SNAME(WORDWIDTH_Y)>& in_y,
		hls::stream< XF_SNAME(WORDWIDTH_UV) >& in_vu,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& rgba,uint16_t height,uint16_t width)
{

//	assert(( (in_y.cols == (rgba.cols)) && (in_y.rows == rgba.rows))
//			&& "Y plane and RGBA dimensions mismatch");
//	assert(( (in_vu.cols == (rgba.cols)) && (in_vu.rows == (rgba.rows>>1)))
//			&& "VU plane and RGBA dimensions mismatch");

	width=width>>XF_BITSHIFT(NPC);

		KernNv212Rgba<ROWS,COLS,NPC,WORDWIDTH_Y,WORDWIDTH_UV,WORDWIDTH_DST>(in_y, in_vu, rgba,height,width);

}
//Nv212Rgba
/*#pragma SDS data data_mover("src_y.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("src_uv.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_dst1.data":AXIDMA_SIMPLE)*/
#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst0.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst2.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_dst0.data"[0:"_dst0.size"])
template<int SRC_T,int UV_T,int DST_T,int ROWS,int COLS,int NPC=1>
void nv212rgba(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC> & src_uv,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst0)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<XF_TNAME(SRC_T, NPC)> src0;
	hls::stream<XF_TNAME(UV_T, NPC)> src1;
	hls::stream<XF_TNAME(DST_T, NPC)> dst0;


	Read_y_Loop:
	for(int i=0; i<src_y.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_y.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src0.write( *(src_y.data + i*(src_y.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	Read_uv_Loop:
	for(int i=0; i<src_uv.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_uv.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src1.write( *(src_uv.data + i*(src_uv.cols>>(XF_BITSHIFT(NPC))) +j) );

		}
	}


	xFNv212Rgba<ROWS,COLS,NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(UV_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(src0,src1,dst0,src_y.rows,src_y.cols);


	for(int i=0; i<_dst0.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst0.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_dst0.data + i*(_dst0.cols>>(XF_BITSHIFT(NPC))) +j) = dst0.read();

		}
	}


}
template<int ROWS, int COLS, int NPC, int WORDWIDTH_UV, int WORDWIDTH_DST>
void xFNv212Yuv4(hls::stream< XF_SNAME(WORDWIDTH_DST) >& src0,
		hls::stream< XF_SNAME(WORDWIDTH_UV) >& in_vu,
		hls::stream< XF_SNAME(WORDWIDTH_DST) >& dst0,
		hls::stream< XF_SNAME(WORDWIDTH_DST) >& u_out,
		hls::stream< XF_SNAME(WORDWIDTH_DST) >& v_out,uint16_t height,uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
//	assert(( (in_vu.cols == (u_out.cols)) && (in_vu.rows == (u_out.rows>>1)))
//			&& "VU plane and U plane dimensions mismatch");
//	assert(( (in_vu.cols == (v_out.cols)) && (in_vu.rows == (v_out.rows>>1)))
//			&& "VU plane and V plane dimensions mismatch");


	if(NPC == XF_NPPC8)
	{
#pragma HLS DATAFLOW
		KernNv212Yuv4<ROWS,COLS,NPC,WORDWIDTH_UV,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(in_vu, u_out, v_out,height,width);
		write_y<ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(src0,dst0,height,width);

	}
	else
	{
#pragma HLS DATAFLOW
		KernNv212Yuv4<ROWS,COLS,NPC,WORDWIDTH_UV,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(in_vu, u_out, v_out,height,width);
		write_y<ROWS,COLS,NPC,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC)),(ROWS>>1)>(src0,dst0,height,width);
	}

}
//auNv212Yuv4
/*#pragma SDS data data_mover("src_y.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("src_uv.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_y_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_u_image.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover("_v_image.data":AXIDMA_SIMPLE)*/
#pragma SDS data access_pattern("src_y.data":SEQUENTIAL)
#pragma SDS data access_pattern("src_uv.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("src_y.data"[0:"src_y.size"])
#pragma SDS data copy("src_uv.data"[0:"src_uv.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
template<int SRC_T,int UV_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void nv212yuv4(xf::Mat<SRC_T, ROWS, COLS, NPC> & src_y,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & src_uv,xf::Mat<SRC_T, ROWS, COLS, NPC> & _y_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _u_image,xf::Mat<SRC_T, ROWS, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<XF_TNAME(SRC_T, NPC)> src0;
	hls::stream<XF_TNAME(UV_T, NPC_UV)> src1;
	hls::stream<XF_TNAME(SRC_T, NPC)> y_image;
	hls::stream<XF_TNAME(SRC_T, NPC)> u_image;
	hls::stream<XF_TNAME(SRC_T, NPC)> v_image;

	Read_y_Loop:
	for(int i=0; i<src_y.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_y.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src0.write( *(src_y.data + i*(src_y.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	Read_uv_Loop:
	for(int i=0; i<src_uv.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src_uv.cols)>>(XF_BITSHIFT(NPC_UV));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src1.write( *(src_uv.data + i*(src_uv.cols>>(XF_BITSHIFT(NPC_UV))) +j) );

		}
	}

	xFNv212Yuv4<(ROWS>>1),COLS,NPC,XF_WORDWIDTH(UV_T,NPC_UV),XF_WORDWIDTH(SRC_T,NPC)>(src0,src1,y_image,u_image,v_image,src_y.rows,src_y.cols);

	write_y_Loop:
	for(int i=0; i<_y_image.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_y_image.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_y_image.data + i*(_y_image.cols>>(XF_BITSHIFT(NPC))) +j) = y_image.read();

		}
	}

	XF_TNAME(SRC_T, NPC) arr[COLS >>XF_BITSHIFT(NPC)];
	XF_TNAME(SRC_T, NPC) arr1[COLS >>XF_BITSHIFT(NPC)];
	for(int i = 0; i < (_u_image.rows/2); i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j = 0; j < (_u_image.cols>>XF_BITSHIFT(NPC)); j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
#pragma HLS PIPELINE
#pragma HLS loop_flatten off
			XF_TNAME(SRC_T, NPC) pix= u_image.read();
			XF_TNAME(SRC_T, NPC) pix1= v_image.read();
			arr[j]=pix;
			arr1[j]=pix1;
			_u_image.data[((i*2)*(_u_image.cols>>XF_BITSHIFT(NPC)))+j] =pix;
			_v_image.data[((i*2)*(_u_image.cols>>XF_BITSHIFT(NPC)))+j] = pix1;
		}
		for(int j = 0; j < (_u_image.cols>>XF_BITSHIFT(NPC)); j++)
		{
			_u_image.data[(((i*2)+1)*(_u_image.cols>>XF_BITSHIFT(NPC)))+j] =arr[j];
			_v_image.data[(((i*2)+1)*(_u_image.cols>>XF_BITSHIFT(NPC)))+j] =arr1[j];
		}
	}


}
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFUyvy2Iyuv(
				hls::stream < XF_SNAME(WORDWIDTH_SRC) >& uyvy,
				hls::stream < XF_SNAME(WORDWIDTH_DST) >& y_plane,
				hls::stream < XF_SNAME(WORDWIDTH_DST) >& u_plane,
				hls::stream < XF_SNAME(WORDWIDTH_DST) >& v_plane, uint16_t height, uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
/*	assert(( (uyvy.cols == (y_plane.cols<<1)) && (uyvy.rows == y_plane.rows))
			&& "UYVY and Y plane dimensions mismatch");
	assert(( (uyvy.cols == (u_plane.cols<<1)) && (uyvy.rows == (u_plane.rows<<2)))
			&& "UYVY and U plane dimensions mismatch");
	assert(( (uyvy.cols == (v_plane.cols<<1)) && (uyvy.rows == (v_plane.rows<<2)))
			&& "UYVY and V plane dimensions mismatch");*/

	width = width >> XF_BITSHIFT(NPC);


	if(NPC == XF_NPPC8)
	{

		KernUyvy2Iyuv<ROWS,COLS,NPC,WORDWIDTH_SRC, WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(uyvy, y_plane, u_plane, v_plane, height, width);
	}
	else
	{
		KernUyvy2Iyuv<ROWS,COLS,NPC,WORDWIDTH_SRC, WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC))>(uyvy, y_plane, u_plane, v_plane, height, width);
	}

}


//#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_y_image.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_u_image.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_v_image.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])
template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void uyvy2iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _y_image,xf::Mat<DST_T, ROWS/4, COLS, NPC> & _u_image,xf::Mat<DST_T, ROWS/4, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<XF_TNAME(SRC_T, NPC)> src;
	hls::stream<XF_TNAME(DST_T, NPC)> dst0;
	hls::stream<XF_TNAME(DST_T, NPC)> dst1;
	hls::stream<XF_TNAME(DST_T, NPC)> dst2;
	Read_uyvy_Loop:
	for(int i=0; i<_src.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src.write( *(_src.data + i*(_src.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

		xFUyvy2Iyuv<ROWS,COLS,NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(src,dst0,dst1,dst2,_src.rows,_src.cols);

		for(int i=0; i<_y_image.rows;i++)
		{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
			for(int j=0; j<(_y_image.cols)>>(XF_BITSHIFT(NPC));j++)
			{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
				#pragma HLS PIPELINE
	#pragma HLS loop_flatten off
				*(_y_image.data + i*(_y_image.cols>>(XF_BITSHIFT(NPC))) +j) = dst0.read();
			}
		}

		for(int i=0; i<_u_image.rows;i++)
		{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
			for(int j=0; j<(_u_image.cols)>>(XF_BITSHIFT(NPC));j++)
			{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
				#pragma HLS PIPELINE
	#pragma HLS loop_flatten off
				*(_u_image.data + i*(_u_image.cols>>(XF_BITSHIFT(NPC))) +j) = dst1.read();
				*(_v_image.data + i*(_u_image.cols>>(XF_BITSHIFT(NPC))) +j) = dst2.read();
			}
		}




}
//Uyvy2Nv12
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void xFUyvy2Nv12(hls::stream < XF_SNAME(WORDWIDTH_SRC) >& uyvy,hls::stream < XF_SNAME(WORDWIDTH_Y) >& y_plane, hls::stream < XF_SNAME(WORDWIDTH_UV) >& uv_plane,uint16_t height, uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
/*	assert(( (uyvy.cols == (y_plane.cols<<1)) && (uyvy.rows == y_plane.rows))
			&& "UYVY and Y plane dimensions mismatch");
	assert(( (uyvy.cols == (uv_plane.cols<<1)) && (uyvy.rows == (uv_plane.rows<<1)))
			&& "UYVY and UV plane dimensions mismatch");*/

	width=width>>XF_BITSHIFT(NPC);


	if(NPC == XF_NPPC8)
	{
		hls::stream < ap_uint<128> > in;
#pragma HLS DATAFLOW
		//Convert64To128<ROWS,COLS,NPC,WORDWIDTH_SRC,AU_128UW,(COLS>>NPC)>(uyvy, in, height, width>>1);
		KernUyvy2Nv12<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,((COLS>>1)>>XF_BITSHIFT(NPC)),((1<<NPC)>>1)>(uyvy, y_plane, uv_plane, height, width);
		//KernUyvy2Nv12<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,((COLS>>1)>>NPC),((1<<NPC)>>1)>(uyvy, y_plane, uv_plane, height, width);
	}
	else if (NPC == XF_NPPC1)
	{
		KernUyvy2Nv12<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,((COLS>>1)>>XF_BITSHIFT(NPC))> (uyvy, y_plane, uv_plane, height, width);
	}
}
//	#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
//	#pragma SDS data data_mover("_y_image.data":AXIDMA_SIMPLE)
//	#pragma SDS data data_mover("_uv_image.data":AXIDMA_SIMPLE)
	#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
	#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
	#pragma SDS data access_pattern("_uv_image.data":SEQUENTIAL)
	#pragma SDS data copy("_src.data"[0:"_src.size"])
	#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
	#pragma SDS data copy("_uv_image.data"[0:"_uv_image.size"])
template<int SRC_T,int Y_T,int UV_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void uyvy2nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<Y_T, ROWS, COLS, NPC> & _y_image,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv_image)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW

	hls::stream<XF_TNAME(SRC_T, NPC)> src;
	hls::stream<XF_TNAME(Y_T, NPC)> dst0;
	hls::stream<XF_TNAME(UV_T, NPC_UV)> dst1;

	Read_uyvy_Loop:
	for(int i=0; i<_src.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src.write( *(_src.data + i*(_src.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}
	xFUyvy2Nv12<ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(Y_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV)>(src,dst0,dst1,_src.rows,_src.cols);

	for(int i=0; i<_y_image.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_y_image.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_y_image.data + i*(_y_image.cols>>(XF_BITSHIFT(NPC))) +j) = dst0.read();

		}
	}

	for(int i=0; i<_uv_image.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_uv_image.cols)>>(XF_BITSHIFT(NPC_UV));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_uv_image.data + i*(_uv_image.cols>>(XF_BITSHIFT(NPC_UV))) +j) = dst1.read();

		}
	}

}
//Uyvy2Rgba
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFUyvy2Rgba(
		hls::stream < XF_SNAME(WORDWIDTH_SRC) >& uyvy,hls::stream < XF_SNAME(WORDWIDTH_DST) >& rgba,uint16_t height, uint16_t width)
{
/*	assert(( (uyvy.cols == (rgba.cols<<1)) && (uyvy.rows == rgba.rows))
			&& "UYVY and RGBA plane dimensions mismatch");*/
	width=width>>XF_BITSHIFT(NPC);


		KernUyvy2Rgba<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC))>(uyvy, rgba, height, width);

}
//#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void uyvy2rgba(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<XF_TNAME(SRC_T, NPC)> src;
	hls::stream<XF_TNAME(DST_T, NPC)> dst;


	Read_uyvy_Loop:
	for(int i=0; i<_src.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src.write( *(_src.data + i*(_src.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFUyvy2Rgba<ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(src,dst,_src.rows,_src.cols);

	for(int i=0; i<_dst.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_dst.data + i*(_dst.cols>>(XF_BITSHIFT(NPC))) +j) = dst.read();

		}
	}


}
//Yuyv2Iyuv
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFYuyv2Iyuv(
		hls::stream < XF_SNAME(WORDWIDTH_SRC) >& yuyv,hls::stream < XF_SNAME(WORDWIDTH_DST) >& y_plane,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& u_plane,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& v_plane, uint16_t height, uint16_t width)
{

/*	assert(( (yuyv.cols == (y_plane.cols<<1)) && (yuyv.rows == y_plane.rows))
			&& "YUYV and Y plane dimensions mismatch");
	assert(( (yuyv.cols == (u_plane.cols<<1)) && (yuyv.rows == (u_plane.rows<<2)))
			&& "YUYV and U plane dimensions mismatch");
	assert(( (yuyv.cols == (v_plane.cols<<1)) && (yuyv.rows == (v_plane.rows<<2)))
			&& "YUYV and V plane dimensions mismatch");*/

	width=width>>XF_BITSHIFT(NPC);


	if(NPC == XF_NPPC8 )
	{

		KernYuyv2Iyuv<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(yuyv, y_plane, u_plane, v_plane, height, width);
	}
	else
	{
		KernYuyv2Iyuv<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC))>(yuyv, y_plane, u_plane, v_plane, height, width);
	}
}
//#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_y_image.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_u_image.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_v_image.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_u_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_v_image.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_u_image.data"[0:"_u_image.size"])
#pragma SDS data copy("_v_image.data"[0:"_v_image.size"])

template<int SRC_T,int DST_T,int ROWS, int COLS, int NPC=1>
void yuyv2iyuv(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<DST_T, ROWS, COLS, NPC> & _y_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _u_image, xf::Mat<DST_T, ROWS/4, COLS, NPC> & _v_image)
{
#pragma HLS INLINE OFF
	hls::stream<XF_TNAME(SRC_T, NPC)> src;
	hls::stream<XF_TNAME(DST_T, NPC)> dst0;
	hls::stream<XF_TNAME(DST_T, NPC)> dst1;
	hls::stream<XF_TNAME(DST_T, NPC)> dst2;
#pragma HLS DATAFLOW
	Read_yuyv_Loop:
	for(int i=0; i<_src.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src.write( *(_src.data + i*(_src.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}
	xFYuyv2Iyuv<ROWS,COLS,NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(src,dst0,dst1,dst2,_src.rows,_src.cols);


	for(int i=0; i<_y_image.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_y_image.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
#pragma HLS loop_flatten off
			*(_y_image.data + i*(_y_image.cols>>(XF_BITSHIFT(NPC))) +j) = dst0.read();
		}
	}

	for(int i=0; i<_u_image.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_u_image.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
#pragma HLS loop_flatten off
			*(_u_image.data + i*(_u_image.cols>>(XF_BITSHIFT(NPC))) +j) = dst1.read();
			*(_v_image.data + i*(_u_image.cols>>(XF_BITSHIFT(NPC))) +j) = dst2.read();
		}
	}


}

//Yuyv2Nv12
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void xFYuyv2Nv12(hls::stream < XF_SNAME(WORDWIDTH_SRC) >& yuyv, hls::stream < XF_SNAME(WORDWIDTH_Y) >& y_plane,
		hls::stream < XF_SNAME(WORDWIDTH_UV) >& uv_plane,uint16_t height, uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
/*	assert(( (yuyv.cols == (y_plane.cols<<1)) && (yuyv.rows == y_plane.rows))
			&& "YUYV and Y plane dimensions mismatch");
	assert(( (yuyv.cols == (uv_plane.cols<<1)) && (yuyv.rows == (uv_plane.rows<<1)))
			&& "YUYV and UV plane dimensions mismatch");*/

	width=width>>XF_BITSHIFT(NPC);


	if(NPC == XF_NPPC8)
	{
		KernYuyv2Nv12<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,((COLS>>1)>>XF_BITSHIFT(NPC)),((1<<XF_BITSHIFT(NPC))>>1)>(yuyv, y_plane, uv_plane, height, width);
	}
	else if (NPC == XF_NPPC1)
	{
		KernYuyv2Nv12<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_Y,WORDWIDTH_UV,((COLS>>1)>>XF_BITSHIFT(NPC))>(yuyv, y_plane, uv_plane, height, width);
	}
}

//#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_y_image.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_uv_image.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_y_image.data":SEQUENTIAL)
#pragma SDS data access_pattern("_uv_image.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_y_image.data"[0:"_y_image.size"])
#pragma SDS data copy("_uv_image.data"[0:"_uv_image.size"])
template<int SRC_T,int Y_T,int UV_T,int ROWS,int COLS,int NPC=1,int NPC_UV=1>
void yuyv2nv12(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<Y_T, ROWS, COLS, NPC> & _y_image,xf::Mat<UV_T, ROWS/2, COLS/2, NPC_UV> & _uv_image)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<XF_TNAME(SRC_T, NPC)> src;
	hls::stream<XF_TNAME(Y_T, NPC)> dst0;
	hls::stream<XF_TNAME(UV_T, NPC_UV)> dst1;

	Read_yuyv_Loop:
	for(int i=0; i<_src.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src.write( *(_src.data + i*(_src.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFYuyv2Nv12<ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(Y_T,NPC),XF_WORDWIDTH(UV_T,NPC_UV)>(src,dst0,dst1,_src.rows,_src.cols);

	for(int i=0; i<_y_image.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_y_image.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_y_image.data + i*(_y_image.cols>>(XF_BITSHIFT(NPC))) +j) = dst0.read();

		}
	}

	for(int i=0; i<_uv_image.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_uv_image.cols)>>(XF_BITSHIFT(NPC_UV));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_uv_image.data + i*(_uv_image.cols>>(XF_BITSHIFT(NPC_UV))) +j) = dst1.read();

		}
	}

}
//Yuyv2Rgba
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFYuyv2Rgba(hls::stream < XF_SNAME(WORDWIDTH_SRC) >& yuyv,hls::stream < XF_SNAME(WORDWIDTH_DST) >& rgba,uint16_t height, uint16_t width)
{

	width=width>>XF_BITSHIFT(NPC);

	KernYuyv2Rgba<ROWS,COLS,NPC,WORDWIDTH_SRC,WORDWIDTH_DST,((COLS>>1)>>XF_BITSHIFT(NPC))>(yuyv, rgba, height, width);

}

//#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst.data":AXIDMA_SIMPLE)

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int SRC_T,int DST_T,int ROWS,int COLS,int NPC=1>
void yuyv2rgba(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	hls::stream<XF_TNAME(SRC_T, NPC)> src;
	hls::stream<XF_TNAME(DST_T, NPC)> dst0;


	Read_yuyv_Loop:
	for(int i=0; i<_src.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src.write( *(_src.data + i*(_src.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}


	xFYuyv2Rgba<ROWS, COLS, NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC)>(src,dst0,_src.rows,_src.cols);


	for(int i=0; i<_dst.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_dst.data + i*(_dst.cols>>(XF_BITSHIFT(NPC))) +j) = dst0.read();

		}
	}
}

}

#endif
