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
#ifndef _XF_CVT_COLOR_1_HPP_
#define _XF_CVT_COLOR_1_HPP_

#ifndef __cplusplus
#error C++ is needed to compile this header !
#endif

#ifndef _XF_CVT_COLOR_HPP_
#error  This file can not be included independently !
#endif

#include "xf_cvt_color_utils.hpp"


template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC,int TC,int TCC >
void write_y(hls::stream< XF_SNAME(WORDWIDTH_SRC) >& src0,hls::stream< XF_SNAME(WORDWIDTH_SRC) >& dst0,uint16_t height,uint16_t width)
{
XF_SNAME(WORDWIDTH_SRC) tmp;
for(int i=0;i<height;i++)
{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
	for(int i=0;i<width;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
		tmp=src0.read();
		dst0.write(tmp);
	}
}
}
template<int ROWS, int COLS, int NPC, int WORDWIDTH_UV, int WORDWIDTH_DST, int TC>
void KernNv122Yuv4(
		hls::stream< XF_SNAME(WORDWIDTH_UV) >& _uv,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _u_out,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _v_out,uint16_t height,uint16_t width)
{
	XF_PTNAME(XF_16UP) uv;
	XF_SNAME(WORDWIDTH_DST) u, v;
	XF_SNAME(WORDWIDTH_UV) uvPacked;
	ap_uint<13> i,j;
	bool evenBlock = true;
	RowLoop:
	for( i = 0; i < (height>>1); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			if(evenBlock)
			{
				uv = _uv.read();
				u.range(7,0) = (uint8_t)uv.range(7,0);
				v.range(7,0) = (uint8_t)uv.range(15,8);
			}
			_u_out.write(u);
			_v_out.write(v);
			evenBlock = evenBlock ? false : true;
		}
	}
}

template<int ROWS, int COLS, int NPC, int WORDWIDTH_Y, int WORDWIDTH_UV, int WORDWIDTH_DST>
void KernNv122Rgba(
		hls::stream<XF_SNAME(WORDWIDTH_Y)>& _y,
		hls::stream< XF_SNAME(WORDWIDTH_UV) >& _uv,
		hls::stream< XF_SNAME(WORDWIDTH_DST)>& _rgba,uint16_t height,uint16_t width)
{
	hls::stream<XF_SNAME(WORDWIDTH_UV)> uvStream;
#pragma HLS STREAM variable=&uvStream  depth=COLS
	XF_SNAME(WORDWIDTH_Y) yPacked;
	XF_SNAME(WORDWIDTH_UV) uvPacked;
	XF_SNAME(WORDWIDTH_DST) rgba;
	uint8_t y1, y2;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t u, v;
	bool evenRow = true, evenBlock = true;
	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS

			yPacked = _y.read();
			if(evenRow)
			{
				if(evenBlock)
				{
					uvPacked = _uv.read();
					uvStream.write(uvPacked);
				}
			}
			else
			{// Keep a copy of UV row data in stream to use for oddrow
				if(evenBlock)
				{
					uvPacked = uvStream.read();
				}
			}
			//			auExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(UVbuf, UVPacked, 0);
			uint8_t t = yPacked.range(7,0);
			y1 = t > 16 ? t - 16 : 0;
			v = (uint8_t)uvPacked.range(15,8) - 128;
			u = (uint8_t)uvPacked.range(7,0) - 128;

			V2Rtemp = v * (short int)V2R;
			U2Gtemp = (short int)U2G * u;
			V2Gtemp = (short int)V2G * v;
			U2Btemp = u * (short int)U2B;

			// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
			// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
			// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
			rgba.range(7,0)   = CalculateR(y1, V2Rtemp, v);		  //R
			rgba.range(15,8)  = CalculateG(y1, U2Gtemp, V2Gtemp); //G
			rgba.range(23,16) = CalculateB(y1, U2Btemp, u);		  //B
			rgba.range(31,24) = 255;					          //A

			//			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			_rgba.write(rgba);
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
	if(height & 1)
	{
		for(int i = 0; i < width; i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			uvStream.read();
		}
	}
}

