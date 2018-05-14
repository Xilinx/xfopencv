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
 HOWEVER CXFSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ***************************************************************************/
#ifndef _XF_PYR_DOWN_GAUSSIAN_DOWN_
#define _XF_PYR_DOWN_GAUSSIAN_DOWN_

#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.h"

template<int NPC,int DEPTH, int WIN_SZ, int WIN_SZ_SQ>
void xFPyrDownApplykernel(
		XF_PTUNAME(DEPTH) OutputValues[XF_NPIXPERCYCLE(NPC)],
		XF_PTUNAME(DEPTH) src_buf[WIN_SZ][XF_NPIXPERCYCLE(NPC)+(WIN_SZ-1)],
		ap_uint<8> win_size
		)
{
#pragma HLS INLINE
	unsigned int array[WIN_SZ_SQ];
#pragma HLS ARRAY_PARTITION variable=array complete dim=1
		
		
		int array_ptr=0;
		Compute_Grad_Loop:
		for(int copy_arr=0;copy_arr<WIN_SZ;copy_arr++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIN_SZ
#pragma HLS UNROLL
			for (int copy_in=0; copy_in<WIN_SZ; copy_in++)
			{
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIN_SZ
#pragma HLS UNROLL
				array[array_ptr] = src_buf[copy_arr][copy_in];
				array_ptr++;
			}
		}
unsigned int out_pixel = 0;
		int k[25]={1,  4,  6,  4, 1, 
		           4, 16, 24, 16, 4, 
				   6, 24, 36, 24, 6, 
				   4, 16, 24, 16, 4, 
				   1,  4,  6,  4, 1};
		out_pixel  = array[0*5 + 0] + array[0*5 + 4] + array[4*5 + 0] + array[4*5 + 4];
		out_pixel += (array[0*5 + 1] + array[0*5 + 3] + array[1*5 + 0] + array[1*5 + 4]) << 2;
		out_pixel += (array[4*5 + 1] + array[4*5 + 3] + array[3*5 + 0] + array[3*5 + 4]) << 2;
		out_pixel += (array[0*5 + 2] + array[2*5 + 0] + array[2*5 + 4] + array[4*5 + 2]) << 2;
		out_pixel += (array[0*5 + 2] + array[2*5 + 0] + array[2*5 + 4] + array[4*5 + 2]) << 1;
		out_pixel += (array[1*5 + 1] + array[1*5 + 3] + array[3*5 + 1] + array[3*5 + 3]) << 4;
		out_pixel += (array[1*5 + 2] + array[2*5 + 1] + array[2*5 + 3] + array[3*5 + 2]) << 4;
		out_pixel += (array[1*5 + 2] + array[2*5 + 1] + array[2*5 + 3] + array[3*5 + 2]) << 3;
		out_pixel += (array[2*5 + 2]) << 5;
		out_pixel += (array[2*5 + 2]) << 2;
		OutputValues[0] = (unsigned char)( (out_pixel + 128) >> 8);
		return;
}

template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH, int TC, int WIN_SZ, int WIN_SZ_SQ>
void xFPyrDownprocessgaussian(hls::stream< XF_TNAME(DEPTH,NPC) > & _src_mat,
		hls::stream< XF_TNAME(DEPTH,NPC) > & _out_mat,
		XF_TNAME(DEPTH,NPC) buf[WIN_SZ][(COLS >> XF_BITSHIFT(NPC))], XF_PTUNAME(DEPTH) src_buf[WIN_SZ][XF_NPIXPERCYCLE(NPC)+(WIN_SZ-1)],
		XF_PTUNAME(DEPTH) OutputValues[XF_NPIXPERCYCLE(NPC)],
		XF_PTUNAME(DEPTH) &P0, uint16_t img_width,  uint16_t img_height, uint16_t &shift_x,  ap_uint<13> row_ind[WIN_SZ], ap_uint<13> row, ap_uint<8> win_size)
{
#pragma HLS INLINE

	XF_TNAME(DEPTH,NPC) buf_cop[WIN_SZ];
#pragma HLS ARRAY_PARTITION variable=buf_cop complete dim=1
	
	uint16_t npc = XF_NPIXPERCYCLE(NPC);
	Col_Loop:
	for(ap_uint<13> col = 0; col < img_width+(WIN_SZ>>1); col++)
	{
#pragma HLS LOOP_FLATTEN OFF
#pragma HLS LOOP_TRIPCOUNT min=1 max=TC
#pragma HLS pipeline

        XF_TNAME(DEPTH,NPC) bufWord[WIN_SZ];
#pragma HLS ARRAY_PARTITION variable=bufWord complete dim=1
	    for (int k=0; k<WIN_SZ; k++) bufWord[k] = buf[k][col];
		if(row < img_height && col < img_width)
			bufWord[row_ind[win_size-1]] = _src_mat.read(); // Read data
	    for (int k=0; k<WIN_SZ; k++) buf[k][col] = bufWord[k];

		for(int copy_buf_var=0;copy_buf_var<WIN_SZ;copy_buf_var++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIN_SZ
#pragma HLS UNROLL
			if(	(row >(img_height-1)) && (copy_buf_var>(win_size-1-(row-(img_height-1)))))
			{
				buf_cop[copy_buf_var] = buf[(row_ind[win_size-1-(row-(img_height-1))])][col];
			}
			else
			{
				buf_cop[copy_buf_var] = buf[(row_ind[copy_buf_var])][col];
			}
		}
		for(int extract_px=0;extract_px<WIN_SZ;extract_px++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIN_SZ
#pragma HLS UNROLL
			if(col<img_width)
			{
				src_buf[extract_px][win_size-1] = buf_cop[extract_px];
			}
			else
			{
				src_buf[extract_px][win_size-1] = src_buf[extract_px][win_size-2];
			}
		}

		xFPyrDownApplykernel<NPC, DEPTH, WIN_SZ, WIN_SZ_SQ>(OutputValues,src_buf, win_size);
		if(col >= (win_size>>1))
		{
			_out_mat.write(OutputValues[0]);
		}
		
		for(int wrap_buf=0;wrap_buf<WIN_SZ;wrap_buf++)
		{
	#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIN_SZ
			for(int col_warp=0; col_warp<WIN_SZ-1;col_warp++)
			{
	#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIN_SZ
				if(col==0)
				{
					src_buf[wrap_buf][col_warp] = src_buf[wrap_buf][win_size-1];
				}
				else
				{
					src_buf[wrap_buf][col_warp] = src_buf[wrap_buf][col_warp+1];
				}
			}
		}
	} // Col_Loop
}



