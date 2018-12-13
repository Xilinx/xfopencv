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


#ifndef _XF_WARPPERSPECTIVE_HPP_
#define _XF_WARPPERSPECTIVE_HPP_


#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "core/xf_math.h"

#define TWO_POW_32		4294967296

#define INPUT_BLOCK_LENGTH 		256

#define BLOCKSIZE			256
#define BLOCKSIZE_BY8		(BLOCKSIZE>>3)
#define SUBBLOCKSIZE		128
#define SUBBLOCKSIZE_BY8	(SUBBLOCKSIZE>>3)

#define HBLOCKS				2
#define VBLOCKS 			2
#define nBLOCKS				(HBLOCKS*VBLOCKS)
#define BUFSIZE				(BLOCKSIZE_BY8*BLOCKSIZE/nBLOCKS)
#define BRAMSIZE			(SUBBLOCKSIZE*((SUBBLOCKSIZE>>3)+1)/HBLOCKS)


namespace xf{

/*
 * Finding the maximum and minimum of the four input values
 */
static void xFFindMaxMin(int16_t in1,int16_t in2,int16_t in3,int16_t in4,int16_t &max,int16_t &min)
{
#pragma HLS inline off
	int16_t tempmax0,tempmax1,tempmin0,tempmin1;
	if(in1 > in2)
	{
		tempmax0 = in1;
		tempmin0 = in2;
	}
	else
	{
		tempmax0 = in2;
		tempmin0 = in1;
	}

	if(in3 > in4)
	{
		tempmax1 = in3;
		tempmin1 = in4;
	}
	else
	{
		tempmax1 = in4;
		tempmin1 = in3;
	}

	if(tempmax0 > tempmax1)
		max = tempmax0;
	else
		max = tempmax1;

	if(tempmin0 < tempmin1)
		min = tempmin0;
	else
		min = tempmin1;
}

/*
 * Calculating the dominant bits in the Q32.32 variable
 * To facilitate 16bit division of any Qformat
 */
static ap_uint8_t xFPerspectiveDominantBits(ap_int64_t Val)
{
	ap_uint8_t cnt=0;
	ap_uint1_t pres_val,prev_val = Val.range(63,63);

	for(ap_uint<6> i=62;i>31;i--)
	{
#pragma HLS PIPELINE
		pres_val = Val.range(i,i);
		if(pres_val !=  prev_val)
			break;
		cnt++;
	}
	return cnt;
}



/*
 * Inverse Perspective Transform
 *
 *		x = xout*M[0] + yout*M[1] + M[2]
 *		y = xout*M[3] + yout*M[4] + M[5]
 *		z = xout*M[6] + yout*M[7] + M[8]
 *
 *		xin = x/z;
 *		yin = y/z;
 */


template<int WORDWIDTH_T>
void xFPerspectiveTransform(int16_t x_out, int16_t y_out,XF_PTNAME(WORDWIDTH_T) A,XF_PTNAME(WORDWIDTH_T) B,XF_PTNAME(WORDWIDTH_T) C,
		XF_PTNAME(WORDWIDTH_T) D,XF_PTNAME(WORDWIDTH_T) E,XF_PTNAME(WORDWIDTH_T) F,XF_PTNAME(WORDWIDTH_T) G,XF_PTNAME(WORDWIDTH_T) H,
		XF_PTNAME(WORDWIDTH_T) I,ap_int64_t *x_frac,ap_int64_t *y_frac, int16_t *x_in, int16_t *y_in)
{
#pragma HLS INLINE off
	ap_int64_t Ax_out,By_out,Dx_out,Ey_out,Gx_out,Hy_out;
	ap_int48_t frac_x_h,frac_y_h;
	ap_int64_t temp_x_h,temp_y_h;
	ap_int64_t frac_z_h;
	int16_t z_h;
	uchar_t Nbits ;
	int offsetX=0,offsetY=0;
	char N;
	uint16_t temp_z_hnew;
	uint16_t temp_z_h;
	int16_t z_hnew=0;

	//	Ax_out = ((ap_int64_t)A*x_out + (ap_int64_t)C);			// A - Q16.32	x_out Q16.0
	//	By_out = ((ap_int64_t)B*y_out);							// B - Q16.32	y_out Q16.0
	//	Dx_out = ((ap_int64_t)D*x_out + (ap_int64_t)F);			// D - Q16.32	F - Q16.32
	//	Ey_out = ((ap_int64_t)E*y_out);							// E - Q16.32
	//	Gx_out = ((ap_int64_t)G*x_out + (ap_int64_t)I);			// G - Q16.32	I - Q16.32
	//	Hy_out = ((ap_int64_t)H*y_out);							// H - Q16.32
	//
	//	temp_x_h = (ap_int64_t)((Ax_out + By_out));
	//	temp_y_h = (ap_int64_t)((Dx_out + Ey_out));
	//	frac_z_h = (ap_int64_t)(Gx_out + Hy_out);				// frac_z_h - Q32.32

	temp_x_h = ((ap_int64_t)A*(ap_int64_t)x_out + (ap_int64_t)C) + ((ap_int64_t)B*(ap_int64_t)y_out);
	temp_y_h = ((ap_int64_t)D*(ap_int64_t)x_out + (ap_int64_t)F) + ((ap_int64_t)E*(ap_int64_t)y_out);
	frac_z_h = ((ap_int64_t)G*(ap_int64_t)x_out + (ap_int64_t)I) + ((ap_int64_t)H*(ap_int64_t)y_out);

	frac_x_h = (temp_x_h)>>16;								//	(temp_x_h)Q32.32 >> 16 --> (frac_x_h)Q32.16
	frac_y_h = (temp_y_h)>>16;								//	(temp_y_h)Q32.32 >> 16 --> (frac_y_h)Q32.16

	Nbits = xFPerspectiveDominantBits(frac_z_h);						// Calculating the dominant bits to decrease the bitwidth required for division
	uchar_t integerbits = (Nbits)<16?16:(32-Nbits);			// number of integer bits in frac_z_h
	z_h = (int16_t)((frac_z_h)>>(48-Nbits));

	if(frac_z_h < 0)
		temp_z_h = -z_h;
	else
		temp_z_h = z_h;


	if(temp_z_h != 0)
		temp_z_hnew = xf::Inverse(temp_z_h,integerbits,&N); 	// calculating 1/temp_z_h
	else
		temp_z_hnew = 0;



	if(N > 12)
	{
		if(temp_z_hnew==65535)
		{
			temp_z_hnew = (uint16_t)(temp_z_hnew>>(N-12));
			temp_z_hnew+=1;
		}
		else
			temp_z_hnew = (uint16_t)(temp_z_hnew>>(N-12));
	}
	else
		temp_z_hnew = (uint16_t)(temp_z_hnew<<(12-N));

	if(frac_z_h < 0)
		z_hnew = -temp_z_hnew;
	else
		z_hnew = temp_z_hnew;

	ap_int64_t tempvalue1 = (ap_int64_t)(frac_x_h<<4);
	ap_int64_t  tempvalue2 = (ap_int64_t)(frac_y_h<<4);

	x_frac[0] = (ap_int64_t)(tempvalue1*(ap_int64_t)z_hnew);
	y_frac[0] = (ap_int64_t)(tempvalue2*(ap_int64_t)z_hnew);


	if((x_frac[0] & 0xFFFFFFFF) >= 0x80000000)		// Rounding the x_frac value, if fractional part is >0.5
		offsetX = 1;
	if((y_frac[0] & 0xFFFFFFFF) >= 0x80000000)      // Rounding the y_frac value, if fractional part is >0.5
		offsetY = 1;

	*x_in = ((x_frac[0])>>32) + offsetX;
	*y_in = ((y_frac[0])>>32) + offsetY;


}


/*
 * 	Calculating the Maximum output size that can be caluculated
 * 	using a 256*256 input block
 */

template<int WORDWIDTH_T>
int xFPerspectiveFindMaxOutSize(XF_PTNAME(WORDWIDTH_T) *transform_matrix, int x_offset, int y_offset,ap_uint<13> img_height,ap_uint<13> img_width){

	int16_t max_output_range=0,max_output_range_mul16=0;
	int16_t x_min, y_min, x_max, y_max;
	XF_PTNAME(WORDWIDTH_T) A, B, C, D, E, F, G, H, I;
	ap_int64_t x_fixed,y_fixed;

	int16_t x_out[4],y_out[4],x_in[4],y_in[4];
#pragma HLS ARRAY_PARTITION variable=x_out complete dim=1
#pragma HLS ARRAY_PARTITION variable=y_out complete dim=1
#pragma HLS ARRAY_PARTITION variable=x_in complete dim=1
#pragma HLS ARRAY_PARTITION variable=y_in complete dim=1

	A = transform_matrix[0];
	B = transform_matrix[1];
	C = transform_matrix[2];
	D = transform_matrix[3];
	E = transform_matrix[4];
	F = transform_matrix[5];
	G = transform_matrix[6];
	H = transform_matrix[7];
	I = transform_matrix[8];

	int16_t xindex[4],yindex[4],outputranges[4],min_output_range=256;

	xindex[0] = 128; 			yindex[0] = 128;			// Center of Top Left 256x256 block of the image
	xindex[1] = img_width - 128; 	yindex[1] = 128;			// Center of Top Right 256x256 block of the image
	xindex[2] = 128; 			yindex[2] = img_height - 128;	// Center of Bottom Left 256x256 block of the image
	xindex[3] = img_width - 128; 	yindex[3] = img_height - 128;	// Center of Bottom Right 256x256 block of the image

	/*
	 * Calculating the maxblock size at the four corners of the image
	 * to counter the difference in the scaling across the image.
	 */

	for(ap_uint<3> i=0;i<4;i++)
	{
		max_output_range = 0;
		MAX_OUT_LOOP:
		for(ap_int<9> p=128; p>=0; p=p-4)
		{
			x_out[0] = xindex[i] - p; x_out[1] = xindex[i] + p;
			x_out[2] = xindex[i] - p; x_out[3] = xindex[i] + p;

			y_out[0] = yindex[i] - p; y_out[1] = yindex[i] - p;
			y_out[2] = yindex[i] + p; y_out[3] = yindex[i] + p;

			for(ap_uint<3> k=0;k<4;k++)
			{
#pragma HLS PIPELINE
				xFPerspectiveTransform<WORDWIDTH_T>(x_out[k], y_out[k], A, B, C, D, E, F, G, H, I,&x_fixed,&y_fixed, &x_in[k], &y_in[k]);
			}
			xFFindMaxMin(x_in[0],x_in[1],x_in[2],x_in[3],x_max,x_min);
			xFFindMaxMin(y_in[0],y_in[1],y_in[2],y_in[3],y_max,y_min);

			if(((y_max-y_min) <= INPUT_BLOCK_LENGTH) && ((x_max-x_min) <= INPUT_BLOCK_LENGTH))
			{
				if(max_output_range < (p<<1))
					max_output_range = (p<<1);
			}
		}
		outputranges[i] = max_output_range;
		min_output_range = min_output_range>max_output_range?max_output_range:min_output_range;
	}

	// maximum output range should be made multiple of 16
	// as we are further dividing the block into 2x2 subblocks
	max_output_range_mul16 = (min_output_range-8);
	max_output_range_mul16 = ((max_output_range_mul16>>4)<<4);

	return max_output_range_mul16;
}

/*
 *	Traversing through the top edge to calculate the
 *	Minimum x position from where the input is to be read
 */

template<int ROWS, int COLS, int WORDWIDTH_T>
void xFPerspectiveFindTopMin(int16_t x_index,int16_t y_index,XF_PTNAME(WORDWIDTH_T) A,XF_PTNAME(WORDWIDTH_T) B,XF_PTNAME(WORDWIDTH_T) C,
		XF_PTNAME(WORDWIDTH_T) D,XF_PTNAME(WORDWIDTH_T) E,XF_PTNAME(WORDWIDTH_T) F,XF_PTNAME(WORDWIDTH_T) G,
		XF_PTNAME(WORDWIDTH_T) H,XF_PTNAME(WORDWIDTH_T) I,ap_uint<9> max_output_range,
		int16_t &x_min,int16_t &y_min, ap_uint<13> img_height, ap_uint<13> img_width)
{
#pragma HLS INLINE off
	int16_t x_in,y_in,x_mintemp,y_mintemp,y_out=0;
	ap_int<64> x_fixed,y_fixed;
	ap_uint<9> x_out=0;
	x_mintemp = img_width;
	y_mintemp = img_height;
	for(x_out=0;x_out<max_output_range;x_out++)
	{
#pragma HLS loop_tripcount  min=108 max=176 avg=176
#pragma HLS PIPELINE
		xFPerspectiveTransform<WORDWIDTH_T>((x_out+x_index), (y_out+y_index), A, B, C, D, E, F, G, H, I,&x_fixed,&y_fixed, &x_in, &y_in);
		if((x_in>=0) && (x_in<x_mintemp))
		{
			x_mintemp = x_in;
		}

		if((y_in>=0) && (y_in<y_mintemp))
		{
			y_mintemp = y_in;
		}
	}
	x_min = x_mintemp;
	y_min = y_mintemp;
}

/*
 *	Traversing through the Bottom edge to calculate the
 *	Minimum x position from where the input is to be read
 */
template<int ROWS, int COLS, int WORDWIDTH_T>
void xFPerspectiveFindBottomMin(int16_t x_index,int16_t y_index,XF_PTNAME(WORDWIDTH_T) A,XF_PTNAME(WORDWIDTH_T) B,XF_PTNAME(WORDWIDTH_T) C,
		XF_PTNAME(WORDWIDTH_T) D,XF_PTNAME(WORDWIDTH_T) E,XF_PTNAME(WORDWIDTH_T) F,XF_PTNAME(WORDWIDTH_T) G,
		XF_PTNAME(WORDWIDTH_T) H,XF_PTNAME(WORDWIDTH_T) I,ap_uint<9> max_output_range,
		int16_t &x_min,int16_t &y_min, ap_uint<13> img_height, ap_uint<13> img_width)
{
#pragma HLS INLINE off
	int16_t x_in,y_in,x_mintemp,y_mintemp;
	ap_uint<9> y_out2=max_output_range-1,x_out2=0;
	ap_int<64> x_fixed,y_fixed;

	x_mintemp = img_width;
	y_mintemp = img_height;
	for(x_out2=0;x_out2<max_output_range;x_out2++)
	{
#pragma HLS loop_tripcount  min=108 max=176 avg=176
#pragma HLS pipeline
		xFPerspectiveTransform<WORDWIDTH_T>((x_out2+x_index), (y_out2+y_index), A, B, C, D, E, F, G, H, I,&x_fixed,&y_fixed, &x_in, &y_in);
		if((x_in>=0) && (x_in<x_mintemp))
		{
			x_mintemp = x_in;
		}

		if((y_in>=0) && (y_in<y_mintemp))
		{
			y_mintemp = y_in;
		}
	}
	x_min = x_mintemp;
	y_min = y_mintemp;
}

/*
 *	Traversing through the Left edge to calculate the
 *	Minimum x position from where the input is to be read
 */
template<int ROWS, int COLS, int WORDWIDTH_T>
void xFPerspectiveFindLeftMin(int16_t x_index,int16_t y_index,XF_PTNAME(WORDWIDTH_T) A,XF_PTNAME(WORDWIDTH_T) B,XF_PTNAME(WORDWIDTH_T) C,
		XF_PTNAME(WORDWIDTH_T) D,XF_PTNAME(WORDWIDTH_T) E,XF_PTNAME(WORDWIDTH_T) F,XF_PTNAME(WORDWIDTH_T) G,
		XF_PTNAME(WORDWIDTH_T) H,XF_PTNAME(WORDWIDTH_T) I,ap_uint<9> max_output_range,
		int16_t &x_min,int16_t &y_min, ap_uint<13> img_height, ap_uint<13> img_width)
{
#pragma HLS INLINE off
	int16_t x_in,y_in,x_mintemp,y_mintemp;
	ap_uint<9> x_out=0,y_out=0;
	ap_int<64> x_fixed,y_fixed;

	x_mintemp = img_width;
	y_mintemp = img_height;
	for( y_out=0;y_out<max_output_range;y_out++)
	{
#pragma HLS loop_tripcount  min=108 max=176 avg=176
#pragma HLS pipeline
		xFPerspectiveTransform<WORDWIDTH_T>((x_out+x_index), (y_out+y_index), A, B, C, D, E, F, G, H, I,&x_fixed,&y_fixed, &x_in, &y_in);
		if((x_in>=0) && (x_in<x_mintemp))
		{
			x_mintemp = x_in;
		}

		if((y_in>=0) && (y_in<y_min))
		{
			y_mintemp = y_in;
		}
	}
	x_min = x_mintemp;
	y_min = y_mintemp;
}

/*
 *	Traversing through the Right edge to calculate the
 *	Minimum x position from where the input is to be read
 */
template<int ROWS, int COLS, int WORDWIDTH_T>
void xFPerspectiveFindRightMin(int16_t x_index,int16_t y_index,XF_PTNAME(WORDWIDTH_T) A,XF_PTNAME(WORDWIDTH_T) B,XF_PTNAME(WORDWIDTH_T) C,
		XF_PTNAME(WORDWIDTH_T) D,XF_PTNAME(WORDWIDTH_T) E,XF_PTNAME(WORDWIDTH_T) F,XF_PTNAME(WORDWIDTH_T) G,
		XF_PTNAME(WORDWIDTH_T) H,XF_PTNAME(WORDWIDTH_T) I,ap_uint<9> max_output_range,
		int16_t &x_min,int16_t &y_min, ap_uint<13> img_height, ap_uint<13> img_width)
{
#pragma HLS INLINE off
	int16_t x_in,y_in,x_mintemp,y_mintemp;
	ap_uint<9> x_out=max_output_range-1,y_out=0;
	ap_int<64> x_fixed,y_fixed;
	x_mintemp = img_width;
	y_mintemp = img_height;
	for( y_out=0;y_out<max_output_range;y_out++)
	{
#pragma HLS loop_tripcount  min=108 max=176 avg=176
#pragma HLS pipeline
		xFPerspectiveTransform<WORDWIDTH_T>((x_out+x_index), (y_out+y_index), A, B, C, D, E, F, G, H, I,&x_fixed,&y_fixed, &x_in, &y_in);
		if((x_in>=0) && (x_in<x_mintemp))
		{
			x_mintemp = x_in;
		}

		if((y_in>=0) && (y_in<y_mintemp))
		{
			y_mintemp = y_in;
		}
	}
	x_min = x_mintemp;
	y_min = y_mintemp;
}
/*
 *	Finding Xminimum and Yminimum from where the
 *	data is to be read
 */
template<int ROWS, int COLS, int WORDWIDTH_T>
void xFPerspectiveFindInputPatchPosition(XF_PTNAME(WORDWIDTH_T) *transform_matrix,int16_t *ymin_patch,
		int16_t *xmin_patch,int16_t y_index, int16_t x_index, ap_uint<9> max_output_range, ap_uint<13> img_height, ap_uint<13> img_width)
{
	//#pragma HLS inline

	int16_t x_min,x_max, y_min,y_max,x_minbyshift;
	int16_t x_min1,x_min2,x_min3,x_min4;
	int16_t y_min1,y_min2,y_min3,y_min4;
	XF_PTNAME(WORDWIDTH_T) A, B, C, D, E, F, G, H, I;

	x_min1 = x_min2 = x_min3 = x_min4 = img_width;
	y_min1 = y_min2 = y_min3 = y_min4 = img_height;

	A = transform_matrix[0];
	B = transform_matrix[1];
	C = transform_matrix[2];
	D = transform_matrix[3];
	E = transform_matrix[4];
	F = transform_matrix[5];
	G = transform_matrix[6];
	H = transform_matrix[7];
	I = transform_matrix[8];

	xFPerspectiveFindTopMin<ROWS,COLS,WORDWIDTH_T>(x_index,y_index,A,B,C,D,E,F,G,H,I,max_output_range,x_min1,y_min1,img_height,img_width);
	xFPerspectiveFindBottomMin<ROWS,COLS,WORDWIDTH_T>(x_index,y_index,A,B,C,D,E,F,G,H,I,max_output_range,x_min2,y_min2,img_height,img_width);
	xFPerspectiveFindLeftMin<ROWS,COLS,WORDWIDTH_T>(x_index,y_index,A,B,C,D,E,F,G,H,I,max_output_range,x_min3,y_min3,img_height,img_width);
	xFPerspectiveFindRightMin<ROWS,COLS,WORDWIDTH_T>(x_index,y_index,A,B,C,D,E,F,G,H,I,max_output_range,x_min4,y_min4,img_height,img_width);

	xFFindMaxMin(x_min1,x_min2,x_min3,x_min4,x_max,x_min);
	xFFindMaxMin(y_min1,y_min2,y_min3,y_min4,y_max,y_min);

	x_minbyshift = x_min>>3;

	*xmin_patch = (x_minbyshift<<3);
	*ymin_patch = y_min;
}

/*
 * Calculating the Xminimum, Yminimum, Xmaximum and Ymaximum
 * for the 2x2 subblocks, these parameters are reqd during
 * the BRAM organization
 */
template<int WORDWIDTH_T>
void xFPerspectiveCalculateBlockPositions(XF_PTNAME(WORDWIDTH_T)* transform_matrix,int16_t y_index,int16_t x_index,
		int16_t *y_patchmin,int16_t  *x_patchmin,int16_t *y_min,int16_t *x_min,int16_t *y_max,
		int16_t *x_max,ap_uint<9> max_output_range,int16_t img_height,int16_t img_width)
{
#pragma HLS inline
	XF_PTNAME(WORDWIDTH_T) A, B, C, D, E, F, G, H, I;
	ap_int<64> x_fixed,y_fixed;
	int16_t x_in[9],y_in[9],xpoint,ypoint,pointno=0,xtemp,ytemp,x_blockmin,y_blockmin,x_blockmax,y_blockmax;
#pragma HLS ARRAY_PARTITION variable=x_in complete dim=1
#pragma HLS ARRAY_PARTITION variable=y_in complete dim=1

	A = transform_matrix[0];
	B = transform_matrix[1];
	C = transform_matrix[2];
	D = transform_matrix[3];
	E = transform_matrix[4];
	F = transform_matrix[5];
	G = transform_matrix[6];
	H = transform_matrix[7];
	I = transform_matrix[8];

	x_blockmin = x_patchmin[0];
	y_blockmin = y_patchmin[0];

	/*
	 *  Finding the input positions of 9 corners
	 *  of 2x2 subblocks
	 */
	ytemp = 0;
	BLOCKPOSITION_OUTERLOOP:
	for(ap_uint<3> i=0;i<3;i++)
	{
		xtemp = 0;
		BLOCKPOSITION_INNERLOOP:
		for(ap_uint<3> j=0;j<3;j++)
		{
			//#pragma HLS PIPELINE
			xFPerspectiveTransform<WORDWIDTH_T>((x_index + xtemp), (y_index + ytemp), A, B, C, D, E, F, G, H, I,&x_fixed,&y_fixed, &xpoint, &ypoint);
			x_in[pointno] = xpoint>x_blockmin?xpoint:x_blockmin;
			y_in[pointno] = ypoint>y_blockmin?ypoint:y_blockmin;
			xtemp += (max_output_range>>1);
			pointno++;
		}
		ytemp += (max_output_range>>1);
	}

	xFFindMaxMin(x_in[0],x_in[1],x_in[3],x_in[4],x_max[0],x_min[0]);
	xFFindMaxMin(x_in[1],x_in[2],x_in[4],x_in[5],x_max[1],x_min[1]);
	xFFindMaxMin(x_in[3],x_in[4],x_in[6],x_in[7],x_max[2],x_min[2]);
	xFFindMaxMin(x_in[4],x_in[5],x_in[7],x_in[8],x_max[3],x_min[3]);

	xFFindMaxMin(y_in[0],y_in[1],y_in[3],y_in[4],y_max[0],y_min[0]);
	xFFindMaxMin(y_in[1],y_in[2],y_in[4],y_in[5],y_max[1],y_min[1]);
	xFFindMaxMin(y_in[3],y_in[4],y_in[6],y_in[7],y_max[2],y_min[2]);
	xFFindMaxMin(y_in[4],y_in[5],y_in[7],y_in[8],y_max[3],y_min[3]);

	for(ap_uint<3> i=0;i<4;i++)
	{
#pragma HLS UNROLL
		x_max[i] += (x_max[i] < (img_width-1))?1:0;
		y_max[i] += (y_max[i] < (img_height-1))?1:0;
		x_min[i] -= (x_min[i] != 0 ? 1:0);
		y_min[i] -= (y_min[i] != 0 ? 1:0);
	}

	xFFindMaxMin(y_min[0],y_min[1],y_min[2],y_min[3],y_blockmax,y_blockmin);
	xFFindMaxMin(x_min[0],x_min[1],x_min[2],x_min[3],x_blockmax,x_blockmin);

	x_patchmin[0] = x_blockmin;
	y_patchmin[0] = y_blockmin;
}


/*
 * Organize the read row from the input
 * in the respective BRAMs
 */

template<int WORDWIDTH_SRC>
void xFPerspectiveOrganizePackedData(XF_SNAME(WORDWIDTH_SRC) *linebuf,int16_t y_patchmin,int16_t x_patchmin,int16_t *y_min, int16_t *x_min,int16_t *y_max, int16_t *x_max, int16_t rowno,XF_SNAME(WORDWIDTH_SRC) bufs[][BRAMSIZE])
{
#pragma HLS inline off
	XF_SNAME(WORDWIDTH_SRC) PackedPixels,*buf_ptr;
	int16_t x_in,y_in,blockindex,bufindex;
	READ_INNERLOOP:
	for(ap_uint<6> j=0;j<(BLOCKSIZE_BY8);j++)
	{
#pragma HLS loop_tripcount max=32
#pragma HLS PIPELINE
		PackedPixels = linebuf[j];
		x_in = ((x_patchmin)>>3) + j;
		y_in = y_patchmin + rowno;

		CHECK_VBLOCKS:
		for(ap_uint<2> ii=0;ii<2;ii++)
		{
			CHECK_HBLOCKS:
			for(ap_uint<2> jj=0;jj<2;jj++)
			{
				blockindex = (ii<<1) + jj;
				if((x_in >= (x_min[blockindex]>>3)) && (x_in < (x_max[blockindex]+7)>>3)
						&& (y_in >= y_min[blockindex]) && (y_in < (y_max[blockindex])))
				{
					bufindex = ((y_in - y_min[blockindex])>>1)*(SUBBLOCKSIZE_BY8+1) + (x_in - (x_min[blockindex]>>3));
					if((y_in - y_min[blockindex]) & 0x1)
						buf_ptr = (XF_SNAME(WORDWIDTH_SRC) *)bufs[(blockindex<<1) + 1];
					else
						buf_ptr = (XF_SNAME(WORDWIDTH_SRC) *)bufs[(blockindex<<1)];
					buf_ptr[bufindex] = PackedPixels;
				}
			}
		}
	}
}

/*
 * Reading the data from the DDR and
 * arranging them in the respective BRAMS
 */

/*
 * Organize the read row from the input
 * in the respective BRAMs
 */

template<int WORDWIDTH_SRC>
void xFOrganizePackedData_Perspective(XF_SNAME(WORDWIDTH_SRC) *linebuf,int16_t y_patchmin,int16_t x_patchmin,int16_t *y_min, int16_t *x_min,int16_t *y_max, int16_t *x_max, int16_t rowno,XF_SNAME(WORDWIDTH_SRC) bufs[][BRAMSIZE],int16_t blocksize)
{
	XF_SNAME(WORDWIDTH_SRC) PackedPixels,*buf_ptr;
	int16_t x_in,y_in,blockindex,buf_index;
	int16_t blocksizeby8 = (blocksize>>3);

	READ_INNERLOOP:for(int16_t j=0;j<(blocksizeby8);j++)
	{
#pragma HLS LOOP_TRIPCOUNT MAX=32
#pragma HLS PIPELINE
		PackedPixels = linebuf[j];
		x_in = ((x_patchmin)>>3) + j;
		y_in = y_patchmin + rowno;

		CHECK_VBLOCKS:for(ap_uint<2> ii=0;ii<2;ii++)
		{
			CHECK_HBLOCKS:for(ap_uint<2> jj=0;jj<2;jj++)
			{
				blockindex = (ii<<1) + jj;
				if((x_in >= (x_min[blockindex]>>3)) && (x_in < (x_max[blockindex]+7)>>3)
						&& (y_in >= y_min[blockindex]) && (y_in < (y_max[blockindex])))
				{
					buf_index = ((y_in - y_min[blockindex])>>1)*((SUBBLOCKSIZE_BY8)+1) + (x_in - (x_min[blockindex]>>3));
					if((y_in - y_min[blockindex]) & 0x1)
						buf_ptr = (XF_SNAME(WORDWIDTH_SRC) *)bufs[(blockindex<<1) + 1];
					else
						buf_ptr = (XF_SNAME(WORDWIDTH_SRC) *)bufs[(blockindex<<1)];
					buf_ptr[buf_index] = PackedPixels;
				}
			}
		}
	}
}


template<int ROWS, int COLS, int WORDWIDTH_SRC, int WORDWIDTH_T>
void xFReadInputPatchBlocks_pingpong_perspective(unsigned long long int *gmem, XF_PTNAME(WORDWIDTH_T) *transform_matrix,int16_t y_index,int16_t x_index,ap_uint<9> blocksize,int16_t *ypatchmin,int16_t *xpatchmin,
		int16_t y_min[], int16_t x_min[], XF_SNAME(WORDWIDTH_SRC) bufs[][BRAMSIZE], ap_uint<13> img_height, ap_uint<13> img_width)
{
#pragma HLS inline off
	XF_SNAME(WORDWIDTH_SRC) linebuf1[BLOCKSIZE],linebuf2[BLOCKSIZE];
	XF_SNAME(WORDWIDTH_SRC) PackedPixels;
	int16_t y_max[4], x_max[4];

	xFPerspectiveFindInputPatchPosition<ROWS, COLS, WORDWIDTH_T>(transform_matrix,ypatchmin,xpatchmin, y_index, x_index, blocksize, img_height, img_width);
	xFPerspectiveCalculateBlockPositions<WORDWIDTH_T>(transform_matrix,y_index, x_index,ypatchmin,xpatchmin,y_min,x_min,y_max,x_max,blocksize,img_height,img_width);

	//int16_t x_patchmin = *xpatchmin;
	//int16_t y_patchmin =  *ypatchmin;


	bool flag = false;
	int _offset, i;
	ap_uint1_t block_flag = 0;

	ap_uint<12> img_widthby8 = img_width>>3;
	int16_t x_pathcminby8 = *xpatchmin>>3;
	int16_t readblocksize;
	int16_t rowreads;


	if( (*xpatchmin>=0) && (*xpatchmin<img_width) && (*ypatchmin>=0) && (*ypatchmin<img_height) )
	{
		block_flag=1;
	}

	if(block_flag)
	{
		if((*ypatchmin+BLOCKSIZE) > img_height)
			rowreads = img_height - *ypatchmin;
		else
			rowreads = BLOCKSIZE;

		_offset = (*ypatchmin * img_widthby8) + x_pathcminby8;
		if( ((*xpatchmin>>3)+32) > img_widthby8)
			readblocksize = (img_widthby8 - x_pathcminby8)<<3;
		else
			readblocksize = BLOCKSIZE;

		xFCopyBlockMemoryIn1<BLOCKSIZE, WORDWIDTH_SRC>(gmem+_offset,linebuf1,readblocksize);

		READ_OUTERLOOP:
		for(i=1;i<rowreads;i++)
		{
#pragma HLS loop_tripcount  min=1 max=256 avg=256

			if((i+ *ypatchmin) < img_height)
			{
				_offset = _offset + img_widthby8;
				if(!flag)
				{
					xFCopyBlockMemoryIn1<BLOCKSIZE, WORDWIDTH_SRC>(gmem+_offset,linebuf2,readblocksize);
					xFOrganizePackedData_Perspective<WORDWIDTH_SRC>(linebuf1,*ypatchmin,*xpatchmin,y_min, x_min,y_max, x_max,i-1,bufs,readblocksize);
					flag = true;
				}
				else
				{
					xFCopyBlockMemoryIn1<BLOCKSIZE, WORDWIDTH_SRC>(gmem+_offset,linebuf1,readblocksize);
					xFOrganizePackedData_Perspective<WORDWIDTH_SRC>(linebuf2,*ypatchmin,*xpatchmin,y_min, x_min,y_max, x_max,i-1,bufs,readblocksize);
					flag = false;
				}
			}else
				break;
		}
		if(flag)
			xFOrganizePackedData_Perspective<WORDWIDTH_SRC>(linebuf2,*ypatchmin,*xpatchmin,y_min, x_min,y_max, x_max,i-1,bufs,readblocksize);
		else
			xFOrganizePackedData_Perspective<WORDWIDTH_SRC>(linebuf1,*ypatchmin,*xpatchmin,y_min, x_min,y_max, x_max,i-1,bufs,readblocksize);
	}

}



static void Pixel_Compute(uint32_t x_frac, uint32_t y_frac, uint32_t xy_frac,int16_t pix1,int16_t pix2,int16_t pix3,ap_uint8_t pixel1,ap_uint8_t pixel2,ap_uint8_t pixel3,ap_uint8_t pixel4,int64_t P1,int64_t P2,int64_t P3,int64_t P4,ap_uint8_t &pixel)
{
#pragma HLS INLINE OFF
	xy_frac = ((uint64_t)x_frac*y_frac) >> 32;

	pix1 = pixel2 - pixel1;
	pix2 = pixel3 - pixel1;
	pix3 = (pixel4 - pixel3)-pix1;

	P1 = ((int64_t)pix3*xy_frac);
	P2 = ((int64_t)pix1*x_frac);
	P3 = ((int64_t)pix2*y_frac);
	P4 = ((int64_t)pixel1<<32);
	pixel = (uchar_t)((int64_t)(P1 + P2 + P3 + P4)>>32);
}

/*
 * Processing Block:
 * Processes a block of the 2x2 subblocks

xFPerspectiveTransform<WORDWIDTH_T>(tempx_out,tempy_out,A,B,C,D,E,F,G,H,I,&x_temp,&y_temp,&x_in2,&y_in2);
 */

template<int ROWS,int COLS,int WORDWIDTH_SRC, int WORDWIDTH_DST, int WORDWIDTH_T>
void xFPerspectiveProcessBlock(XF_PTNAME(WORDWIDTH_T) *transform_matrix,int16_t y_patchmin,int16_t x_patchmin, int16_t y_min, int16_t x_min, int16_t y_index, int16_t x_index,
		XF_SNAME(WORDWIDTH_DST)* lbuf_out, ap_uint<9> max_output_range,XF_SNAME(WORDWIDTH_SRC)* buf0,
		XF_SNAME(WORDWIDTH_SRC)* buf1,ap_uint8_t blockindex,ap_uint<13> img_height, ap_uint<13> img_width,ap_uint<1> interpolation)
{

	XF_SNAME(WORDWIDTH_DST) OutPackedPixels;
	ap_uint8_t pixpos,pixpos2,pixpos3,pixpos4;
	ap_uint8_t pixel,pixel1,pixel2,pixel3,pixel4;
	XF_SNAME(WORDWIDTH_DST) PackedPixel,PackedPixel2,PackedPixel3,PackedPixel4;

	int64_t P1, P2, P3, P4;
	int16_t pix1, pix2, pix3;
	int16_t max_block_range = (max_output_range>>1);
	ap_int<64> x_fixed,y_fixed;
	int16_t x_in1, x_in2,y_in1, y_in2;
	XF_PTNAME(WORDWIDTH_T) A, B, C, D, E, F, G, H, I;
	uint32_t x_frac, y_frac, xy_frac;
	uint16_t buf_index,k,outindex;

	XF_SNAME(WORDWIDTH_SRC) *buf_ptr0,*buf_ptr1;
	uint16_t buf_offset;
	int16_t tempx_out,tempy_out,x_floor,y_floor;

	A = transform_matrix[0];
	B = transform_matrix[1];
	C = transform_matrix[2];
	D = transform_matrix[3];
	E = transform_matrix[4];
	F = transform_matrix[5];
	G = transform_matrix[6];
	H = transform_matrix[7];
	I = transform_matrix[8];

	OutPackedPixels = 0;

	tempy_out = y_index;
	PROCESSBLOCK_OUTER:
	for (ap_uint<9> y_out = 0; y_out < max_block_range; y_out++)
	{
#pragma HLS loop_tripcount  min=50 max=88 avg=88					// blocksize/2
		tempx_out = x_index;
		PROCESSBLOCK_INNER:
		for (ap_uint<9> x_out = 0; x_out < max_block_range; x_out++)
		{
#pragma HLS loop_tripcount  min=50 max=88 avg=88					// blocksize/2
#pragma HLS PIPELINE
			xFPerspectiveTransform<WORDWIDTH_T>(tempx_out,tempy_out,A,B,C,D,E,F,G,H,I,&x_fixed,&y_fixed,&x_in2,&y_in2);
			pixel= 0x0;


			x_in1 = x_in2 - ((x_min>>3)<<3);
			y_in1 = y_in2 - y_min;


			if((x_in2>0)&&((x_in2+1)<img_width)&&(y_in2>0)&&((y_in2+1)<img_height)&&(x_in1>=0)&&(y_in1>=0))
			{

				if(interpolation == XF_INTERPOLATION_BILINEAR)
				{
					buf_index =  (y_in1>>1)*(SUBBLOCKSIZE_BY8 + 1) + (x_in1>>3);
					pixpos = (x_in1 & 0x7);

					PackedPixel   = (y_in1 & 0x1)? buf1[buf_index]:buf0[buf_index];
					PackedPixel3  = (y_in1 & 0x1)? buf0[buf_index + ((SUBBLOCKSIZE_BY8)+1)]:buf1[buf_index + (0)];

					if(pixpos == 7)
					{
						PackedPixel2 = (y_in1 & 0x1)? buf1[buf_index + 1]:buf0[buf_index + 1];
						PackedPixel4 = (y_in1 & 0x1)? buf0[buf_index + ((SUBBLOCKSIZE_BY8)+1) + 1]:buf1[buf_index + (0) + 1];
						pixpos2 = pixpos4 = 0;
					}
					else
					{
						PackedPixel2 = PackedPixel;
						PackedPixel4 = PackedPixel3;
						pixpos2 = pixpos4 = pixpos + 1;
					}
					pixpos3 = pixpos;

					x_frac = (uint32_t) (x_fixed - ((ap_int<64>)x_in2<<32));
					y_frac = (uint32_t) (y_fixed - ((ap_int<64>)y_in2<<32));

					pixel1 = PackedPixel.range((pixpos<<3)+7,(pixpos)<<3);
					pixel2 = PackedPixel2.range((pixpos2<<3)+7,(pixpos2<<3));
					pixel3 = PackedPixel3.range((pixpos3<<3)+7,(pixpos3<<3));
					pixel4 = PackedPixel4.range((pixpos4<<3)+7,(pixpos4<<3));


					Pixel_Compute(x_frac,y_frac, xy_frac, pix1, pix2, pix3, pixel1, pixel2, pixel3, pixel4, P1, P2, P3, P4, pixel);


				}
				else if(interpolation == XF_INTERPOLATION_NN){
					buf_index =  (y_in1>>1)*(SUBBLOCKSIZE_BY8 + 1) + (x_in1>>3);
					pixpos = (x_in1 & 0x7);

					if(y_in1 & 0x1)
						buf_ptr0 = buf1;
					else
						buf_ptr0 = buf0;

					PackedPixel = buf_ptr0[buf_index];
					pixel = PackedPixel.range((pixpos<<3)+7,(pixpos)<<3);
				}
			}
			else if((((y_in2+1)==img_height)&&(x_in2>=0)&&((x_in2)<img_width)) ||
					(((x_in2+1)==img_width)&&(y_in2>=0)&&(y_in2<img_height))   ||
					(((y_in2)==0)&&(x_in2>=0)&&((x_in2)<img_width)) ||
					(((x_in2)==0)&&(y_in2>=0)&&(y_in2<img_height))	)
			{
				buf_index =  (y_in1>>1)*(SUBBLOCKSIZE_BY8 + 1) + (x_in1>>3);
				if(y_in1 & 0x1)
					buf_ptr0 = buf1;
				else
					buf_ptr0 = buf0;
				PackedPixel = buf_ptr0[buf_index];
				pixpos = (x_in1 & 0x7);
				pixel = PackedPixel.range((pixpos<<3)+7,(pixpos)<<3);
			}

			k = (x_out & 0x7);
			OutPackedPixels = OutPackedPixels | (XF_SNAME(WORDWIDTH_DST)(pixel) << (k<<3));
			if((x_out & 0x7) == 7)
			{
				outindex = (y_out*(max_block_range>>3)  + (x_out>>3) );
				lbuf_out[outindex] = OutPackedPixels;
				OutPackedPixels = 0;
			}
			tempx_out++;
		}
		tempy_out++;
	}





}

/*
 * Processes the maxoutput block that can be
 * calculated using 256x256 input block
 */
template<int ROWS, int COLS, int NPC,int WORDWIDTH_SRC, int WORDWIDTH_DST, int WORDWIDTH_T>
void xFPerspectiveProcessFunction(XF_PTNAME(WORDWIDTH_T) *transform_matrix,int16_t y_patchmin,int16_t x_patchmin, int16_t *y_min, int16_t *x_min, int16_t y_index, int16_t x_index,
		XF_SNAME(WORDWIDTH_DST) lbuf_out[][BUFSIZE], ap_uint<9> max_output_range,
		XF_SNAME(WORDWIDTH_SRC) bufs[][BRAMSIZE],ap_uint<13> img_height,ap_uint<13> img_width,ap_uint<1> interpolation)
{
#pragma HLS INLINE off

	XF_SNAME(WORDWIDTH_SRC) PackedPixel0;
	XF_PTNAME(WORDWIDTH_T) trans_mat0[9];
	int16_t write_index,write_index_y,max_output_range_by8,yout_offset[4],xout_offset[4];
	ap_uint<9> x_out, y_out;
	ap_uint1_t block_flag = 0;
	if( (x_patchmin>=0) && (x_patchmin<img_width) && (y_patchmin>=0) && (y_patchmin<img_height) )
	{
		block_flag=1;
	}

	max_output_range_by8 = (max_output_range >>3);

	xout_offset[0] = x_index; xout_offset[1] = x_index + (max_output_range>>1);
	xout_offset[2] = x_index; xout_offset[3] = x_index + (max_output_range>>1);

	yout_offset[0] = y_index; yout_offset[1] = y_index;
	yout_offset[2] = y_index + (max_output_range>>1); yout_offset[3] = y_index + (max_output_range>>1);

	trans_mat0[0] = transform_matrix[0];
	trans_mat0[1] = transform_matrix[1];
	trans_mat0[2] = transform_matrix[2];
	trans_mat0[3] = transform_matrix[3];
	trans_mat0[4] = transform_matrix[4];
	trans_mat0[5] = transform_matrix[5];
	trans_mat0[6] = transform_matrix[6];
	trans_mat0[7] = transform_matrix[7];
	trans_mat0[8] = transform_matrix[8];

	if(block_flag==1)
	{
		for(ap_uint<3> i=0;i<4;i++)
		{
			if(XF_BITSHIFT(NPC)){
#pragma HLS UNROLL
			}
			//xFPerspectiveProcessBlock<ROWS, COLS, WORDWIDTH_SRC, WORDWIDTH_DST, WORDWIDTH_T>(trans_mat0,y_patchmin,x_patchmin, y_min[i],x_min[i],yout_offset[i],xout_offset[i],lbuf_out[i],max_output_range,bufs[i<<1],bufs[(i<<1)+1],i, interpolation, img_height, img_width);
			xFPerspectiveProcessBlock<ROWS, COLS, WORDWIDTH_SRC, WORDWIDTH_DST, WORDWIDTH_T>(trans_mat0,y_patchmin,x_patchmin, y_min[i],x_min[i],yout_offset[i],xout_offset[i],lbuf_out[i],max_output_range,bufs[(i<<1)],bufs[(i<<1)+1],i,img_height,img_width,interpolation);
		}
	}
	else
	{
		OUTLOOP2:
		for (y_out = 0; y_out < (max_output_range>>1); y_out++)
		{
#pragma HLS loop_tripcount  min=32 max=88 avg=88
			write_index_y = y_out*(max_output_range_by8>>1);
			OUTLOOP3:for(x_out = 0; x_out < (max_output_range_by8>>1); x_out++)
			{
#pragma HLS loop_tripcount  min=4 max=11 avg=11
#pragma HLS PIPELINE
				write_index = write_index_y +x_out;
				for(ap_uint<3> k=0;k<4;k++)
				{
#pragma HLS UNROLL
					lbuf_out[k][write_index] = 0x0;
				}
			}
		}
	}
}

/*
 * Writing out the data to the DDR, 2x2 blocks which are processed
 * parallely are written into the DDR sequentially.
 */

template<int ROWS, int COLS, int WORDWIDTH_SRC, int WORDWIDTH_DST, int WORDWIDTH_T>
void xFPerspectiveWriteOutPatch(XF_SNAME(WORDWIDTH_DST) lbuf_out[][BUFSIZE], unsigned long long int * addr_out, int16_t y_index, int16_t x_index,
		ap_uint<9> max_output_range, int16_t out_block_height, int16_t out_block_width,ap_uint<13> img_height,ap_uint<13> img_width)
{
#pragma HLS INLINE OFF
	int outindexX,outindexY,inindexY;

	int16_t writeblocksize02,writeblocksize13;
	int16_t loopbound01,loopbound23;

	if(out_block_height == (max_output_range))
	{
		loopbound23 = loopbound01 = (max_output_range>>1);
	}
	else if(out_block_height > (max_output_range>>1))
	{
		loopbound23 = out_block_height - (max_output_range>>1);
		loopbound01 = (max_output_range>>1);
	}
	else
	{
		loopbound23 = 0;
		loopbound01 = out_block_height;
	}

	if(out_block_width == max_output_range)
	{
		writeblocksize02 = writeblocksize13 = (max_output_range>>1);
	}
	else if(out_block_width > (max_output_range>>1))
	{
		writeblocksize02 = (max_output_range>>1);
		writeblocksize13 = out_block_width - (max_output_range>>1);
	}
	else
	{
		writeblocksize02 = out_block_width;
		writeblocksize13 = 0;
	}

	outindexX = (x_index>>3);
	outindexY =  y_index*(img_width>>3) + outindexX;
	inindexY = 0;

	WRITE_OUT_BLOCK0:
	for (int16_t y_out = 0; y_out < (loopbound01); y_out++)
	{
#pragma HLS loop_tripcount  min=104 max=120 avg=120					// blocksize/2
#if _XF_SYNTHESIS_
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[0][inindexY],addr_out+outindexY,120);
#else
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[0][inindexY],addr_out+outindexY,writeblocksize02);
#endif
		outindexY += (img_width>>3);
		inindexY += (max_output_range>>4);
	}
	outindexX = (x_index>>3)+(writeblocksize02>>3);
	outindexY =  y_index*(img_width>>3) + outindexX;
	inindexY = 0;

	WRITE_OUT_BLOCK1:
	for (int16_t y_out = 0; y_out < (loopbound01); y_out++)
	{
#pragma HLS loop_tripcount  min=88 max=88 avg=88					// blocksize/2
#if _XF_SYNTHESIS_
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[1][inindexY],addr_out+outindexY,120);
#else
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[1][inindexY],addr_out+outindexY,writeblocksize13);
#endif
		outindexY += (img_width>>3);
		inindexY += (max_output_range>>4);
	}

	outindexX = (x_index>>3);
	outindexY = (y_index + (max_output_range>>1))*(img_width>>3) + outindexX;
	inindexY = 0;
	WRITE_OUT_BLOCK2:
	for (int16_t y_out = 0; y_out < loopbound23; y_out++)
	{
#pragma HLS loop_tripcount  min=88 max=88 avg=88				// blocksize/2
#if _XF_SYNTHESIS_
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[2][inindexY],addr_out+outindexY,120);
#else
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[2][inindexY],addr_out+outindexY,writeblocksize02);
#endif
		outindexY += (img_width>>3);
		inindexY += (max_output_range>>4);
	}

	outindexX = (x_index>>3)+(writeblocksize02>>3);
	outindexY = (y_index + (max_output_range>>1))*(img_width>>3) + outindexX;
	inindexY = 0;

	WRITE_OUT_BLOCK3:
	for (int16_t y_out = 0; y_out < loopbound23; y_out++)
	{
#pragma HLS loop_tripcount  min=88 max=88 avg=88				// blocksize/2
#if _XF_SYNTHESIS_
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[3][inindexY],addr_out+outindexY,120);
#else
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[3][inindexY],addr_out+outindexY,writeblocksize13);
#endif
		outindexY += (img_width>>3);
		inindexY += (max_output_range>>4);
	}
}