template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void KernNv122Iyuv(
		hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _uv,
		hls::stream< XF_SNAME(WORDWIDTH_DST) >& _u,
		hls::stream< XF_SNAME(WORDWIDTH_DST) >& _v,uint16_t height,uint16_t width)
{
	XF_PTNAME(XF_8UP) u, v;
	XF_SNAME(WORDWIDTH_SRC) uv;
ap_uint<13> i,j;
	RowLoop:
	for( i = 0; i < height>>1; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for( j = 0; j < (width>>1); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			//			u = _uv.read();
			//			v = _uv.read();
			uv = _uv.read();

			_u.write(uv.range(7,0));
			_v.write(uv.range(15,8));
		}
	}
}

template<int ROWS, int COLS, int NPC, int WORDWIDTH_VU, int WORDWIDTH_DST, int TC>
void KernNv212Yuv4(
		hls::stream< XF_SNAME(WORDWIDTH_VU) >& _vu,
		hls::stream< XF_SNAME(WORDWIDTH_DST) >& _u_out,
		hls::stream< XF_SNAME(WORDWIDTH_DST) >& _v_out,uint16_t height,uint16_t width)
{
	XF_SNAME(WORDWIDTH_VU) vu;
ap_uint<13> i,j;
	bool evenBlock = true;
	RowLoop:
	for( i = 0; i < (height>>1); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			if(evenBlock)
			{
				vu = _vu.read();
			}
			_u_out.write(vu.range(15,8));
			_v_out.write(vu.range(7,0));
			evenBlock = evenBlock ? false : true;
		}
	}
}

template<int ROWS, int COLS, int NPC, int WORDWIDTH_Y, int WORDWIDTH_VU, int WORDWIDTH_DST>
void KernNv212Rgba(
		hls::stream< XF_SNAME(WORDWIDTH_Y) >& _y,
		hls::stream< XF_SNAME(WORDWIDTH_VU) >& _vu,
		hls::stream< XF_SNAME(WORDWIDTH_DST) >& _rgba,uint16_t height,uint16_t width)
{
	hls::stream<XF_SNAME(WORDWIDTH_VU)> vuStream;
#pragma HLS STREAM variable=&vuStream  depth=COLS
	XF_SNAME(WORDWIDTH_Y) yPacked;
	XF_SNAME(WORDWIDTH_VU) vuPacked;
	XF_SNAME(WORDWIDTH_DST) rgba;
	ap_uint<13> i,j;
	uint8_t y1, y2;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t u, v;
	bool evenRow = true, evenBlock = true;
	RowLoop:
	for( i = 0; i < (height); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS

			yPacked = _y.read();
			//			auExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(Ybuf, YPacked, 0);
			if(evenRow)
			{
				if(evenBlock)
				{
					vuPacked = _vu.read();
					vuStream.write(vuPacked);
				}
			}
			else
			{// Keep a copy of UV row data in stream to use for oddrow
				if(evenBlock)
				{
					vuPacked = vuStream.read();
				}
			}
			//			auExtractPixels<NPC, WORDWIDTH_SRC, XF_8UP>(UVbuf, UVPacked, 0);
			uint8_t t = yPacked.range(7,0);
			y1 = t > 16 ? t - 16 : 0;
			u = (uint8_t)vuPacked.range(15,8) - 128;
			v = (uint8_t)vuPacked.range(7,0) - 128;

			V2Rtemp = v * (short int)V2R;
			U2Gtemp = (short int)U2G * u;
			V2Gtemp = (short int)V2G * v;
			U2Btemp = u * (short int)U2B;

			// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
			// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
			// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
			rgba.range(7,0)   = CalculateR(y1, V2Rtemp, v);		  //R
			rgba.range(15,8)  = CalculateG(y1, U2Gtemp, V2Gtemp); //G
			rgba.range(23,16) = CalculateB(y1, U2Btemp, u);		  //B
			rgba.range(31,24) = 255;					          //A

			//			PackedPixels = PackRGBAPixels<WORDWIDTH_DST>(RGB);
			_rgba.write(rgba);
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
	if(height & 1)
	{
		for( i = 0; i < width; i++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			vuStream.read();
		}
	}
}

template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void KernNv212Iyuv(
		hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _vu,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _u,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _v,uint16_t height,uint16_t width)
{
ap_uint<13> i,j;
	XF_PTNAME(XF_8UP) u, v;
	XF_SNAME(WORDWIDTH_SRC) VUPacked, UVPacked0, UVPacked1;

	RowLoop:
	for( i = 0; i < (height>>1); i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for( j = 0; j < (width>>1); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
			VUPacked = _vu.read();
			u = (uint8_t)VUPacked.range(15,8);
			v = (uint8_t)VUPacked.range(7,0);
			_u.write(u);
			_v.write(v);
		}
	}
}

template< int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void KernIyuv2Rgba(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _y,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _u,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _v,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _rgba,uint16_t height,uint16_t width)
{
	ap_uint<13> i,j;
	hls::stream<XF_SNAME(WORDWIDTH_SRC)> uStream, vStream;
#pragma HLS STREAM variable=&uStream  depth=COLS
#pragma HLS STREAM variable=&vStream  depth=COLS

	XF_SNAME(WORDWIDTH_SRC)  yPacked, uPacked, vPacked;
	XF_SNAME(WORDWIDTH_DST) rgba;


	uint8_t y1, y2;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;
	int8_t u, v;
	bool evenRow = true, evenBlock = true;
	RowLoop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			yPacked = _y.read();
			//dummy1 =  dst1.read();
			//dummy2 = dst2.read();

			if(evenBlock)
			{
				if(evenRow)
				{
					uPacked = _u.read();
					uStream.write(uPacked);
					vPacked = _v.read();
					vStream.write(vPacked);
				}
				else
				{
					/* Copy of the U and V values are pushed into stream to be used for next row */
					uPacked = uStream.read();
					vPacked = vStream.read();
				}
			}

			y1 = (uint8_t)yPacked.range(7,0) > 16 ? (uint8_t)yPacked.range(7,0)-16 : 0;

			u = (uint8_t)uPacked.range(7,0) - 128;
			v = (uint8_t)vPacked.range(7,0) - 128;

			V2Rtemp = v * (short int)V2R;
			U2Gtemp = (short int)U2G * u;
			V2Gtemp = (short int)V2G * v;
			U2Btemp = u * (short int)U2B;

			// R = 1.164*Y + 1.596*V = Y + 0.164*Y + V + 0.596*V
			// G = 1.164*Y - 0.813*V - 0.391*U = Y + 0.164*Y - 0.813*V - 0.391*U
			// B = 1.164*Y + 2.018*U = Y + 0.164 + 2*U + 0.018*U
			rgba.range(7,0)   = CalculateR(y1, V2Rtemp, v);		  //R
			rgba.range(15,8)  = CalculateG(y1, U2Gtemp, V2Gtemp); //G
			rgba.range(23,16) = CalculateB(y1, U2Btemp, u);		  //B
			rgba.range(31,24) = 255;					          //A

			_rgba.write(rgba);
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false: true;
	}
}

