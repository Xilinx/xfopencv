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
#ifndef __XF_WARP_TRANSFORM__
#define __XF_WARP_TRANSFORM__
#include "ap_int.h"
#include "hls_stream.h"
#include "assert.h"
#include "common/xf_common.h"

//Number of fractional bits used for interpolation
#define INTER_BITS 5
#define INTER_TAB_SIZE (1 << INTER_BITS)
#define INTER_SCALE 1.f / INTER_TAB_SIZE

#define AB_BITS std::max(10,INTER_BITS)
#define AB_SCALE (1 << AB_BITS)
//Number of bits used to linearly interpolate
#define INTER_REMAP_COEF_BITS 15
#define INTER_REMAP_COEF_SCALE (1 << INTER_REMAP_COEF_BITS)
#define ROUND_DELTA (1 << (AB_BITS - INTER_BITS - 1))
typedef float image_comp;

namespace xf{

//function to store the image in 4 memories of a combined size of (0:STORE_LINES-1,0:COLS-1)
template<int COLS, int STORE_LINES, int DEPTH, int NPC>
void store_EvOd_image1(XF_TNAME(DEPTH,NPC) in_pixel, ap_uint<16> i,ap_uint<16> j, XF_TNAME(DEPTH,NPC) store1_pt_2EvR_EvC[(STORE_LINES>>2)][COLS], XF_TNAME(DEPTH,NPC) store1_pt_2OdR_EvC[(STORE_LINES>>2)][COLS], XF_TNAME(DEPTH,NPC) store1_pt_2EvR_OdC[(STORE_LINES>>2)][COLS], XF_TNAME(DEPTH,NPC) store1_pt_2OdR_OdC[(STORE_LINES>>2)][COLS])
{
#pragma HLS INLINE
//	const int COLS = COLS;
//	const int STORE_LINES = Num_Store_Rows;
	//finding if i and j are even or odd to access the appropriate memory
	bool i_a1 = (i&0x00000002)>>1;
	bool i_a = i&0x00000001;
	bool j_a = j&0x00000001;
	ap_uint<16> I = ap_uint<16>(i>>2);
	ap_uint<16> J=0;
	if(i_a1)
	{
		J = ap_uint<16>(j>>1) + (COLS>>1);
	}
	else
	{
		J = (j>>1);
	}
	switch (j_a <<1 | i_a)
	{
	case 0:  store1_pt_2EvR_EvC[I][J] = in_pixel;
	break;
	case 1:  store1_pt_2OdR_EvC[I][J] = in_pixel;
	break;
	case 2:  store1_pt_2EvR_OdC[I][J] = in_pixel;
	break;
	case 3:  store1_pt_2OdR_OdC[I][J] = in_pixel;
	break;
	}
};

template<int COLS, int STORE_LINES, int DEPTH, int NPC>
XF_TNAME(DEPTH,NPC) retrieve_EvOd_image1(int i,int j, XF_TNAME(DEPTH,NPC) store1_pt_2EvR_EvC[(STORE_LINES>>2)][COLS], XF_TNAME(DEPTH,NPC) store1_pt_2OdR_EvC[(STORE_LINES>>2)][COLS], XF_TNAME(DEPTH,NPC) store1_pt_2EvR_OdC[(STORE_LINES>>2)][COLS], XF_TNAME(DEPTH,NPC) store1_pt_2OdR_OdC[(STORE_LINES>>2)][COLS])
{
#pragma HLS INLINE
//	const int COLS = COLS;
//	const int STORE_LINES = Num_Store_Rows;
	//finding if i and j are even or odd to access the appropriate memory
	XF_TNAME(DEPTH,NPC) return_val = 0;

	//temporary variables to compute the indices for memory access
	int I,J,temp1;

	//snapping the row number from 0:STORE_LINES-1 and wrapping around the indices
	// for i>STORE_LINES and i<0
	temp1 = i > (STORE_LINES - 1)? (i - STORE_LINES) : ((i < 0)? (i + STORE_LINES) : i);

	bool i_a1 = (temp1&0x00000002)>>1;
	bool i_a = temp1&0x00000001;
	bool j_a = j&0x00000001;
	I = temp1>>2;
	if(i_a1)
	{
		J = (j>>1) + (COLS>>1);
	}
	else
	{
		J = (j>>1);
	}

	switch (j_a <<1 | i_a)
	{
	case 0: return_val = store1_pt_2EvR_EvC[I][J];
	break;
	case 1: return_val = store1_pt_2OdR_EvC[I][J];
	break;
	case 2: return_val = store1_pt_2EvR_OdC[I][J];
	break;
	case 3: return_val = store1_pt_2OdR_OdC[I][J];
	break;
	}
	return return_val;
};
//function to store the image in 4 memories of a combined size of (0:STORE_LINES-1,0:COLS-1)
template<int COLS, int STORE_LINES, int DEPTH, int NPC>
XF_TNAME(DEPTH,NPC) retrieve_EvOd_image4x1(int i,int j,int A, int B, int C, int D, XF_TNAME(DEPTH,NPC) store1_pt_2EvR_EvC[(STORE_LINES>>2)][COLS], XF_TNAME(DEPTH,NPC) store1_pt_2OdR_EvC[(STORE_LINES>>2)][COLS], XF_TNAME(DEPTH,NPC) store1_pt_2EvR_OdC[(STORE_LINES>>2)][COLS], XF_TNAME(DEPTH,NPC) store1_pt_2OdR_OdC[(STORE_LINES>>2)][COLS])
{
#pragma HLS INLINE
//	const int COLS = COLS;
//	const int STORE_LINES = Num_Store_Rows;
	//inlin-ing the function to enable the tool to pipeline the memory accesses
	//finding the LSB of i and j to find even/odd conditions for memory reads

	//output value computed in fixed point (17,15)
	int op_val = 0;
	//temporary variables to compute the indices for 4 memories for linear interpolation
	//computing i/2 (i+1)/2 j/2 and (j+1)/2 to access the 4 memories
	int I,J,I1,J1,temp1,temp2,Ja,Ja1;

	//snapping the row number from 0:STORE_LINES-1 and wrapping around the indices
	// for i>STORE_LINES and i<0
	temp1 = (i > (STORE_LINES - 1))? (i - STORE_LINES) : ((i < 0)? (i + STORE_LINES) : i);

	//snapping the row number from 0:STORE_LINES-1 and wrapping around the indices
	// for i+1>STORE_LINES and i+1<0
	temp2 = ((i+1)>(STORE_LINES - 1))? (i+1 - STORE_LINES) : ((i+1 < 0)? (i+1 + STORE_LINES) : i+1);
	//dividing the indices by 2 to get the indices for the 4 memories
	XF_TNAME(DEPTH,NPC) px00=0,px01=0,px10=0,px11=0;

	ap_uint<2> i_a1 = (temp1&0x00000003);
	ap_uint<2> i_a2 = (temp2&0x00000003);
	bool i_a = i&0x00000001;
	bool j_a = j&0x00000001;
	I = temp1>>2;
	I1 = temp2>>2;

	J = j>>1;
	J1 = (j+1)>>1;
	Ja = (j>>1) + (COLS>>1);
	Ja1 = ((j+1)>>1) + (COLS>>1);
	int EvR_EvC_colAddr, EvR_OdC_colAddr, OdR_EvC_colAddr, OdR_OdC_colAddr;
	int EvR_rowAddr, OdR_rowAddr;
	
	if      ((i_a1==0) && (j_a==0)) {
	    EvR_EvC_colAddr = J;
	    EvR_OdC_colAddr = J1;
	    OdR_EvC_colAddr = J;
	    OdR_OdC_colAddr = J1;
	}
	else if ((i_a1==0) && (j_a==1)) {
	    EvR_EvC_colAddr = J1;
	    EvR_OdC_colAddr = J;
	    OdR_EvC_colAddr = J1;
	    OdR_OdC_colAddr = J;
	}
	else if ((i_a1==1) && (j_a==0)) {
	    EvR_EvC_colAddr = Ja;
	    EvR_OdC_colAddr = Ja1;
	    OdR_EvC_colAddr = J;
	    OdR_OdC_colAddr = J1;
	}
	else if ((i_a1==1) && (j_a==1)) {
	    EvR_EvC_colAddr = Ja1;
	    EvR_OdC_colAddr = Ja;
	    OdR_EvC_colAddr = J1;
	    OdR_OdC_colAddr = J;
	}
	else if ((i_a1==2) && (j_a==0)) {
	    EvR_EvC_colAddr = Ja;
	    EvR_OdC_colAddr = Ja1;
	    OdR_EvC_colAddr = Ja;
	    OdR_OdC_colAddr = Ja1;
	}
	else if ((i_a1==2) && (j_a==1)) {
	    EvR_EvC_colAddr = Ja1;
	    EvR_OdC_colAddr = Ja;
	    OdR_EvC_colAddr = Ja1;
	    OdR_OdC_colAddr = Ja;
	}
	else if ((i_a1==3) && (j_a==0)) {
	    EvR_EvC_colAddr = J;
	    EvR_OdC_colAddr = J1;
	    OdR_EvC_colAddr = Ja;
	    OdR_OdC_colAddr = Ja1;
	}
	else {
	    EvR_EvC_colAddr = J1;
	    EvR_OdC_colAddr = J;
	    OdR_EvC_colAddr = Ja1;
	    OdR_OdC_colAddr = Ja;
	}
	
	if ((i_a1==0) || (i_a1==2)) {
	    EvR_rowAddr = I;
	    OdR_rowAddr = I1;
	}
	else {
	    EvR_rowAddr = I1;
	    OdR_rowAddr = I;
	}
	XF_TNAME(DEPTH,NPC) pop_bram_EvR_EvC = store1_pt_2EvR_EvC[EvR_rowAddr][EvR_EvC_colAddr];
	XF_TNAME(DEPTH,NPC) pop_bram_EvR_OdC = store1_pt_2EvR_OdC[EvR_rowAddr][EvR_OdC_colAddr];
	XF_TNAME(DEPTH,NPC) pop_bram_OdR_EvC = store1_pt_2OdR_EvC[OdR_rowAddr][OdR_EvC_colAddr];
	XF_TNAME(DEPTH,NPC) pop_bram_OdR_OdC = store1_pt_2OdR_OdC[OdR_rowAddr][OdR_OdC_colAddr];
	
	if (((i_a1==0)||(i_a1==2))) {
	    if (j_a==0) {
		    px00 = pop_bram_EvR_EvC;
		    px01 = pop_bram_EvR_OdC;
		    px10 = pop_bram_OdR_EvC;
		    px11 = pop_bram_OdR_OdC;
		}
		else {
		    px00 = pop_bram_EvR_OdC;
		    px01 = pop_bram_EvR_EvC;
		    px10 = pop_bram_OdR_OdC;
		    px11 = pop_bram_OdR_EvC;
        }		
	}
	else {
	    if (j_a==0) {
		    px00 = pop_bram_OdR_EvC;
		    px01 = pop_bram_OdR_OdC;
		    px10 = pop_bram_EvR_EvC;
		    px11 = pop_bram_EvR_OdC;
        }		
        else {
		    px00 = pop_bram_OdR_OdC;
		    px01 = pop_bram_OdR_EvC;
		    px10 = pop_bram_EvR_OdC;
		    px11 = pop_bram_EvR_EvC;
        }		
    }        	    
		op_val  = (A*px00);
		op_val += (B*px01);
		op_val += (C*px10);
		op_val += (D*px11);
//returning the computed interpolated output after rounding off the op_val by adding 0.5
//and shifting to right by INTER_REMAP_COEF_BITS
	return XF_TNAME(DEPTH,NPC)((op_val+(1<<(INTER_REMAP_COEF_BITS-1)))>>INTER_REMAP_COEF_BITS);
};

template <int NPC, int ROWS, int COLS, int DEPTH, int STORE_LINES, int START_ROW, int TRANSFORM, bool INTERPOLATION_TYPE>
int xFwarpTransformKernel(hls::stream< XF_TNAME(DEPTH,NPC) > &input_image, hls::stream< XF_TNAME(DEPTH,NPC) > &output_image, float P_matrix[9], short img_rows, short img_cols)
{
#pragma HLS INLINE
	//dividing memory (0:STORE_LINES-1,0:COLS-1) to ensure that the same memory
	//is not accesses more than once per iteration
	//removing intra and inter read dependencies between the reads and writes of memories
	//declaring them as true port brams
	assert(((img_rows <= ROWS ) && (img_cols <= COLS)) && "ROWS and COLS should be greater than input image");
	if(TRANSFORM==0)
	{
		assert(((P_matrix[6] == 0)&&(P_matrix[7] == 0)&&(P_matrix[8] == 0)) && "Third row of the transformation matrix must be 0s for Affine");
	}
	XF_TNAME(DEPTH,NPC) store1_pt_2EvR_EvC[(STORE_LINES>>2)][COLS];
	XF_TNAME(DEPTH,NPC) store1_pt_2EvR_OdC[(STORE_LINES>>2)][COLS];
	XF_TNAME(DEPTH,NPC) store1_pt_2OdR_EvC[(STORE_LINES>>2)][COLS];
	XF_TNAME(DEPTH,NPC) store1_pt_2OdR_OdC[(STORE_LINES>>2)][COLS];
#pragma HLS ARRAY_PARTITION variable=store1_pt_2EvR_EvC complete dim=1
#pragma HLS ARRAY_PARTITION variable=store1_pt_2EvR_OdC complete dim=1
#pragma HLS ARRAY_PARTITION variable=store1_pt_2OdR_EvC complete dim=1
#pragma HLS ARRAY_PARTITION variable=store1_pt_2OdR_OdC complete dim=1
#pragma HLS RESOURCE variable=store1_pt_2EvR_EvC core=RAM_T2P_BRAM
#pragma HLS RESOURCE variable=store1_pt_2EvR_OdC core=RAM_T2P_BRAM
#pragma HLS RESOURCE variable=store1_pt_2OdR_EvC core=RAM_T2P_BRAM
#pragma HLS RESOURCE variable=store1_pt_2OdR_OdC core=RAM_T2P_BRAM
#pragma HLS DEPENDENCE variable=store1_pt_2EvR_EvC inter false
#pragma HLS DEPENDENCE variable=store1_pt_2EvR_OdC inter false
#pragma HLS DEPENDENCE variable=store1_pt_2OdR_EvC inter false
#pragma HLS DEPENDENCE variable=store1_pt_2OdR_OdC inter false
#pragma HLS DEPENDENCE variable=store1_pt_2EvR_EvC intra false
#pragma HLS DEPENDENCE variable=store1_pt_2EvR_OdC intra false
#pragma HLS DEPENDENCE variable=store1_pt_2OdR_EvC intra false
#pragma HLS DEPENDENCE variable=store1_pt_2OdR_OdC intra false


	//varables for loop counters
	ap_uint<16> i=0,j=0,k=0,l=0,m=0,n=0,p=0;

	//copying transformation matrix to a local variable
	float R[3][3];
#pragma HLS ARRAY_PARTITION variable=R complete dim=1
#pragma HLS ARRAY_PARTITION variable=R complete dim=2
	COPY_MAT1:for(i=0;i<3;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=3 max=3 avg=3
		COPY_MAT2:for(j=0;j<3;j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=3 max=3 avg=3
			R[i][j] = float(P_matrix[i*3+j]);
		}
	}
	//output of the transformation matrix multiplication
	//partitioning the array to avoid memory read/write operations
	image_comp output_vec[3];
	#pragma HLS ARRAY_PARTITION variable=output_vec complete dim=1

	//variables for image indices, they can be negative
	//and can go out of bounds of the input image indices
	int I=0,J=0,I1=0;

	//variables to compute the transformations
	int X=0,Y=0,X_t=0,Y_t=0,X_t1=0,Y_t1=0,round_delta=0;

	//variables to compute the interpolation weights
	int A=0,B=0,C=0,D=0;

	//variable to store the output of the memory read/interpolation
	//to avoid multiple function calls in conditional statements
	XF_TNAME(DEPTH,NPC) output_value = 0;

	//op_val for storing the interpolated pixel value.
	//a, b for finding the fractional pixel values
	XF_TNAME(DEPTH,NPC) op_val=0;
	
	//main loop
	MAIN_ROWS:for (i=0;i<(img_rows + START_ROW);i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		MAIN_COLS:for(j=0;j<(img_cols);j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS
		#pragma HLS PIPELINE
		#pragma HLS LOOP_FLATTEN off

			//condition to store the input image in the image buffers
			//only until the number of rows are streamed in
			if(i<img_rows)
			{
				//compute n to wrap the buffer writes
				//to 0 once STORE_LINES rows are filled
				n = i-l;
				if(n >= STORE_LINES)
				{
					l = l + STORE_LINES;
				}
				//function to store the input image stream to
				//a buffer of size STORE_LINES rows
				//computing i-l to snap the writes to STORE_LINES size buffer
				store_EvOd_image1<COLS,STORE_LINES,DEPTH,NPC>( input_image.read() ,i-l,j, store1_pt_2EvR_EvC, store1_pt_2EvR_OdC, store1_pt_2OdR_EvC, store1_pt_2OdR_OdC);
			}

			//condition to compute and stream out the output image
			//after START_ROW number of rows
			if(i>=START_ROW)
			{
				//computing k from i to index the output image from 0
				k = i - (START_ROW);

				if (TRANSFORM==1)
				{
					//transforming the output coordinate (kth row, jth column)
					//to find the input coordinate using the transform matrix R
					//destination X coordinate is output_vec[0][0]
					//destination Y coordinate is output_vec[0][1]
					//destination Z coordinate is output_vec[0][2]
					output_vec[0] = image_comp(R[0][0])*(j) + image_comp(R[0][1])*(k) + image_comp(R[0][2]);
					output_vec[1] = image_comp(R[1][0])*(j) + image_comp(R[1][1])*(k) + image_comp(R[1][2]);
					output_vec[2] = image_comp(R[2][0])*(j) + image_comp(R[2][1])*(k) + image_comp(R[2][2]);

					//find the inverse of the Z element of the transform
					//and make it 0 if the z element is 0 to evade divide by 0
					if(INTERPOLATION_TYPE==0)
					{
						output_vec[2] = (output_vec[2] != 0.0f)? (1.0f/output_vec[2]) : 0.0f;
					}
					else
					{
						output_vec[2] = (output_vec[2] != 0.0f)? (INTER_TAB_SIZE/output_vec[2]) : 0.0f;
					}
					//find the X and Y indices of the destination by dividing
					//with the Z element of the transform
					X = round( output_vec[0]*output_vec[2] );
					Y = round( output_vec[1]*output_vec[2] );
				}
				else
				{
					if(INTERPOLATION_TYPE==0)
					{
						X_t  = round(image_comp(R[0][0])*((unsigned short)(j)<<(AB_BITS)));
						X_t1 = round((image_comp(R[0][1])*k + image_comp(R[0][2]))*AB_SCALE);
						Y_t  = round(image_comp(R[1][0])*((unsigned short)(j)<<(AB_BITS)));
						Y_t1 = round((image_comp(R[1][1])*k + image_comp(R[1][2]))*AB_SCALE);
						round_delta = (AB_SCALE >> 1);
						X = X_t + X_t1 + round_delta;
						Y = Y_t + Y_t1 + round_delta;
						X     = round(X >> (AB_BITS));
						Y     = round(Y >> (AB_BITS));
					}
					else
					{
						X_t  = round(image_comp(R[0][0])*(int(j)<<(AB_BITS)));
						X_t1 = round((image_comp(R[0][1])*k + image_comp(R[0][2]))*AB_SCALE);
						Y_t  = round(image_comp(R[1][0])*(int(j)<<(AB_BITS)));
						Y_t1 = round((image_comp(R[1][1])*k + image_comp(R[1][2]))*AB_SCALE);
						X = X_t + X_t1 + ROUND_DELTA;
						X     = X >> (AB_BITS - INTER_BITS);
						Y = Y_t + Y_t1 + ROUND_DELTA;
						Y     = Y >> (AB_BITS - INTER_BITS);
					}
				}

				if(INTERPOLATION_TYPE==0)
				{
					I = Y;
					J = X;
				}
				else
				{
					//finding the integer part by shifting to the right
					//by the number of fractional bits
					I = Y>>INTER_BITS;
					J = X>>INTER_BITS;

					//finding the fractional part of the indices in fixed point
					short a = Y & (INTER_TAB_SIZE - 1);
					short b = X & (INTER_TAB_SIZE - 1);

					//finding the fractional part of the indices in floating point
					float taby = (float(INTER_REMAP_COEF_SCALE)/INTER_TAB_SIZE)*a;
					float tabx = (1.0f/INTER_TAB_SIZE)*b;

					//finding the coefficients to multiply with the 4 pixels for interpolation
					//(I,J)*A + (I,J+1)*B + (I+1,J)*C, (I+1,J+1)*D
					//converting to fixed point with 15 fractional bits
					A =  floor((float(INTER_REMAP_COEF_SCALE)-taby)*(1.0f-tabx));
					B =  floor((float(INTER_REMAP_COEF_SCALE)-taby)*tabx);
					C =  floor(taby*(1.0f-tabx));
					D =  floor(taby*tabx);
				}

				//compute k-m to wrap the buffer reads
				//to 0 once N_Store rows are reads
				if(k-m >= STORE_LINES)
				{
					m = m + STORE_LINES;
				}

				//calling the read function with interpolation
				if((J >=0 ) && (J < img_cols-int(INTERPOLATION_TYPE)) && (I >= 0) && (I < img_rows-int(INTERPOLATION_TYPE)))
				{
					//computing the row index for the stored STORE_LINES rows
					I1 = I - m;
					if(INTERPOLATION_TYPE==0)
					{
						op_val = retrieve_EvOd_image1<COLS,STORE_LINES,DEPTH,NPC>(I1,J, store1_pt_2EvR_EvC, store1_pt_2EvR_OdC, store1_pt_2OdR_EvC, store1_pt_2OdR_OdC);
					}
					else
					{
						//calling the read function with interpolation
						op_val = retrieve_EvOd_image4x1<COLS,STORE_LINES,DEPTH,NPC>(I1,J,A,B,C,D, store1_pt_2EvR_EvC, store1_pt_2EvR_OdC, store1_pt_2OdR_EvC, store1_pt_2OdR_OdC);
					}
				}
				else
				{
					//change this to an input if the border
					op_val = 0;
				}
				//streaming out the computed output value and incrementing the out address pointer
				output_image.write(op_val);
			}
		}
	}

return 0;
};

#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"])
#pragma SDS data copy("_dst_mat.data"[0:"_dst_mat.size"])
//#pragma SDS data data_mover("_src_mat.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst_mat.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst_mat.data":SEQUENTIAL)
#pragma SDS data mem_attribute ("_src_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS, "_dst_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
template <int STORE_LINES, int START_ROW, int TRANSFORM, bool INTERPOLATION_TYPE, int TYPE, int ROWS, int COLS, int NPC>
void warpTransform(xf::Mat<TYPE,ROWS,COLS,NPC> & _src_mat, xf::Mat<TYPE,ROWS,COLS,NPC> & _dst_mat, float P_matrix[9])
{
	#pragma HLS INLINE OFF
	#pragma HLS DATAFLOW
hls::stream< XF_TNAME(TYPE,NPC) > in_stream;
hls::stream< XF_TNAME(TYPE,NPC) > out_stream;

	for(int i=0; i<_src_mat.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src_mat.cols)>>XF_BITSHIFT(NPC);j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			in_stream.write( *(_src_mat.data + i*(_src_mat.cols>>XF_BITSHIFT(NPC)) +j) );
		}
	}

xFwarpTransformKernel<NPC, ROWS, COLS, TYPE, STORE_LINES, START_ROW, TRANSFORM, INTERPOLATION_TYPE>(in_stream, out_stream, P_matrix, _src_mat.rows, _src_mat.cols);

	for(int i=0; i<_dst_mat.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst_mat.cols)>>XF_BITSHIFT(NPC);j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			*(_dst_mat.data + i*(_dst_mat.cols>>XF_BITSHIFT(NPC)) +j) = out_stream.read();
		}
	}
}
}

#endif