/*
 * Perspective Transform Kernel function:
 *
 */
template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFWarpPerspective(unsigned long long int *source, unsigned long long int *dst, unsigned short int img_height, unsigned short int img_width, ap_uint<1> interpolation, float *transformation_matrix)
{

	//#pragma HLS license key=IPAUVIZ_WarpPerspective
	//	assert(((interpolation == XF_INTERPOLATION_NN) || (interpolation == XF_INTERPOLATION_BILINEAR))
	//			&& "Interpolation supported are AU_INTERPOLATION_NN and AU_INTERPOLATION_BILINEAR");
	//
	//	assert(((img_height <= ROWS ) && (img_width <= COLS)) && "ROWS and COLS should be greater than input image");

	XF_SNAME(WORDWIDTH_SRC) lbuf_out1[4][BUFSIZE];
	XF_SNAME(WORDWIDTH_SRC) lbuf_out2[4][BUFSIZE];
	if(XF_BITSHIFT(NPC))
	{
#pragma HLS ARRAY_PARTITION variable=lbuf_out1 block factor=4 dim=1
#pragma HLS ARRAY_PARTITION variable=lbuf_out2 block factor=4 dim=1
	}

	ap_uint<9> processed_blocks_col, processed_blocks_row;
	ap_uint<9>  max_out_blocks_col, max_out_blocks_row;
	int16_t x_index, y_index;
	int16_t x_offset, y_offset,x_patchmin,y_patchmin;
	int16_t x_min[4], y_min[4],x_max[4], y_max[4];
	int16_t x_index1,x_index2;
	int16_t y_min2[4], y_min1[4],x_min2[4], x_min1[4];
	int16_t y_patchmin1;
	int16_t y_patchmin2;
	int16_t x_patchmin1;
	int16_t x_patchmin2;

#pragma HLS ARRAY_PARTITION variable=x_min complete dim=1
#pragma HLS ARRAY_PARTITION variable=x_min1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=x_min2 complete dim=1
#pragma HLS ARRAY_PARTITION variable=y_min complete dim=1
#pragma HLS ARRAY_PARTITION variable=y_min1 complete dim=1
#pragma HLS ARRAY_PARTITION variable=y_min2 complete dim=1

#pragma HLS ARRAY_PARTITION variable=x_max complete dim=1
#pragma HLS ARRAY_PARTITION variable=y_max complete dim=1

	int16_t out_block_width, out_block_height;
	ap_uint<9> max_output_range=0;
	ap_uint1_t block_flag, process_flag;
	XF_SNAME(WORDWIDTH_SRC) bufs1[8][BRAMSIZE],bufs2[8][BRAMSIZE];

#pragma HLS ARRAY_PARTITION variable=bufs1 block factor=8 dim=1
#pragma HLS ARRAY_PARTITION variable=bufs2 block factor=8 dim=1
	XF_PTNAME(XF_48SP) transform_matrix[9];
#pragma HLS ARRAY_PARTITION variable=transform_matrix complete dim=0
	ap_uint<9> write_count=0;

	y_offset = (img_height>>1) - 1;
	x_offset = (img_width>>1) - 1;

	ap_uint<13> imgheight = img_height;
	ap_uint<13> imgwidth  = img_width;

	for(int i=0;i<9;i++)
		transform_matrix[i] = XF_PTNAME(XF_48SP)(transformation_matrix[i] * TWO_POW_32);

	max_output_range = xFPerspectiveFindMaxOutSize<XF_48SP>(transform_matrix, x_offset, y_offset,img_height,img_width);

	assert(( max_output_range >= 32 )
			&& "Scaling factor in the transformation matrix should be greater than 0.25");

	max_out_blocks_col = (img_width/max_output_range);
	if((img_width-(max_out_blocks_col*max_output_range))>0)
		max_out_blocks_col += 1;
	max_out_blocks_row = (img_height/max_output_range);
	if((img_height-(max_out_blocks_row*max_output_range))>0)
		max_out_blocks_row += 1;

	y_index = 0;

	for(ap_uint<4> j=0;j<8;j++)
	{
		for(ap_uint<13> i=0;i<BRAMSIZE;i++)
		{
			bufs1[j][i] = 0xFFFFFFFFFFFFFFFF;
			bufs2[j][i] = 0xFFFFFFFFFFFFFFFF;
		}
	}

	for(ap_uint<3> j=0;j<4;j++)
	{
		for(ap_uint<13> i=0;i<BUFSIZE;i++)
		{
			lbuf_out1[j][i] = 0xFFFFFFFFFFFFFFFF;
			lbuf_out2[j][i] = 0xFFFFFFFFFFFFFFFF;
		}
	}

	x_patchmin = y_patchmin = 0;
	ap_uint<1> flag_X;


	ROWLOOP:
	for (processed_blocks_row = 0; processed_blocks_row < max_out_blocks_row; processed_blocks_row++)
	{
#pragma HLS loop_tripcount  min=7 max=7 avg=7
		x_index	 = 0;
		x_index1 = 0;
		x_index2 = 0;
		if(img_height>=(processed_blocks_row*max_output_range+max_output_range))
		{
			out_block_height = max_output_range;
		}
		else
		{
			out_block_height = img_height -(processed_blocks_row*max_output_range);
		}

		flag_X = 0;
		write_count = 0;
		WIDTHLOOP:for (processed_blocks_col = 0; processed_blocks_col < max_out_blocks_col+2; processed_blocks_col++)
		{
#pragma HLS loop_tripcount  min=7 max=13 avg=13
			if(img_width>=((write_count)*max_output_range+max_output_range))
				out_block_width = max_output_range;
			else
				out_block_width = imgwidth -((write_count)*max_output_range);


			if(processed_blocks_col == 0)
			{
				xFReadInputPatchBlocks_pingpong_perspective<ROWS,COLS,WORDWIDTH_SRC, XF_48SP>(source,transform_matrix,y_index, x_index,max_output_range,&y_patchmin,&x_patchmin, y_min, x_min, bufs1, imgheight, imgwidth);
			}
			else if(processed_blocks_col == 1) // One set of data is already read, hence it needs to be processed, and a new set of data has to be read
			{
				xFReadInputPatchBlocks_pingpong_perspective<ROWS,COLS,WORDWIDTH_SRC, XF_48SP>(source,transform_matrix,y_index, x_index,max_output_range,&y_patchmin,&x_patchmin, y_min, x_min, bufs2, imgheight, imgwidth);

				xFPerspectiveProcessFunction<ROWS,COLS,NPC,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(transform_matrix, y_patchmin1,x_patchmin1,y_min1, x_min1,
						y_index, x_index1, lbuf_out1, max_output_range, bufs1, imgheight, imgwidth,interpolation);
			}
			else if(processed_blocks_col == max_out_blocks_col+1 || processed_blocks_col == max_out_blocks_col ) // The last extra two iterations for flushing the already read data
			{
				if(flag_X == 0)
				{
					//Process
					xFPerspectiveProcessFunction<ROWS,COLS,NPC,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(transform_matrix, y_patchmin1,x_patchmin1,y_min1, x_min1,
							y_index, x_index1, lbuf_out2, max_output_range, bufs2, imgheight, imgwidth,interpolation);

					write_count++;
					// Write
					xFPerspectiveWriteOutPatch<ROWS,COLS,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(lbuf_out1, dst, y_index, x_index2, max_output_range, out_block_height, out_block_width,imgheight, imgwidth);
				}
				else if(flag_X == 1)
				{
					//Process
					xFPerspectiveProcessFunction<ROWS,COLS,NPC,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(transform_matrix, y_patchmin1,x_patchmin1,y_min1, x_min1,
							y_index, x_index1, lbuf_out1, max_output_range, bufs1, imgheight, imgwidth,interpolation);

					write_count++;
					// Write
					xFPerspectiveWriteOutPatch<ROWS,COLS,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(lbuf_out2, dst, y_index, x_index2, max_output_range, out_block_height, out_block_width,imgheight, imgwidth);
				}
			}
			else // For all the other cases, do read, process and then write in the respective order on the data available in ping pong fashion
			{
				if (flag_X == 0)
				{
					// Read
					xFReadInputPatchBlocks_pingpong_perspective<ROWS,COLS,WORDWIDTH_SRC, XF_48SP>(source,transform_matrix,y_index, x_index,max_output_range,&y_patchmin,&x_patchmin, y_min, x_min, bufs1, imgheight, imgwidth);

					//Process
					xFPerspectiveProcessFunction<ROWS,COLS,NPC,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(transform_matrix, y_patchmin1,x_patchmin1,y_min1, x_min1,
							y_index, x_index1, lbuf_out2, max_output_range, bufs2, imgheight, imgwidth,interpolation);

					write_count++;
					// Write
					xFPerspectiveWriteOutPatch<ROWS,COLS,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(lbuf_out1, dst, y_index, x_index2, max_output_range, out_block_height, out_block_width,imgheight, imgwidth);
				}
				else if (flag_X == 1)
				{
					// Read
					xFReadInputPatchBlocks_pingpong_perspective<ROWS,COLS,WORDWIDTH_SRC, XF_48SP>(source,transform_matrix,y_index, x_index,max_output_range,&y_patchmin,&x_patchmin, y_min, x_min, bufs2, imgheight, imgwidth);

					//Process
					xFPerspectiveProcessFunction<ROWS,COLS,NPC,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(transform_matrix, y_patchmin1,x_patchmin1,y_min1, x_min1,
							y_index, x_index1, lbuf_out1, max_output_range, bufs1, imgheight, imgwidth,interpolation);

					write_count++;
					// Write
					xFPerspectiveWriteOutPatch<ROWS,COLS,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(lbuf_out2, dst, y_index, x_index2, max_output_range, out_block_height, out_block_width,imgheight, imgwidth);
				}
			} // End if If(Row_Start_Flag) Else Condition

			y_patchmin1 = y_patchmin;
			x_patchmin1 = x_patchmin;

			UPDATION_XY_MIN:for(ap_uint<3> i = 0; i < 4; i++)
			{
#pragma HLS unroll
				y_min1[i] = y_min[i];
				x_min1[i] = x_min[i];
			}
			x_index2 = x_index1;
			x_index1 = x_index;

			if(0 == flag_X)
			{
				flag_X = 1;
			}
			else if(1 == flag_X)
			{
				flag_X = 0;
			}

			if(processed_blocks_col < max_out_blocks_col-1)
			{
				x_index += max_output_range;
			}
		} // End of Column Loop
		y_index += max_output_range;
	} // End of Row Loop*/
}

#pragma SDS data zero_copy("_src_mat.data"[0:"_src_mat.size"])
#pragma SDS data zero_copy("_dst_mat.data"[0:"_dst_mat.size"])
#pragma SDS data zero_copy(transformation_matrix[0:9])

#pragma SDS data mem_attribute ("_src_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS, "_dst_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS, transformation_mat:NON_CACHEABLE|PHYSICAL_CONTIGUOUS)

template<int INTERPOLATION_TYPE,int SRC_T, int ROWS, int COLS,int NPC>
void warpPerspective(xf::Mat<SRC_T, ROWS, COLS, XF_NPPC8> & _src_mat,xf::Mat<SRC_T, ROWS, COLS, XF_NPPC8> & _dst_mat,float *transformation_matrix)
{

	xFWarpPerspective<ROWS,COLS,XF_DEPTH(SRC_T,XF_NPPC8),NPC,XF_WORDWIDTH(SRC_T,XF_NPPC8),XF_WORDWIDTH(SRC_T,XF_NPPC8)>((unsigned long long int*)_src_mat.data,(unsigned long long int*)_dst_mat.data,_src_mat.rows,_src_mat.cols,INTERPOLATION_TYPE,transformation_matrix);

}
}

#endif