template<int ROWS, int COLS, int NPC, int WORDWIDTH, int rTC, int cTC>
void KernIyuv2Yuv4(
		hls::stream<XF_SNAME(WORDWIDTH)>& _in_u,
		hls::stream<XF_SNAME(WORDWIDTH)>& _in_v,
		hls::stream<XF_SNAME(WORDWIDTH)>& _out_u,
		hls::stream<XF_SNAME(WORDWIDTH)>& _out_v,uint16_t height,uint16_t width)
{
	XF_SNAME(WORDWIDTH) IUPacked, IVPacked;
	XF_PTNAME(XF_8UP) in_u, in_v;
	RowLoop:
	for(int i = 0; i < ((height>>2) << 1); i++)
	{
#pragma HLS LOOP_FLATTEN
#pragma HLS LOOP_TRIPCOUNT min=rTC max=rTC
		ColLoop:
		for(int j = 0; j < (width >> 1); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=cTC max=cTC
			IUPacked = _in_u.read();
			IVPacked = _in_v.read();

			_out_u.write(IUPacked);
			_out_v.write(IVPacked);
			_out_u.write(IUPacked);
			_out_v.write(IVPacked);
		}
	}
}

template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_UV, int rTC, int cTC>
void KernIyuv2Nv12(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>&  _u,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>&  _v,
		hls::stream< XF_SNAME(WORDWIDTH_UV) >& _uv,uint16_t height,uint16_t width)
{
ap_uint<13> i,j;
	XF_SNAME(WORDWIDTH_SRC) u, v;
	XF_SNAME(WORDWIDTH_UV) uv;
	RowLoop:
	for( i = 0; i < height>>1; i++)
	{
		// Reading the plane interleaved U and V data from streams,
		// packing them in pixel interleaved and writing out to UV stream
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=rTC max=rTC
		ColLoop:
		for( j = 0; j < (width>>1); j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=cTC max=cTC
			u = _u.read();
			v = _v.read();
			uv.range(7,0) = u;
			uv.range(15,8) = v;
			_uv.write(uv);
		}
	}
}


