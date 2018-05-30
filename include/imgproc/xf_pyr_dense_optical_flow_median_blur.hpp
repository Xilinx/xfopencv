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
#ifndef __XF_PYR_DENSE_OPTICAL_FLOW_MEDIAN_BLUR__
#define __XF_PYR_DENSE_OPTICAL_FLOW_MEDIAN_BLUR__
template<int NPC,int DEPTH, int WIN_SZ, int WIN_SZ_SQ, int FLOW_WIDTH, int FLOW_INT>
void auMedianProc(
		ap_fixed<FLOW_WIDTH,FLOW_INT> OutputValues[1],
		ap_fixed<FLOW_WIDTH,FLOW_INT> src_buf[WIN_SZ][1+(WIN_SZ-1)],
		ap_uint<8> win_size
		)
{
#pragma HLS INLINE

	ap_fixed<FLOW_WIDTH,FLOW_INT> array[WIN_SZ_SQ];
// #pragma HLS RESOURCE variable=array core=DSP48
#pragma HLS ARRAY_PARTITION variable=array complete dim=1
		
		
		int array_ptr=0;
		// OutputValues[0] = src_buf[WIN_SZ>>1][WIN_SZ>>1];
		// return;
		Compute_Grad_Loop:
		for(int copy_arr=0;copy_arr<WIN_SZ;copy_arr++)
		{
#pragma HLS LOOP_TRIPCOUNT min=WIN_SZ max=WIN_SZ
#pragma HLS UNROLL
			for (int copy_in=0; copy_in<WIN_SZ; copy_in++)
			{
#pragma HLS LOOP_TRIPCOUNT min=WIN_SZ max=WIN_SZ
#pragma HLS UNROLL
				array[array_ptr] = src_buf[copy_arr][copy_in];
				array_ptr++;
			}
		}
		// OutputValues[0] = array[(WIN_SZ_SQ)>>1];
		// return;
		
		
		auApplyMaskLoop:
		for(int16_t j = 0; j <=WIN_SZ_SQ-1; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=WIN_SZ max=WIN_SZ
			int16_t tmp = j & 0x0001;
			if(tmp == 0)
			{
				auSortLoop1:
				for(int i = 0; i <=((WIN_SZ_SQ>>1 )-1); i++)		// even sort
				{
#pragma HLS LOOP_TRIPCOUNT min=WIN_SZ max=WIN_SZ
	#pragma HLS unroll
					int c = (i*2);
					int c1 = (c + 1);

					if(array[c] < array[c1])
					{
						ap_fixed<FLOW_WIDTH,FLOW_INT> temp = array[c];
						array[c] = array[c1];
						array[c1] = temp;
					}
				}
			}

			else
			{
				auSortLoop2:
				for(int i = 0; i <=((WIN_SZ_SQ>>1 )-1); i++)			// odd sort WINDOW_SIZE_H>>1 -1
				{
#pragma HLS LOOP_TRIPCOUNT min=WIN_SZ max=WIN_SZ
	#pragma HLS unroll
					int c = (i*2);
					int c1 = (c + 1);
					int c2 = (c + 2);
					if(array[c1] < array[c2])
					{
						ap_fixed<FLOW_WIDTH,FLOW_INT> temp = array[c1];
						array[c1] = array[c2];
						array[c2] = temp;
					}
				}
			}
		}
		
		// OutputValues[0] = auapplymedian3x3<DEPTH, WIN_SZ>(array, WIN_SZ);
		OutputValues[0] = array[(WIN_SZ_SQ)>>1];
		return;
}