template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH, int TC,int WIN_SZ, int WIN_SZ_SQ, bool USE_URAM>
void xf_pyrdown_gaussian_nxn(hls::stream< XF_TNAME(DEPTH,NPC) > &_src_mat,
		hls::stream< XF_TNAME(DEPTH,NPC) > &_out_mat, ap_uint<8> win_size,
		uint16_t img_height, uint16_t img_width)
{
	ap_uint<13> row_ind[WIN_SZ];
#pragma HLS ARRAY_PARTITION variable=row_ind complete dim=1
	
	ap_uint<8> buf_size = XF_NPIXPERCYCLE(NPC) + (WIN_SZ-1);
	uint16_t shift_x = 0;
	ap_uint<13> row, col;

	XF_TNAME(DEPTH,NPC) OutputValues[XF_NPIXPERCYCLE(NPC)];
#pragma HLS ARRAY_PARTITION variable=OutputValues complete dim=1


	XF_PTUNAME(DEPTH) src_buf[WIN_SZ][XF_NPIXPERCYCLE(NPC)+(WIN_SZ-1)];
#pragma HLS ARRAY_PARTITION variable=src_buf complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf complete dim=2
// src_buf1 et al merged 
	XF_TNAME(DEPTH,NPC) P0;

	XF_TNAME(DEPTH,NPC) buf[WIN_SZ][(COLS >> XF_BITSHIFT(NPC))];
#pragma HLS ARRAY_RESHAPE variable=buf complete dim=1 	
if (USE_URAM) {
#pragma HLS RESOURCE variable=buf core=XPM_MEMORY uram
} else {
#pragma HLS RESOURCE variable=buf core=RAM_S2P_BRAM
}

//initializing row index
	
	for(int init_row_ind=0; init_row_ind<win_size; init_row_ind++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=WIN_SZ
		row_ind[init_row_ind] = init_row_ind; 
	}
	
	read_lines:
	for(int init_buf=row_ind[win_size>>1]; init_buf <row_ind[win_size-1] ;init_buf++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=WIN_SZ
		for(col = 0; col < img_width; col++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=TC
	#pragma HLS LOOP_FLATTEN OFF
	#pragma HLS pipeline
			buf[init_buf][col] = _src_mat.read();
		}
	}
	
	//takes care of top borders
		for(col = 0; col < img_width; col++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=TC
			for(int init_buf=0; init_buf < WIN_SZ>>1;init_buf++)
			{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=WIN_SZ
	#pragma HLS UNROLL
				buf[init_buf][col] = buf[row_ind[win_size>>1]][col];
			}
		}
	
	Row_Loop:
	for(row = (win_size>>1); row < img_height+(win_size>>1); row++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		P0 = 0;
		xFPyrDownprocessgaussian<ROWS, COLS, DEPTH, NPC, WORDWIDTH, TC, WIN_SZ, WIN_SZ_SQ>(_src_mat, _out_mat, buf, src_buf,OutputValues, P0, img_width, img_height, shift_x, row_ind, row,win_size);
	
		//update indices
		ap_uint<13> zero_ind = row_ind[0];
		for(int init_row_ind=0; init_row_ind<WIN_SZ-1; init_row_ind++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=WIN_SZ
	#pragma HLS UNROLL
			row_ind[init_row_ind] = row_ind[init_row_ind + 1];
		}
		row_ind[win_size-1] = zero_ind;
		
	} // Row_Loop
}

template<int ROWS,int COLS,int DEPTH,int NPC,int WORDWIDTH,int PIPELINEFLAG, int WIN_SZ, int WIN_SZ_SQ, bool USE_URAM = false>
void xFPyrDownGaussianBlur(
		hls::stream< XF_TNAME(DEPTH,NPC) > &_src,
		hls::stream< XF_TNAME(DEPTH,NPC) > &_dst, ap_uint<8> win_size,
		int _border_type,uint16_t imgheight,uint16_t imgwidth)
{
	assert(((imgheight <= ROWS ) && (imgwidth <= COLS)) && "ROWS and COLS should be greater than input image");
	
	assert( (win_size <= WIN_SZ) && "win_size must not be greater than WIN_SZ");

	imgwidth = imgwidth >> XF_BITSHIFT(NPC);

	xf_pyrdown_gaussian_nxn<ROWS,COLS,DEPTH,NPC,WORDWIDTH,(COLS>>XF_BITSHIFT(NPC))+(WIN_SZ>>1),WIN_SZ, WIN_SZ_SQ, USE_URAM>(_src, _dst,WIN_SZ,imgheight,imgwidth);

}

#endif