template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void KernRgba2Yuv4(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _rgba,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _y,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _u,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _v,uint16_t height,uint16_t width )
{
	XF_SNAME(XF_32UW) rgba;
	uint8_t y, u, v;

	RowLoop:
	for(int i = 0; i < height; ++i)
	{
#pragma HLS LOOP_FLATTEN OFF
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < width; ++j)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
#pragma HLS PIPELINE
			rgba = _rgba.read();

			y = CalculateY(rgba.range(7,0),rgba.range(15,8), rgba.range(23,16));
			u = CalculateU(rgba.range(7,0),rgba.range(15,8), rgba.range(23,16));
			v = CalculateV(rgba.range(7,0),rgba.range(15,8), rgba.range(23,16));

			_y.write(y);
			_u.write(u);
			_v.write(v);
		}
	}
}

template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int ROWS_U,int ROWS_V>
void KernRgba2Iyuv(
		hls::stream < XF_SNAME(WORDWIDTH_SRC) >& _rgba,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& _y,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& _u,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& _v,uint16_t height,uint16_t width)
{
	XF_SNAME(XF_32UW) rgba;
	uint8_t y, u, v;
	bool evenRow = true, evenBlock = true;

	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			rgba = _rgba.read();
			uint8_t r = rgba.range(7,0);
			uint8_t g = rgba.range(15,8);
			uint8_t b = rgba.range(23,16);

			y = CalculateY(r, g, b);
			if(evenRow)
			{
				if(evenBlock)
				{
					u = CalculateU(r, g, b);
					v = CalculateV(r, g, b);
				}
			}
			_y.write(y);
			if(evenRow & !evenBlock)
			{
				_u.write(u);
				_v.write(v);
			}
			evenBlock = evenBlock ? false : true;
		}
		evenRow = evenRow ? false : true;
	}
}

template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV>
void KernRgba2Nv12(
		hls::stream < XF_SNAME(WORDWIDTH_SRC) >&  _rgba,
		hls::stream < XF_SNAME(WORDWIDTH_Y) >&  _y,
		hls::stream < XF_SNAME(WORDWIDTH_UV) >&  _uv,uint16_t height,uint16_t width)
{
	//	XF_SNAME(XF_32UW) rgba;
	ap_uint<32> rgba;
	ap_uint<16> val1;
	uint8_t y, u, v;
	bool evenRow = true, evenBlock = true;

	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			rgba = _rgba.read();
			uint8_t r = rgba.range(7,0);
			uint8_t g = rgba.range(15,8);
			uint8_t b = rgba.range(23,16);

			y = CalculateY(r, g, b);
			if(evenRow)
			{
				u = CalculateU(r, g, b);
				v = CalculateV(r, g, b);
			}
			_y.write(y);
			if(evenRow)
			{
				if((j & 0x01) == 0)
				//{
					_uv.write(u | (uint16_t)v << 8);
					//_uv.write(v);
				//}
				//	_uv.write(u | (uint16_t)v << 8);
			}
		}
		evenRow = evenRow ? false : true;
	}
}

template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_VU>
void KernRgba2Nv21(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _rgba,
		hls::stream<XF_SNAME(WORDWIDTH_Y)>& _y,
		hls::stream<XF_SNAME(WORDWIDTH_VU) >& _vu,uint16_t height,uint16_t width)
{
	width=width>>XF_BITSHIFT(NPC);
	XF_SNAME(XF_32UW) rgba;
	uint8_t y, u, v;
	bool evenRow = true, evenBlock = true;

	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < width; j++)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=COLS max=COLS
			rgba = _rgba.read();
			uint8_t r = rgba.range(7,0);
			uint8_t g = rgba.range(15,8);
			uint8_t b = rgba.range(23,16);

			y = CalculateY(r, g, b);
			if(evenRow)
			{
				u = CalculateU(r, g, b);
				v = CalculateV(r, g, b);
			}
			_y.write(y);
			if(evenRow)
			{
				if((j & 0x01)==0)
					_vu.write(v | ((uint16_t)u << 8));
			}
		}
		evenRow = evenRow ? false : true;
	}
}