template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH, int TC, int WIN_SZ, int WIN_SZ_SQ, int FLOW_WIDTH, int FLOW_INT>
void ProcessMedian3x3(hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > & _src_mat,
		hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > & _out_mat, hls::stream< bool > &flag,
		ap_fixed<FLOW_WIDTH,FLOW_INT> buf[WIN_SZ][(COLS >> NPC)], ap_fixed<FLOW_WIDTH,FLOW_INT> src_buf[WIN_SZ][1+(WIN_SZ-1)], ap_fixed<FLOW_WIDTH,FLOW_INT> buf_cop[WIN_SZ],
		ap_fixed<FLOW_WIDTH,FLOW_INT> OutputValues[1],
		ap_fixed<FLOW_WIDTH,FLOW_INT> &P0, uint16_t img_width,  uint16_t img_height, uint16_t &shift_x,  ap_uint<13> row_ind[WIN_SZ], ap_uint<13> row, ap_uint<16> col, ap_uint<8> win_size)
{
#pragma HLS INLINE

	uint16_t npc = 1;

		for(int copy_buf_var=0;copy_buf_var<WIN_SZ;copy_buf_var++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=WIN_SZ
#pragma HLS UNROLL
			buf_cop[copy_buf_var] = buf[copy_buf_var][col];
		}

		if(row < img_height && col < img_width)
			 buf_cop[row_ind[win_size-1]] = _src_mat.read(); // Read data
		else buf_cop[row_ind[win_size-1]] = 0;

		buf[row_ind[win_size-1]][col] = buf_cop[row_ind[win_size-1]];

		
		// if(NPC == AU_NPPC8)
		// {
			// for(int extract_px=0;extract_px<win_size;extract_px++)
			// {
// #pragma HLS LOOP_TRIPCOUNT min=WIN_SZ max=WIN_SZ
				// auExtractPixels<NPC, WORDWIDTH, DEPTH>(&src_buf[extract_px][win_size-1], buf_cop[extract_px], 0);
			// }
		// }
		// else
		{
			for(int extract_px=0;extract_px<WIN_SZ;extract_px++)
			{
#pragma HLS LOOP_TRIPCOUNT min=WIN_SZ max=WIN_SZ
	#pragma HLS UNROLL
				if(col<img_width)
				{
                   if((row >(img_height-1)) && (extract_px>(win_size-1-(row-(img_height-1)))))
                        src_buf[extract_px][win_size-1] = buf_cop[(row_ind[win_size-1-(row-(img_height-1))])];
                   else src_buf[extract_px][win_size-1] = buf_cop[(row_ind[extract_px])];
				}
				else
				{
					src_buf[extract_px][win_size-1] = src_buf[extract_px][win_size-2];
				}
			}
		}


		auMedianProc<NPC, DEPTH, WIN_SZ, WIN_SZ_SQ, FLOW_WIDTH, FLOW_INT>(OutputValues,src_buf, win_size);
		if(col >= (win_size>>1))
		{
			// auPackPixels<NPC, WORDWIDTH, DEPTH>(&OutputValues[0], P0, 0, 1, shift_x);
			// shift_x = 0;
			// P0 = 0;
			// auPackPixels<NPC, WORDWIDTH, DEPTH>(&OutputValues[0], P0, 1, (npc-1), shift_x);
			if(flag.read())
			{
				_out_mat.write(OutputValues[0]);
			}
			else
			{
				_out_mat.write(src_buf[WIN_SZ>>1][WIN_SZ>>1]);
			}
		}
		
		for(int wrap_buf=0;wrap_buf<WIN_SZ;wrap_buf++)
		{
	#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min=WIN_SZ max=WIN_SZ
			for(int col_warp=0; col_warp<WIN_SZ-1;col_warp++)
			{
	#pragma HLS UNROLL
#pragma HLS LOOP_TRIPCOUNT min=WIN_SZ max=WIN_SZ
				if((col>=(img_width-1)-(win_size>>1))  && (wrap_buf >= win_size>>1) )
				{
					src_buf[wrap_buf][col_warp] = src_buf[win_size-1][col_warp];
				}
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
}



template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH, int TC,int WIN_SZ, int WIN_SZ_SQ, int FLOW_WIDTH, int FLOW_INT, bool USE_URAM>
void auMedian3x3(hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &_src_mat0,
                 hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &_out_mat0, hls::stream< bool > &flag0,
                 hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &_src_mat1,
                 hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &_out_mat1, hls::stream< bool > &flag1,
        ap_uint<8> win_size,
		uint16_t img_height, uint16_t img_width)
{
	ap_uint<13> row_ind[WIN_SZ];
#pragma HLS ARRAY_PARTITION variable=row_ind complete dim=1
	
	ap_uint<8> buf_size = 1 + (WIN_SZ-1);
	uint16_t shift_x = 0;
	ap_uint<16> row, col;


	ap_fixed<FLOW_WIDTH,FLOW_INT> OutputValues[2][1];
#pragma HLS ARRAY_PARTITION variable=OutputValues complete dim=1
#pragma HLS ARRAY_PARTITION variable=OutputValues complete dim=2

	ap_fixed<FLOW_WIDTH,FLOW_INT> buf_cop[2][WIN_SZ];
#pragma HLS ARRAY_PARTITION variable=buf_cop complete dim=1
#pragma HLS ARRAY_PARTITION variable=buf_cop complete dim=2
	

	ap_fixed<FLOW_WIDTH,FLOW_INT> src_buf[2][WIN_SZ][1+(WIN_SZ-1)];
#pragma HLS ARRAY_PARTITION variable=src_buf complete dim=1
#pragma HLS ARRAY_PARTITION variable=src_buf complete dim=2
#pragma HLS ARRAY_PARTITION variable=src_buf complete dim=3
// src_buf1 et al merged 
	ap_fixed<FLOW_WIDTH,FLOW_INT> P0;

	ap_fixed<FLOW_WIDTH,FLOW_INT> buf[2][WIN_SZ][(COLS >> NPC)];
#pragma HLS ARRAY_RESHAPE variable=buf complete dim=1
#pragma HLS ARRAY_RESHAPE variable=buf complete dim=2

if (USE_URAM) {	
#pragma HLS RESOURCE variable=buf core=XPM_MEMORY uram
}
else {
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
	#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
	#pragma HLS pipeline
	#pragma HLS LOOP_FLATTEN OFF
			buf[0][init_buf][col] = _src_mat0.read();
			buf[1][init_buf][col] = _src_mat1.read();
		}
	}
	
	//takes care of top borders
		for(col = 0; col < img_width; col++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=TC
			for(int init_buf=0; init_buf < WIN_SZ>>1;init_buf++)
			{
	#pragma HLS LOOP_TRIPCOUNT min=WIN_SZ max=WIN_SZ
	#pragma HLS UNROLL
				buf[0][init_buf][col] = buf[0][row_ind[win_size>>1]][col];
				buf[1][init_buf][col] = buf[1][row_ind[win_size>>1]][col];
			}
		}
	
	Row_Loop:
	for(row = (win_size>>1); row < img_height+(win_size>>1); row++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		
		// //initialize buffers to be sent for sorting
		// for(int init_src=0;init_src<(win_size>>1);init_src++)
		// {
			// for(int init_src1=0;init_src1<win_size;init_src1++)
			// {
	// #pragma HLS UNROLL
				// src_buf[init_src1][init_src] =  buf[row_ind[init_src1]][0];
			// }
		// }
		P0 = 0;
	    Col_Loop:
	    for(ap_uint<16> col = 0; col < img_width+(WIN_SZ>>1); col++)
	    {
#pragma HLS LOOP_TRIPCOUNT min=1 max=TC
#pragma HLS pipeline
#pragma HLS LOOP_FLATTEN OFF
		ProcessMedian3x3<ROWS, COLS, DEPTH, NPC, WORDWIDTH, TC, WIN_SZ, WIN_SZ_SQ, FLOW_WIDTH, FLOW_INT>(_src_mat0, _out_mat0, flag0, buf[0], src_buf[0], buf_cop[0], OutputValues[0], P0, img_width, img_height, shift_x, row_ind, row,col,win_size);
		ProcessMedian3x3<ROWS, COLS, DEPTH, NPC, WORDWIDTH, TC, WIN_SZ, WIN_SZ_SQ, FLOW_WIDTH, FLOW_INT>(_src_mat1, _out_mat1, flag1, buf[1], src_buf[1], buf_cop[1], OutputValues[1], P0, img_width, img_height, shift_x, row_ind, row,col,win_size);
	    } // Col_Loop
	
		//update indices
		ap_uint<13> zero_ind = row_ind[0];
		for(int init_row_ind=0; init_row_ind<WIN_SZ-1; init_row_ind++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=WIN_SZ max=WIN_SZ
	#pragma HLS UNROLL
			row_ind[init_row_ind] = row_ind[init_row_ind + 1];
		}
		row_ind[win_size-1] = zero_ind;
		
	} // Row_Loop
}

template<int ROWS,int COLS,int DEPTH,int NPC,int WORDWIDTH,int PIPELINEFLAG, int WIN_SZ, int WIN_SZ_SQ, int FLOW_WIDTH, int FLOW_INT, bool USE_URAM>
void auMedianBlur(
		hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &_src0,
		hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &_dst0, hls::stream< bool > &flag0,
		hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &_src1,
		hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &_dst1, hls::stream< bool > &flag1,
        ap_uint<8> win_size,
		int _border_type,uint16_t imgheight,uint16_t imgwidth)
{
#pragma HLS inline off

// #pragma HLS license key=IPAUVIZ_CV_BASIC
	// assert(_border_type == AU_BORDER_CONSTANT && "Only AU_BORDER_CONSTANT is supported");

	assert(((imgheight <= ROWS ) && (imgwidth <= COLS)) && "ROWS and COLS should be greater than input image");
	
	assert( (win_size <= WIN_SZ) && "win_size must not be greater than WIN_SZ");

	imgwidth = imgwidth >> NPC;


	auMedian3x3< ROWS, COLS, DEPTH, NPC, WORDWIDTH, (COLS>>NPC)+(WIN_SZ>>1), WIN_SZ, WIN_SZ_SQ, FLOW_WIDTH, FLOW_INT, USE_URAM>(_src0, _dst0,flag0,_src1, _dst1,flag1,WIN_SZ,imgheight,imgwidth);


}
#endif