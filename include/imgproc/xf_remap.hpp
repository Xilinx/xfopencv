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

#ifndef _XF_REMAP_HPP_
#define _XF_REMAP_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header.
#endif

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

#define HLS_INTER_TAB_SIZE 32
#define HLS_INTER_BITS     5

namespace xf{

template <int WIN_ROW, int ROWS, int COLS, typename SRC_T, typename DST_T, typename MAP_T>
void xFRemapNNI(
		hls::stream< SRC_T >   &src,
		hls::stream< DST_T >   &dst,
		hls::stream< MAP_T >   &mapx,
		hls::stream< MAP_T >   &mapy,
		uint16_t rows, uint16_t cols
)
{
	DST_T buf[WIN_ROW][COLS];
#pragma HLS ARRAY_PARTITION variable=buf complete dim=1

	SRC_T s;
	DST_T d;
	MAP_T mx_fl;
	MAP_T my_fl;

	assert(rows <= ROWS);
	assert(cols <= COLS);
	int ishift=WIN_ROW/2;
	int r[WIN_ROW] = {};

	loop_height: for( int i=0; i< rows+ishift; i++)
	{
#pragma HLS LOOP_FLATTEN OFF
		loop_width: for( int j=0; j< cols; j++)
		{
#pragma HLS PIPELINE II=1
#pragma HLS dependence array inter false
			if(i<rows&& j<cols)
			{
				src >> s;
			}
			buf[i % WIN_ROW][j] = s;
			r[i % WIN_ROW] = i;

			if(i>=ishift)
			{
				mapx >> mx_fl;
				mapy >> my_fl;
				int x = (int)mx_fl;
				int y = (int)my_fl;

				bool in_range = (y>=0 && y<rows && r[y%WIN_ROW] == y && x>=0 && x<cols);
				if(in_range)
					d = buf[y%WIN_ROW][x];
				else
					d = 0;

				dst << d;
			}
		}
	}
}


#define TWO_POW_16 65536
template <int WIN_ROW, int ROWS, int COLS, typename SRC_T, typename DST_T, typename MAP_T>
void xFRemapLI(
		hls::stream< SRC_T >   &src,
		hls::stream< DST_T >   &dst,
		hls::stream< MAP_T >   &mapx,
		hls::stream< MAP_T >   &mapy,
		uint16_t rows, uint16_t cols
)
{
	// Add one to always get zero for boundary interpolation. Maybe need initialization here?
	DST_T buf[WIN_ROW/2+1][2][COLS/2+1][2];
#pragma HLS array_partition complete variable=buf dim=2
#pragma HLS array_partition complete variable=buf dim=4
	SRC_T s;
	MAP_T mx;
	MAP_T my;

	assert(rows <= ROWS);
	assert(cols <= COLS);
	int ishift=WIN_ROW/2;
	int r1[WIN_ROW] = {};
	int r2[WIN_ROW] = {};

	loop_height: for( int i=0; i< rows+ishift; i++)
	{
#pragma HLS LOOP_FLATTEN OFF
		loop_width: for( int j=0; j< cols; j++)
		{
#pragma HLS PIPELINE II=1
#pragma HLS dependence array inter false
			if(i<rows&& j<cols)
			{
				src >> s;
			}
			if((i % WIN_ROW) % 2) {
				buf[(i % WIN_ROW)/2][(i % WIN_ROW) % 2][j/2][j%2] = s;
			} else {
				buf[(i % WIN_ROW)/2][(i % WIN_ROW) % 2][j/2][j%2] = s;
			}
			r1[i % WIN_ROW] = i;
			r2[i % WIN_ROW] = i;

			if(i>=ishift)
			{
				mapx >> mx;
				mapy >> my;
				float x_fl = mx;
				float y_fl = my;

				int x_fix = (int) ((float)x_fl * (float)HLS_INTER_TAB_SIZE);  // mapx data in A16.HLS_INTER_TAB_SIZE format
				int y_fix = (int) ((float)y_fl * (float)HLS_INTER_TAB_SIZE);  // mapy data in A16.HLS_INTER_TAB_SIZE format

				int x = x_fix >> HLS_INTER_BITS;
				int y = y_fix >> HLS_INTER_BITS;
				int x_frac = x_fix  & (HLS_INTER_TAB_SIZE-1);
				int y_frac = y_fix  & (HLS_INTER_TAB_SIZE-1);
				int ynext = y+1;

				ap_ufixed<HLS_INTER_BITS, 0> iu, iv;
				iu(HLS_INTER_BITS-1, 0) = x_frac;
				iv(HLS_INTER_BITS-1, 0) = y_frac;

				// Note that the range here is larger than expected by 1 horizontal and 1 vertical pixel, to allow
				// Interpolating at the edge of the image
				bool in_range = (y>=0 && y<rows && r1[y%WIN_ROW] == y && r2[ynext%WIN_ROW] == ynext && x>=0 && x<cols);

				int xa0, xa1, ya0, ya1;
				// The buffer is essentially cyclic partitioned, but we have
				// to do this manually because HLS can't figure it out.
				// The code below is wierd, but it is this code expanded.
				//  if ((y % WIN_ROW) % 2) {
				//                     // Case 1, where y hits in bank 1 and ynext in bank 0
				//                     ya0 = (ynext%WIN_ROW)/2;
				//                     ya1 = (y%WIN_ROW)/2;
				//                 } else {
				//                     // The simpler case, where y hits in bank 0 and ynext hits in bank 1
				//                     ya0 = (y%WIN_ROW)/2;
				//                     ya1 = (ynext%WIN_ROW)/2;
				//                 }
				// Both cases reduce to this, if WIN_ROW is a multiple of two.
				assert(((WIN_ROW & 1) == 0) && "WIN_ROW must be a multiple of two");
				xa0 = x/2 + x%2;
				xa1 = x/2;
				ya0 = (y/2 + y%2)%(WIN_ROW/2);
				ya1 = (y/2)%(WIN_ROW/2);

				DST_T d00, d01, d10, d11;
				d00=buf[ya0][0][xa0][0];
				d01=buf[ya0][0][xa1][1];
				d10=buf[ya1][1][xa0][0];
				d11=buf[ya1][1][xa1][1];

				if(x%2) {
					std::swap(d00,d01);
					std::swap(d10,d11);
				}
				if(y%2) {
					std::swap(d00,d10);
					std::swap(d01,d11);
				}
				ap_ufixed<2*HLS_INTER_BITS + 1, 1> k01 = (1-iv)*(  iu); // iu-iu*iv
				ap_ufixed<2*HLS_INTER_BITS + 1, 1> k10 = (  iv)*(1-iu); // iv-iu*iv
				ap_ufixed<2*HLS_INTER_BITS + 1, 1> k11 = (  iv)*(  iu); // iu*iv
				ap_ufixed<2*HLS_INTER_BITS + 1, 1> k00 = 1-iv-k01; //(1-iv)*(1-iu) = 1-iu-iv+iu*iv = 1-iv-k01
				assert( k00 + k01 + k10 + k11 == 1);

				DST_T d;

				if(in_range)
					d = d00 * k00 + d01 * k01 + d10 * k10 + d11 * k11;
				else
					d = 0;

				dst<< d;
			}
		}
	}
}

template <int WIN_ROW, int ROWS, int COLS, typename SRC_T, typename DST_T, typename MAP_T>
void xFRemapKernel(
		hls::stream< SRC_T >    &src,
		hls::stream< DST_T >   &dst,
		hls::stream< MAP_T >   &mapx,
		hls::stream< MAP_T >   &mapy,
		int interpolation,
		uint16_t rows, uint16_t cols
)
{
	if(interpolation == XF_INTERPOLATION_NN) {
		xFRemapNNI<WIN_ROW,ROWS,COLS>(src, dst, mapx, mapy,rows,cols);
	} else if(interpolation == XF_INTERPOLATION_BILINEAR) {
		xFRemapLI<WIN_ROW,ROWS,COLS>(src, dst, mapx, mapy,rows,cols);
	}
}

//#pragma SDS data data_mover("_src_mat.data":AXIDMA_SIMPLE,"_remapped_mat.data":AXIDMA_SIMPLE,"_mapx_mat.data":AXIDMA_SIMPLE,"_mapy_mat.data":AXIDMA_SIMPLE)
#pragma SDS data mem_attribute("_src_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS,"_remapped_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS,"_mapx_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS,"_mapy_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL,"_remapped_mat.data":SEQUENTIAL,"_mapx_mat.data":SEQUENTIAL,"_mapy_mat.data":SEQUENTIAL)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.rows*_src_mat.cols"], "_remapped_mat.data"[0:"_remapped_mat.size"],"_mapx_mat.data"[0:"_mapx_mat.size"],"_mapy_mat.data"[0:"_mapy_mat.size"])
template<int WIN_ROWS, int SRC_T, int MAP_T, int DST_T, int ROWS, int COLS, int NPC = XF_NPPC1>
void remap (xf::Mat<SRC_T, ROWS, COLS, NPC> &_src_mat, xf::Mat<DST_T, ROWS, COLS, NPC> &_remapped_mat, xf::Mat<MAP_T, ROWS, COLS, NPC> &_mapx_mat,
		xf::Mat<MAP_T, ROWS, COLS, NPC> &_mapy_mat, int interpolation=XF_INTERPOLATION_NN)
{
#pragma HLS inline off
#pragma HLS dataflow

	assert ((MAP_T == XF_32FC1) && "The MAP_T must be XF_32FC1");
	assert ((SRC_T == XF_8UC1) && "The SRC_T must be XF_8UC1");
	assert ((DST_T == XF_8UC1) && "The DST_T must be XF_8UC1");
	assert ((NPC == XF_NPPC1) && "The NPC must be XF_NPPC1");

	hls::stream< XF_TNAME(SRC_T,NPC)> _src;
	hls::stream< XF_TNAME(MAP_T,NPC)> _mapx;
	hls::stream< XF_TNAME(MAP_T,NPC)> _mapy;
	hls::stream< XF_TNAME(DST_T,NPC)> _remapped;

	int depth_est = WIN_ROWS*_src_mat.cols;

	uint16_t rows = _src_mat.rows;
	uint16_t cols = _src_mat.cols;

	int loop_count = (rows*cols);
	int TC=(ROWS*COLS);

	int ishift = WIN_ROWS/2;

	xfremap_rows_loop:
	for (int i = 0; i < rows+ishift; i++)
	{
#pragma HLS LOOP_FLATTEN OFF

		xfremap_cols_loop:
		for (int j = 0; j < cols; j++)
		{
#pragma HLS pipeline ii=1

			if (i < rows) {
				_src.write(*(_src_mat.data + i*cols + j));
			}

			if (i >= ishift) {
				_mapx.write(*(_mapx_mat.data + (i-ishift)*cols + j));
				_mapy.write(*(_mapy_mat.data + (i-ishift)*cols + j));
			}
		}
	}

	xFRemapKernel <WIN_ROWS,ROWS,COLS> (_src, _remapped, _mapx, _mapy, interpolation, rows, cols);

	xfremap_output_loop:
	for (int i = 0; i < loop_count; i++)
	{
#pragma HLS pipeline ii=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=TC
		_remapped_mat.data[i] = _remapped.read();
	}
}
}

#endif//_XF_REMAP_HPP_