//Yuyv2Rgba
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void KernYuyv2Rgba(
		hls::stream < XF_SNAME(WORDWIDTH_SRC) >& _yuyv,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& _rgba, uint16_t height, uint16_t width)
{
	XF_SNAME(WORDWIDTH_DST) rgba;
	XF_SNAME(WORDWIDTH_SRC) yu,yv;
	XF_PTNAME(XF_8UP)  r, g, b;
	int8_t y1, y2, u, v;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;

	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		ColLoop:
		for(int j = 0; j < width ; j += 2)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline

			yu = _yuyv.read();
			yv = _yuyv.read();
			u = (uint8_t)yu.range(15,8) - 128;
			y1 = (yu.range(7,0) > 16) ? ((uint8_t)yu.range(7,0) - 16) : 0;

			v = (uint8_t)yv.range(15,8) - 128;
			y2 = (yv.range(7,0) > 16) ? ((uint8_t)yv.range(7,0) - 16) : 0;

			V2Rtemp = v * (short int)V2R;
			U2Gtemp = (short int)U2G * u;
			V2Gtemp = (short int)V2G * v;
			U2Btemp = u * (short int)U2B;

			r = CalculateR(y1, V2Rtemp, v);
			g = CalculateG(y1, U2Gtemp, V2Gtemp);
			b = CalculateB(y1, U2Btemp, u);

			rgba = ((ap_uint32_t)r) | ((ap_uint32_t)g << 8) | ((ap_uint32_t)b << 16) | (0xFF000000);
			_rgba.write(rgba);

			r = CalculateR(y2, V2Rtemp, v);
			g = CalculateG(y2, U2Gtemp, V2Gtemp);
			b = CalculateB(y2, U2Btemp, u);

			rgba = ((ap_uint32_t)r) | ((ap_uint32_t)g << 8) | ((ap_uint32_t)b << 16) | (0xFF000000);
			_rgba.write(rgba);
		}
	}
}


//Yuyv2Nv12
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV, int TC>
void KernYuyv2Nv12(hls::stream < XF_SNAME(WORDWIDTH_SRC) >& _yuyv,
		hls::stream < XF_SNAME(WORDWIDTH_Y) >& _y,
		hls::stream < XF_SNAME(WORDWIDTH_UV) >& _uv, uint16_t height, uint16_t width)
{
	XF_SNAME(WORDWIDTH_SRC) yu,yv;
	XF_PTNAME(XF_8UP) y1, y2;
	XF_SNAME(WORDWIDTH_UV) uv;
	bool evenRow = true;
	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < width; j+=2)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC

			yu = _yuyv.read();
			yv = _yuyv.read();
			y1 = yu.range(7,0);
			if(evenRow)
				uv.range(7,0) = yu.range(15,8);

			y2 = yv.range(7,0);
			if(evenRow)
				uv.range(15,8) = yv.range(15,8);

			_y.write(y1); _y.write(y2);
			if(evenRow)
			{
				_uv.write(uv);
			}
		}
		evenRow = evenRow ? false : true;
	}
}

//Yuyv2Iyuv
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void KernYuyv2Iyuv(	hls::stream < XF_SNAME(WORDWIDTH_SRC) >& _yuyv,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& _y,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& _u,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& _v, uint16_t height, uint16_t width)
{
	XF_SNAME(WORDWIDTH_SRC) yu,yv;
	bool evenRow = true, evenBlock = true;
	XF_PTNAME(XF_8UP) y1, y2, u, v;

	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < width; j+=2)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC

			yu = _yuyv.read();
			yv = _yuyv.read();
			y1 = yu.range(7,0);
			y2 = yv.range(7,0);
			_y.write(y1);
			_y.write(y2);
			if(evenRow)
				u = yu.range(15,8);

			if(evenRow)
				v = yv.range(15,8);

			if(evenRow)
			{
				_u.write(u);
				_v.write(v);
			}
		}
		evenRow = evenRow ? false : true;
	}
}

template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void KernUyvy2Iyuv(	hls::stream < XF_SNAME(WORDWIDTH_SRC) >& _uyvy,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& _y,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& _u,
		hls::stream < XF_SNAME(WORDWIDTH_DST)>& _v, uint16_t height, uint16_t width)

{

	XF_SNAME(WORDWIDTH_SRC) uy,vy;
	bool evenRow = true, evenBlock = true;
	XF_PTNAME(XF_8UP) y1, y2, u, v;

	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < width; j+=2)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC

			uy = _uyvy.read();
			vy = _uyvy.read();


			y1 = uy.range(15,8);

			_y.write(y1);
			if(evenRow)
				u = uy.range(7,0);

			y2 = vy.range(15,8);

			_y.write(y2);
			if(evenRow)
				v = vy.range(7,0);

			if(evenRow)
			{
				_u.write(u);
				_v.write(v);
			}
		}
		evenRow = evenRow ? false : true;
	}

}

//Uyvy2Nv12
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_Y, int WORDWIDTH_UV, int TC>
void KernUyvy2Nv12(hls::stream < XF_SNAME(WORDWIDTH_SRC) >& _uyvy,
		hls::stream < XF_SNAME(WORDWIDTH_Y) >& _y,
		hls::stream < XF_SNAME(WORDWIDTH_UV) >& _uv, uint16_t height, uint16_t width)
{

	XF_SNAME(WORDWIDTH_SRC) uy,vy;
	XF_PTNAME(XF_8UP) y1, y2;
	XF_SNAME(WORDWIDTH_UV) uv;
	bool evenRow = true;
	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_FLATTEN off
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		ColLoop:
		for(int j = 0; j < width; j+=2)
		{
#pragma HLS pipeline
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC

			uy = _uyvy.read();
			vy = _uyvy.read();

			y1 = uy.range(15,8);
			if(evenRow)
				uv.range(7,0) = uy.range(7,0);

			y2 = vy.range(15,8);
			if(evenRow)
				uv.range(15,8) = vy.range(7,0);

			_y.write(y1); _y.write(y2);
			if(evenRow)
			{
				_uv.write(uv);
			}
		}
		evenRow = evenRow ? false : true;
	}

}

//Uyvy2Rgba
template<int ROWS, int COLS, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int TC>
void KernUyvy2Rgba(
		hls::stream < XF_SNAME(WORDWIDTH_SRC) >& _uyvy,
		hls::stream < XF_SNAME(WORDWIDTH_DST) >& _rgba, uint16_t height, uint16_t width)
{
	XF_SNAME(WORDWIDTH_DST) rgba;

	XF_SNAME(WORDWIDTH_SRC) uyvy;

	XF_SNAME(WORDWIDTH_SRC) uy;
	XF_SNAME(WORDWIDTH_SRC) vy;

	XF_PTNAME(XF_8UP)  r, g, b;
	int8_t y1, y2, u, v;
	int32_t V2Rtemp, U2Gtemp, V2Gtemp, U2Btemp;

	RowLoop:
	for(int i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		ColLoop:
		for(int j = 0; j < width; j+=2)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline

			//uyvy = _uyvy.read();

			uy = _uyvy.read();
			vy = _uyvy.read();

			u = (uint8_t)uy.range(7,0) - 128;

/*			if(uyvy.range(15,8) > 16)
				y1 = (uint8_t)uyvy.range(15,8) - 16;
			else
				y1 = 0;*/

			y1 = (uy.range(15,8) > 16) ? ((uint8_t)uy.range(15,8) - 16) : 0;

			v = (uint8_t)vy.range(7,0) - 128;

/*			if(uyvy.range(31,24) > 16)
				y2 = ((uint8_t)uyvy.range(31,24) - 16);
			else
				y2 = 0;*/
			y2 = (vy.range(15,8) > 16) ? ((uint8_t)vy.range(15,8) - 16) : 0;

			V2Rtemp = v * (short int)V2R;
			U2Gtemp = (short int)U2G * u;
			V2Gtemp = (short int)V2G * v;
			U2Btemp = u * (short int)U2B;

			r = CalculateR(y1, V2Rtemp, v);
			g = CalculateG(y1, U2Gtemp, V2Gtemp);
			b = CalculateB(y1, U2Btemp, u);

			rgba = ((ap_uint32_t)r) | ((ap_uint32_t)g << 8) | ((ap_uint32_t)b << 16) | (0xFF000000);
			_rgba.write(rgba);

			r = CalculateR(y2, V2Rtemp, v);
			g = CalculateG(y2, U2Gtemp, V2Gtemp);
			b = CalculateB(y2, U2Btemp, u);

			rgba = ((ap_uint32_t)r) | ((ap_uint32_t)g << 8) | ((ap_uint32_t)b << 16) | (0xFF000000);
			_rgba.write(rgba);
		}
	}
}
#endif
