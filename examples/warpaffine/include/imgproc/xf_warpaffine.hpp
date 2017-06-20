#ifndef _XF_WARPAFFINE_HPP_
#define _XF_WARPAFFINE_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#define INPUT_BLOCK_LENGTH 		256
#define INPUT_BLOCK_LENGTH_BY2 (INPUT_BLOCK_LENGTH/2)

#define BLOCKSIZE			256
#define BLOCKSIZE_BY8		(BLOCKSIZE>>3)
#define SUBBLOCKSIZE		128
#define SUBBLOCKSIZE_BY8	(SUBBLOCKSIZE>>3)

#define HBLOCKS			2
#define VBLOCKS 		2
#define nBLOCKS			(HBLOCKS*VBLOCKS)
#define BUFSIZE			(BLOCKSIZE_BY8*BLOCKSIZE/nBLOCKS)

#define AF_BRAMSIZE		(SUBBLOCKSIZE*((SUBBLOCKSIZE_BY8)+1)/HBLOCKS)
#define OUTWRITE		120
#define TWO_POW_16		65536
#define TWO_POW_32		4294967296

#define ROUND_DELTA		429496

/*
 * Inverse Affine Transform
 *
 *		xin = xout*M[0] + yout*M[1] + M[2]
 *		yin = xout*M[3] + yout*M[4] + M[5]
 */
#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

template<int WORDWIDTH_T>
void xFTransform(int16_t x_out, int16_t y_out,XF_PTNAME(WORDWIDTH_T) A, XF_PTNAME(WORDWIDTH_T) B, XF_PTNAME(WORDWIDTH_T) C,
		XF_PTNAME(WORDWIDTH_T) D, XF_PTNAME(WORDWIDTH_T) x_const, XF_PTNAME(WORDWIDTH_T) y_const,XF_PTNAME(WORDWIDTH_T) *x_frac,XF_PTNAME(WORDWIDTH_T) *y_frac,int16_t *x_in, int16_t *y_in)
{
#pragma HLS INLINE off

	XF_PTNAME(WORDWIDTH_T) Ax_out,By_out,Cx_out,Dy_out;
	int16_t offsetX=0,offsetY=0;

	Ax_out = (XF_PTNAME(WORDWIDTH_T))(x_out*A + (int64_t)x_const);
	By_out = (XF_PTNAME(WORDWIDTH_T))(y_out*B);
	Cx_out = (XF_PTNAME(WORDWIDTH_T))(x_out*C + (int64_t)y_const);
	Dy_out = (XF_PTNAME(WORDWIDTH_T))(y_out*D);

	x_frac[0] = (Ax_out + By_out + ROUND_DELTA);
	y_frac[0] = (Cx_out + Dy_out + ROUND_DELTA);

	if((x_frac[0] & 0xFFFFFFFF) > 0x80000000)
		offsetX = 1;
	if((y_frac[0] & 0xFFFFFFFF) > 0x80000000)
		offsetY = 1;

	*x_in = (int16_t)((x_frac[0]>>32) + offsetX);
	*y_in = (int16_t)((y_frac[0]>>32) + offsetY);
}

template<int WORDWIDTH_T>
void xFTransform2_1(int16_t x_out, int16_t y_out,XF_PTNAME(WORDWIDTH_T) A, XF_PTNAME(WORDWIDTH_T) B, XF_PTNAME(WORDWIDTH_T) C,
		XF_PTNAME(WORDWIDTH_T) D, XF_PTNAME(WORDWIDTH_T) x_const, XF_PTNAME(WORDWIDTH_T) y_const,XF_PTNAME(WORDWIDTH_T) *x_frac,XF_PTNAME(WORDWIDTH_T) *y_frac,int16_t *x_in, int16_t *y_in,int16_t index)
{
#pragma HLS INLINE off

	XF_PTNAME(WORDWIDTH_T) Ax_out,By_out,Cx_out,Dy_out;
	int16_t offsetX=0,offsetY=0;

	Ax_out = (XF_PTNAME(WORDWIDTH_T))(x_out*A + (int64_t)x_const);
	By_out = (XF_PTNAME(WORDWIDTH_T))(y_out*B);
	Cx_out = (XF_PTNAME(WORDWIDTH_T))(x_out*C + (int64_t)y_const);
	Dy_out = (XF_PTNAME(WORDWIDTH_T))(y_out*D);

	x_frac[0] = (Ax_out + By_out + ROUND_DELTA);
	y_frac[0] = (Cx_out + Dy_out + ROUND_DELTA);


	if((x_frac[0] & 0xFFFFFFFF) > 0x80000000)
		offsetX = 1;
	if((y_frac[0] & 0xFFFFFFFF) > 0x80000000)
		offsetY = 1;

	x_in[index] = (int16_t)((x_frac[0]>>32) + offsetX);
	y_in[index] = (int16_t)((y_frac[0]>>32) + offsetY);
}

/*
 * Finding the maximum and minimum of the four input values
 */
static void xFAffineFindMaxMin(int16_t in1,int16_t in2,int16_t in3,int16_t in4,int16_t &max,int16_t &min)
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
 *	Traversing through the top edge to calculate the
 *	Minimum x position from where the input is to be read
 */
template<int ROWS, int COLS, int WORDWIDTH_T>
void xFFindTopMin(int16_t x_index,int16_t y_index,XF_PTNAME(WORDWIDTH_T) A,XF_PTNAME(WORDWIDTH_T) B,XF_PTNAME(WORDWIDTH_T) C,XF_PTNAME(WORDWIDTH_T) D,XF_PTNAME(WORDWIDTH_T) x_const,XF_PTNAME(WORDWIDTH_T) y_const,ap_uint<9> max_output_range,int16_t &x_min,int16_t &y_min, ap_uint<13> img_height, ap_uint<13> img_width)
{
#pragma HLS INLINE off
	int16_t x_in,y_in,x_mintemp,y_mintemp;
	int16_t y_out=0,buf_index;
	XF_PTNAME(WORDWIDTH_T) x_frac,y_frac;
	ap_uint<9> x_out=0;
	x_mintemp = img_width;
	y_mintemp = img_height;
	for( x_out=0;x_out < max_output_range;x_out++)
	{
#pragma HLS loop_tripcount  min=240 max=240 avg=240
#pragma HLS PIPELINE

		xFTransform<WORDWIDTH_T>((x_out+x_index), (y_out+y_index), A, B, C, D, x_const, y_const,&x_frac,&y_frac, &x_in, &y_in);

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
void xFFindBottomMin(int16_t x_index,int16_t y_index,XF_PTNAME(WORDWIDTH_T) A,XF_PTNAME(WORDWIDTH_T) B,XF_PTNAME(WORDWIDTH_T) C,XF_PTNAME(WORDWIDTH_T) D,XF_PTNAME(WORDWIDTH_T) x_const,XF_PTNAME(WORDWIDTH_T) y_const,ap_uint<9> max_output_range,int16_t &x_min,int16_t &y_min,ap_uint<13> img_height, ap_uint<13> img_width)
{
#pragma HLS INLINE off
	int16_t x_in,y_in,x_mintemp,y_mintemp;
	int16_t y_out2=max_output_range-1;
	XF_PTNAME(WORDWIDTH_T) x_frac,y_frac;
	ap_uint<9> x_out2;
	x_mintemp = img_width;
	y_mintemp = img_height;
	for( x_out2=0;x_out2 < max_output_range;x_out2++)
	{
#pragma HLS loop_tripcount  min=240 max=240 avg=240
#pragma HLS pipeline
		xFTransform<WORDWIDTH_T>((x_out2+x_index), (y_out2+y_index), A, B, C, D, x_const, y_const,&x_frac,&y_frac, &x_in, &y_in);
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
void xFFindLeftMin(int16_t x_index,int16_t y_index,XF_PTNAME(WORDWIDTH_T) A,XF_PTNAME(WORDWIDTH_T) B,XF_PTNAME(WORDWIDTH_T) C,XF_PTNAME(WORDWIDTH_T) D,XF_PTNAME(WORDWIDTH_T) x_const,XF_PTNAME(WORDWIDTH_T) y_const,ap_uint<9> max_output_range,int16_t &x_min,int16_t &y_min,ap_uint<13> img_height, ap_uint<13> img_width)
{
#pragma HLS INLINE off
	int16_t x_in,y_in,x_mintemp,y_mintemp;
	int16_t x_out=0;
	XF_PTNAME(WORDWIDTH_T) x_frac,y_frac;
	ap_uint<9> y_out;
	x_mintemp = img_width;
	y_mintemp = img_height;
	for( y_out=0;y_out<max_output_range;y_out++)
	{
#pragma HLS loop_tripcount  min=240 max=240 avg=240
#pragma HLS pipeline
		xFTransform<WORDWIDTH_T>((x_out+x_index), (y_out+y_index), A, B, C, D, x_const, y_const,&x_frac,&y_frac, &x_in, &y_in);
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
 *	Traversing through the Right edge to calculate the
 *	Minimum x position from where the input is to be read
 */
template<int ROWS, int COLS, int WORDWIDTH_T>
void xFFindRightMin(int16_t x_index,int16_t y_index,XF_PTNAME(WORDWIDTH_T) A,XF_PTNAME(WORDWIDTH_T) B,XF_PTNAME(WORDWIDTH_T) C,XF_PTNAME(WORDWIDTH_T) D,XF_PTNAME(WORDWIDTH_T) x_const,XF_PTNAME(WORDWIDTH_T) y_const,ap_uint<9> max_output_range,int16_t &x_min,int16_t &y_min,ap_uint<13> img_height, ap_uint<13> img_width)
{
#pragma HLS INLINE off
	int16_t x_in,y_in,x_mintemp,y_mintemp;
	int16_t x_out=max_output_range-1;
	XF_PTNAME(WORDWIDTH_T) x_frac,y_frac;
	ap_uint<9> y_out;
	x_mintemp = img_width;
	y_mintemp = img_height;
	for( y_out=0;y_out< max_output_range;y_out++)
	{
#pragma HLS loop_tripcount  min=240 max=240 avg=240
#pragma HLS pipeline
		xFTransform<WORDWIDTH_T>((x_out+x_index), (y_out+y_index), A, B, C, D, x_const, y_const,&x_frac,&y_frac, &x_in, &y_in);
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
 * 	Calculating the Maximum output size that can be calculated
 * 	using a 256*256 input block
 */

template<int WORDWIDTH_T>
int16_t xFFindMaxOutSize(XF_PTNAME(WORDWIDTH_T) *transform_matrix){

	int16_t max_output_range=0,max_output_range_mul16=0;
	ap_uint<9> max_output_range_out;
	int16_t x_min, y_min, x_max, y_max;
	XF_PTNAME(WORDWIDTH_T) x_frac,y_frac;

	XF_PTNAME(WORDWIDTH_T) A, B, C, D, x_const, y_const;
	int16_t x_out[4],y_out[4],x_in[4],y_in[4];
#pragma HLS ARRAY_PARTITION variable=x_out complete dim=1
#pragma HLS ARRAY_PARTITION variable=y_out complete dim=1
#pragma HLS ARRAY_PARTITION variable=x_in complete dim=1
#pragma HLS ARRAY_PARTITION variable=y_in complete dim=1

	A = transform_matrix[0];
	B = transform_matrix[1];
	x_const = transform_matrix[2];
	C = transform_matrix[3];
	D = transform_matrix[4];
	y_const = transform_matrix[5];
	ap_uint<9> p,pp;
	MAX_OUTERLOOP:
	for( p=256,pp=0; p>=0; p=p-8,pp++)
	{
#pragma HLS loop_tripcount  min=16 max=16 avg=16
		x_out[0] = 0; y_out[0] = 0;
		x_out[1] = p; y_out[1] = 0;
		x_out[2] = 0; y_out[2] = p;
		x_out[3] = p; y_out[3] = p;
		// manually unrolled the loop
		xFTransform<WORDWIDTH_T>(x_out[0], y_out[0], A, B, C, D, x_const, y_const,&x_frac,&y_frac, &x_in[0], &y_in[0]);
		xFTransform<WORDWIDTH_T>(x_out[1], y_out[1], A, B, C, D, x_const, y_const,&x_frac,&y_frac, &x_in[1], &y_in[1]);
		xFTransform<WORDWIDTH_T>(x_out[2], y_out[2], A, B, C, D, x_const, y_const,&x_frac,&y_frac, &x_in[2], &y_in[2]);
		xFTransform<WORDWIDTH_T>(x_out[3], y_out[3], A, B, C, D, x_const, y_const,&x_frac,&y_frac, &x_in[3], &y_in[3]);
		xFAffineFindMaxMin(x_in[0],x_in[1],x_in[2],x_in[3],x_max,x_min);
		xFAffineFindMaxMin(y_in[0],y_in[1],y_in[2],y_in[3],y_max,y_min);
		if(((y_max-y_min) <= INPUT_BLOCK_LENGTH) && ((x_max-x_min) <= INPUT_BLOCK_LENGTH))
		{
			max_output_range = p;
			break;
		}
	}

	max_output_range_mul16 = (max_output_range-8);
	max_output_range_out = (max_output_range_mul16&0xFFF0);

	return max_output_range_out;
}

template<int WORDWIDTH_T>
int16_t xFFindMaxOutSize_fail(XF_PTNAME(WORDWIDTH_T) *transform_matrix){

	int16_t max_output_range=0,max_output_range_mul16=0;
	ap_uint<9> max_output_range_out;
	int16_t x_min, y_min, x_max, y_max;
	XF_PTNAME(WORDWIDTH_T) x_frac,y_frac;

	XF_PTNAME(WORDWIDTH_T) A, B, C, D, x_const, y_const;
	int16_t x_out[4],y_out[4],x_in[4],y_in[4];
#pragma HLS ARRAY_PARTITION variable=x_out complete dim=1
#pragma HLS ARRAY_PARTITION variable=y_out complete dim=1
#pragma HLS ARRAY_PARTITION variable=x_in complete dim=1
#pragma HLS ARRAY_PARTITION variable=y_in complete dim=1

	A = transform_matrix[0];
	B = transform_matrix[1];
	x_const = transform_matrix[2];
	C = transform_matrix[3];
	D = transform_matrix[4];
	y_const = transform_matrix[5];

	MAX_OUTERLOOP:
	for(int16_t p=256,pp=0; p>=0; p=p-8,pp++)
	{
#pragma HLS loop_tripcount  min=1 max=32 avg=32
		x_out[0] = 0; y_out[0] = 0;
		x_out[1] = p; y_out[1] = 0;
		x_out[2] = 0; y_out[2] = p;
		x_out[3] = p; y_out[3] = p;

		for(int16_t k=0;k<4;k++)
		{
#pragma HLS UNROLL
			xFTransform<WORDWIDTH_T>(x_out[k], y_out[k], A, B, C, D, x_const, y_const,&x_frac,&y_frac, &x_in[k], &y_in[k]);
		}

		xFAffineFindMaxMin(x_in[0],x_in[1],x_in[2],x_in[3],x_max,x_min);
		xFAffineFindMaxMin(y_in[0],y_in[1],y_in[2],y_in[3],y_max,y_min);

		if(((y_max-y_min) <= INPUT_BLOCK_LENGTH) && ((x_max-x_min) <= INPUT_BLOCK_LENGTH))
		{
			max_output_range = p;
			break;
		}
	}

	max_output_range_mul16 = (max_output_range-8);
	max_output_range_out = (max_output_range_mul16&0xFFF0);

	return max_output_range_out;
}


/*
 *	Finding Xminimum and Yminimum from where the
 *	data is to be read
 */
template<int ROWS, int COLS, int WORDWIDTH_T>
void xFFindInputPatchPosition(XF_PTNAME(XF_48SP) *transform_matrix,int16_t *ymin_patch,int16_t *xmin_patch, int16_t y_index, int16_t x_index, ap_uint<9> max_output_range,ap_uint<13> img_height, ap_uint<13> img_width)

{

#pragma HLS inline
	int16_t x_min,x_max, y_min,y_max,x_minbyshift,y_minbyshift;
	int16_t x_min1,x_min2,x_min3,x_min4;
	int16_t y_min1,y_min2,y_min3,y_min4;
	XF_PTNAME(XF_48SP) A, B, C, D, x_const, y_const;

	x_min1 = x_min2 = x_min3 = x_min4 = img_width;
	y_min1 = y_min2 = y_min3 = y_min4 = img_height;

	A = transform_matrix[0];
	B = transform_matrix[1];
	x_const = transform_matrix[2];
	C = transform_matrix[3];
	D = transform_matrix[4];
	y_const = transform_matrix[5];

	xFFindTopMin<ROWS, COLS, WORDWIDTH_T>(x_index,y_index,A,B,C,D,x_const,y_const,max_output_range,x_min1,y_min1, img_height, img_width);
	xFFindBottomMin<ROWS, COLS, WORDWIDTH_T>(x_index,y_index,A,B,C,D,x_const,y_const,max_output_range,x_min2,y_min2,img_height, img_width);
	xFFindLeftMin<ROWS, COLS, WORDWIDTH_T>(x_index,y_index,A,B,C,D,x_const,y_const,max_output_range,x_min3,y_min3,img_height, img_width);
	xFFindRightMin<ROWS, COLS, WORDWIDTH_T>(x_index,y_index,A,B,C,D,x_const,y_const,max_output_range,x_min4,y_min4,img_height, img_width);

	xFAffineFindMaxMin(x_min1,x_min2,x_min3,x_min4,x_max,x_min);
	xFAffineFindMaxMin(y_min1,y_min2,y_min3,y_min4,y_max,y_min);

	*xmin_patch = (x_min & 0xFFF8);
	*ymin_patch = (y_min & 0xFFF8);
}

/*
 * Organize the read row from the input
 * in the respective BRAMs
 */

template<int WORDWIDTH_SRC>
void xFOrganizePackedData(XF_SNAME(WORDWIDTH_SRC) *linebuf,int16_t y_patchmin,int16_t x_patchmin,int16_t *y_min, int16_t *x_min,int16_t *y_max, int16_t *x_max, int16_t rowno,XF_SNAME(WORDWIDTH_SRC) bufs[][AF_BRAMSIZE],int16_t blocksize)
{
	XF_SNAME(WORDWIDTH_SRC) PackedPixels,*buf_ptr;

	//unsigned long long int PackedPixels,*buf_ptr;

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


static void xFProcessFunc(ap_uint8_t* pixel,int16_t pix1,int16_t pix2,int16_t pix3,ap_uint8_t pixel1, uint32_t x_frac,uint32_t y_frac,uint32_t xy_frac)
{
#pragma HLS INLINE OFF
	int64_t P1, P2, P3, P4;
	int64_t P5, P6;
	ap_uint8_t P7;
	xy_frac = (x_frac*y_frac) >> 16;

	P1 = ((int64_t)pix3*xy_frac);
	P2 = ((int64_t)pix1*x_frac);
	P3 = ((int64_t)(pix2*y_frac));
	P4 = ((int64_t)pixel1<<16);
	P5 = P1+P2;
	P6 = P3+P4;
	P7 = (ap_uint8_t)((P5+P6)>>16);
	*pixel = P7;
}

/*
 * Processing Block0:
 * Processes the (0,0) block of the 2x2 subblocks
 */
template<int ROWS,int COLS,int WORDWIDTH_SRC, int WORDWIDTH_DST, int WORDWIDTH_T>
void xFProcessBlock(XF_PTNAME(WORDWIDTH_T) *transform_matrix,int16_t y_patchmin,int16_t x_patchmin, int16_t y_min, int16_t x_min, int16_t y_index, int16_t x_index,
		XF_SNAME(WORDWIDTH_SRC)* lbuf_out, ap_uint<9> max_output_range,XF_SNAME(WORDWIDTH_SRC)* buf0,XF_SNAME(WORDWIDTH_SRC)* buf1,ap_uint8_t blockindex, uint32_t interpolation, ap_uint<13> img_height, ap_uint<13> img_width)
{
#pragma HLS INLINE OFF
	XF_SNAME(WORDWIDTH_DST) OutPackedPixels;
	//unsigned long long int OutPackedPixels;
	ap_uint8_t pixpos,pixpos2,pixpos3,pixpos4;
	ap_uint8_t pixel,pixel1,pixel2,pixel3,pixel4;
	XF_SNAME(WORDWIDTH_DST) PackedPixel,PackedPixel2,PackedPixel3,PackedPixel4;
	int16_t pix1, pix2, pix3;
	ap_uint<9> max_block_range = (max_output_range>>1);
	XF_PTNAME(WORDWIDTH_T) x_temp,y_temp;
	int16_t x_in1, x_in2;
	int16_t y_in1, y_in2;
	XF_PTNAME(WORDWIDTH_T) A, B, C, D, x_const, y_const;
	uint32_t x_frac, y_frac, xy_frac;
	uint16_t buf_index,k,outindex;
	XF_SNAME(WORDWIDTH_SRC) *buf_ptr0,*buf_ptr1;
	//unsigned long long int *buf_ptr0,*buf_ptr1;
	uint16_t buf_offset;
	int16_t tempx_out,tempy_out;
	int16_t offsetX,offsetY;

	A = transform_matrix[0];
	B = transform_matrix[1];
	x_const = transform_matrix[2];
	C = transform_matrix[3];
	D = transform_matrix[4];
	y_const = transform_matrix[5];

	OutPackedPixels = 0;
	// Initializations
	pixpos = pixpos2 = pixpos3 = pixpos4 = 0;
	pixel1 = pixel2 = pixel3 = pixel4 = 0;
	PackedPixel = PackedPixel2 = PackedPixel3 = PackedPixel4 = 0;
	//P1 = P2 = P3 = P4 = 0;
	pix1 = pix2 = pix3 = 0;
	x_in1 = x_in2 = y_in1 = y_in2 = 0;
	x_frac = y_frac = 0;
	buf_index = k = outindex = buf_offset = tempx_out = tempy_out = offsetX = offsetY = 0;
	ap_uint<9> y_out,x_out;
	tempy_out = y_index;
	PROCESSBLOCK_OUTER:
	for ( y_out = 0; y_out < max_block_range; y_out++)
	{
#pragma HLS loop_tripcount  min=120 max=120 avg=120
		tempx_out = x_index;
		PROCESSBLOCK_INNER:for ( x_out = 0; x_out < max_block_range; x_out++)
		{
#pragma HLS loop_tripcount  min=120 max=120 avg=120
#pragma HLS PIPELINE
			pixel= 0x0;

			xFTransform<WORDWIDTH_T>(tempx_out,tempy_out,A,B,C,D,x_const,y_const,&x_temp,&y_temp,&x_in2,&y_in2);

			x_in1 = x_in2 - ((x_min>>3)<<3);
			y_in1 = y_in2 - y_min;

			if((x_in2>0)&&((x_in2+1)<img_width)&&(y_in2>0)&&((y_in2+1)<img_height)&&(x_in1>=0)&&(y_in1>=0))
			{

				if(interpolation == XF_INTERPOLATION_BILINEAR)
				{
					int16_t x_floor = (x_temp>>32);
					int16_t y_floor = (y_temp>>32);

					x_frac = (uint32_t)(x_temp - ((XF_PTNAME(WORDWIDTH_T))x_floor<<32))>>16;
					y_frac = (uint32_t)(y_temp - ((XF_PTNAME(WORDWIDTH_T))y_floor<<32))>>16;

					if(x_floor < x_in2)
					{
						x_in1 -= 1;
						x_in2 -= 1;
					}

					if(y_floor < y_in2)
					{
						y_in1 -= 1;
						y_in2 -= 1;
					}

					buf_index =  (y_in1>>1)*((SUBBLOCKSIZE_BY8) + 1) + (x_in1>>3);
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

					pixel1 = PackedPixel.range((pixpos<<3)+7,(pixpos)<<3);
					pixel2 = PackedPixel2.range((pixpos2<<3)+7,(pixpos2<<3));
					pixel3 = PackedPixel3.range((pixpos3<<3)+7,(pixpos3<<3));
					pixel4 = PackedPixel4.range((pixpos4<<3)+7,(pixpos4<<3));

					xy_frac = (x_frac*y_frac) >> 16;

					pix1 = ((int16_t)pixel2 - (int16_t)pixel1);
					pix2 = ((int16_t)pixel3 - (int16_t)pixel1);
					pix3 = (((int16_t)pixel4 - (int16_t)pixel3)-(int16_t)pix1);

					xFProcessFunc(&pixel, pix1, pix2, pix3,pixel1, x_frac, y_frac, xy_frac);
				}
				else if(interpolation == XF_INTERPOLATION_NN)
				{
					buf_index =  (y_in1>>1)*((SUBBLOCKSIZE_BY8) + 1) + (x_in1>>3);
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
					(((x_in2+1)==img_width)&&(y_in2>=0)&&(y_in2<img_height))	||
					(((y_in2)==0)&&(x_in2>=0)&&((x_in2)<img_width)) ||
					(((x_in2)==0)&&(y_in2>=0)&&(y_in2<img_height))		)
			{
				buf_index =  (y_in1>>1)*((SUBBLOCKSIZE_BY8) + 1) + (x_in1>>3);
				if(y_in1 & 0x1)
					buf_ptr0 = buf1;
				else
					buf_ptr0 = buf0;
				PackedPixel = buf_ptr0[buf_index];
				pixpos = (x_in1 & 0x7);
				pixel = PackedPixel.range((pixpos<<3)+7,(pixpos)<<3);
			}
			else
			{
				pixel = 0x0;
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
void xFAffineProcessFunction(XF_PTNAME(WORDWIDTH_T) *transform_matrix,int16_t y_patchmin,int16_t x_patchmin, int16_t *y_min, int16_t *x_min, int16_t y_index, int16_t x_index,
		XF_SNAME(WORDWIDTH_SRC) lbuf_out[][BUFSIZE], ap_uint<9> max_output_range, XF_SNAME(WORDWIDTH_SRC) bufs[][AF_BRAMSIZE], uint32_t interpolation,ap_uint<13> img_height, ap_uint<13> img_width,unsigned long long int * addr_out)
{
#pragma HLS INLINE off

	XF_SNAME(WORDWIDTH_SRC) PackedPixel0;
	XF_PTNAME(WORDWIDTH_T) trans_mat0[6];
	int16_t write_index,write_index_y,max_output_range_by8,yout_offset[4],xout_offset[4];
	ap_uint<9> x_out,y_out;
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

	if(block_flag==1)
	{
		for(ap_uint<3> i=0;i<4;i++)
		{
			if(XF_BITSHIFT(NPC)){
#pragma HLS UNROLL
			}
			xFProcessBlock<ROWS, COLS, WORDWIDTH_SRC, WORDWIDTH_DST, WORDWIDTH_T>(trans_mat0,y_patchmin,x_patchmin, y_min[i],x_min[i],yout_offset[i],xout_offset[i],lbuf_out[i],max_output_range,bufs[i<<1],bufs[(i<<1)+1],i, interpolation, img_height, img_width);
		}
	}
	else
	{
		OUTLOOP2:
		for (y_out = 0; y_out < (max_output_range>>1); y_out++)
		{
#pragma HLS loop_tripcount  min=120 max=120 avg=120
			write_index_y = y_out*(max_output_range_by8>>1);
			OUTLOOP3:for(x_out = 0; x_out < (max_output_range_by8>>1); x_out++)
			{
#pragma HLS loop_tripcount  min=15 max=15 avg=15
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
 * Calculating the Xminimum, Yminimum, Xmaximum and Ymaximum
 * for the 2x2 subblocks, these parameters are reqd during
 * the BRAM organization
 */
template<int WORDWIDTH_T>
void xFCalculateBlockPositions(XF_PTNAME(WORDWIDTH_T) *transform_matrix,int16_t y_index,int16_t x_index,int16_t *y_patchmin,int16_t  *x_patchmin,int16_t *y_min,int16_t *x_min,int16_t *y_max,int16_t *x_max,ap_uint<9> max_output_range)
{
#pragma HLS inline
	XF_PTNAME(WORDWIDTH_T) A, B, C, D, x_const, y_const;
	XF_PTNAME(WORDWIDTH_T) x_frac,y_frac;
	int16_t x_in[9],y_in[9],xpoint,ypoint,pointno=0,xtemp,ytemp,x_blockmin,y_blockmin,x_blockmax,y_blockmax;
#pragma HLS ARRAY_PARTITION variable=x_in complete dim=1
#pragma HLS ARRAY_PARTITION variable=y_in complete dim=1

	A = transform_matrix[0];
	B = transform_matrix[1];
	x_const = transform_matrix[2];
	C = transform_matrix[3];
	D = transform_matrix[4];
	y_const = transform_matrix[5];

	x_blockmin = x_patchmin[0];
	y_blockmin = y_patchmin[0];

	ytemp = 0;
	BLOCKPOSITION_OUTERLOOP:for(ap_uint<2> i=0;i<3;i++)
	{
		xtemp = 0;
		BLOCKPOSITION_INNERLOOP:for(ap_uint<2> j=0;j<3;j++)
		{
#pragma HLS PIPELINE

			xFTransform<WORDWIDTH_T>((x_index + xtemp), (y_index + ytemp), A, B, C, D, x_const, y_const,&x_frac,&y_frac, &xpoint, &ypoint);
			x_in[pointno] = xpoint>x_blockmin?xpoint:x_blockmin;
			y_in[pointno] = ypoint>y_blockmin?ypoint:y_blockmin;
			xtemp += (max_output_range>>1);
			pointno++;
		}
		ytemp += (max_output_range>>1);
	}

	xFAffineFindMaxMin(x_in[0],x_in[1],x_in[3],x_in[4],x_max[0],x_min[0]);
	xFAffineFindMaxMin(x_in[1],x_in[2],x_in[4],x_in[5],x_max[1],x_min[1]);
	xFAffineFindMaxMin(x_in[3],x_in[4],x_in[6],x_in[7],x_max[2],x_min[2]);
	xFAffineFindMaxMin(x_in[4],x_in[5],x_in[7],x_in[8],x_max[3],x_min[3]);

	xFAffineFindMaxMin(y_in[0],y_in[1],y_in[3],y_in[4],y_max[0],y_min[0]);
	xFAffineFindMaxMin(y_in[1],y_in[2],y_in[4],y_in[5],y_max[1],y_min[1]);
	xFAffineFindMaxMin(y_in[3],y_in[4],y_in[6],y_in[7],y_max[2],y_min[2]);
	xFAffineFindMaxMin(y_in[4],y_in[5],y_in[7],y_in[8],y_max[3],y_min[3]);

	for(ap_uint<3> i=0;i<4;i++)
	{
#pragma HLS UNROLL
		x_max[i] += 1;
		y_max[i] += 1;
		x_min[i] -= (x_min[i] != 0 ? 1:0);
		y_min[i] -= (y_min[i] != 0 ? 1:0);
	}

	xFAffineFindMaxMin(y_min[0],y_min[1],y_min[2],y_min[3],y_blockmax,y_blockmin);
	xFAffineFindMaxMin(x_min[0],x_min[1],x_min[2],x_min[3],x_blockmax,x_blockmin);

	x_patchmin[0] = x_blockmin;
	y_patchmin[0] = y_blockmin;
}

/*
 * Writing out the data to the DDR, 2x2 blocks which are processed
 * parallely are written into the DDR sequentially.
 */

template<int ROWS, int COLS, int WORDWIDTH_SRC, int WORDWIDTH_DST, int WORDWIDTH_T>
void xFWriteOutPatchMaxoutsize(XF_SNAME(WORDWIDTH_SRC) lbuf_out[][BUFSIZE], unsigned long long int *addr_out, int16_t y_index, int16_t x_index, ap_uint<9> max_output_range, int16_t out_block_height, int16_t out_block_width,int16_t img_height, int16_t img_width)

{
#pragma HLS INLINE off
	int outindexX0,outindexY0,inindexY0;
	int outindexX1,outindexY1,inindexY1;
	int outindexX2,outindexY2,inindexY2;
	int outindexX3,outindexY3,inindexY3;

	outindexX0  = outindexY0 = inindexY0 = 0;
	outindexX1  = outindexY1 = inindexY1 = 0;
	outindexX2  = outindexY2 = inindexY2 = 0;
	outindexX3  = outindexY3 = inindexY3 = 0;

	int16_t writeblocksize02,writeblocksize13;
	int16_t loopbound01,loopbound23;
	int16_t y_out = 0;

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

	outindexX0 = (x_index>>3);
	outindexY0 =  y_index*(img_width>>3) + outindexX0;
	inindexY0 = 0;

	WRITE_OUT_BLOCK0:for ( y_out = 0; y_out < (loopbound01); y_out++)
	{
#pragma HLS loop_tripcount  min=120 max=120 avg=120
#if _XF_SYNTHESIS_
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[0][inindexY0],addr_out+outindexY0,120);
#else
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[0][inindexY0],addr_out+outindexY0,writeblocksize02);
#endif
		outindexY0 += (img_width>>3);
		inindexY0 += (max_output_range>>4);
	}

	outindexX1 = (x_index>>3)+(writeblocksize02>>3);
	outindexY1 =  y_index*(img_width>>3) + outindexX1;
	inindexY1 = 0;

	WRITE_OUT_BLOCK1:for ( y_out = 0; y_out < (loopbound01); y_out++)
	{
#pragma HLS loop_tripcount  min=56 max=56 avg=56
#if _XF_SYNTHESIS_
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[1][inindexY1],addr_out+outindexY1,120);
#else
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[1][inindexY1],addr_out+outindexY1,writeblocksize13);
#endif
		outindexY1 += (img_width>>3);
		inindexY1 += (max_output_range>>4);
	}

	outindexX2 = (x_index>>3);
	outindexY2 = (y_index + (max_output_range>>1))*(img_width>>3) + outindexX2;
	inindexY2 = 0;

	WRITE_OUT_BLOCK2:for ( y_out = 0; y_out < loopbound23; y_out++)
	{
#pragma HLS loop_tripcount  min=56 max=56 avg=56
#if _XF_SYNTHESIS_
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[2][inindexY2],addr_out+outindexY2,120);
#else
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[2][inindexY2],addr_out+outindexY2,writeblocksize02);
#endif
		outindexY2 += (img_width>>3);
		inindexY2 += (max_output_range>>4);
	}

	outindexX3 = (x_index>>3)+(writeblocksize02>>3);
	outindexY3 = (y_index + (max_output_range>>1))*(img_width>>3) + outindexX3;
	inindexY3 = 0;

	WRITE_OUT_BLOCK3:for ( y_out = 0; y_out < loopbound23; y_out++)
	{
#pragma HLS loop_tripcount  min=56 max=56 avg=56
#if _XF_SYNTHESIS_
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[3][inindexY3],addr_out+outindexY3,120);
#else
		xFCopyBlockMemoryOut1<120,WORDWIDTH_DST>(&lbuf_out[3][inindexY3],addr_out+outindexY3,writeblocksize13);
#endif
		outindexY3 += (img_width>>3);
		inindexY3 += (max_output_range>>4);
	}
}

/*
 * Organize the read row from the input
 * in the respective BRAMs
 */
template<int ROWS, int COLS, int WORDWIDTH_SRC, int XF_48SP>
void xFReadInputPatchBlocks_pingpong(unsigned long long int *gmem,XF_PTNAME(XF_48SP) *transform_matrix,int16_t y_index,int16_t x_index,ap_uint<9> blocksize,int16_t *y_patchmin,int16_t *x_patchmin,
		int16_t y_min[], int16_t x_min[],XF_SNAME(WORDWIDTH_SRC) bufs[][AF_BRAMSIZE], ap_uint<13> img_height, ap_uint<13> img_width)
{
#pragma HLS INLINE off
	XF_SNAME(WORDWIDTH_SRC) linebuf1[BLOCKSIZE_BY8],linebuf2[BLOCKSIZE_BY8];
	//unsigned long long int linebuf1[BLOCKSIZE_BY8],linebuf2[BLOCKSIZE_BY8];
	XF_SNAME(WORDWIDTH_SRC) PackedPixels;
	//unsigned long long int PackedPixels;
	int16_t y_max[4], x_max[4];

	xFFindInputPatchPosition<ROWS, COLS, XF_48SP>(transform_matrix,y_patchmin,x_patchmin, y_index, x_index, blocksize, img_height, img_width);
	xFCalculateBlockPositions<XF_48SP>(transform_matrix,y_index, x_index,y_patchmin,x_patchmin,y_min,x_min,y_max,x_max,blocksize);

	bool flag = false;
	int16_t i;
	int _offset;
	int16_t img_widthby8 = img_width>>3;
	//int16_t x_patchmin1 = *x_patchmin;
	//int16_t y_patchmin1 = *y_patchmin;
	int16_t x_patchminby8 = (*x_patchmin) >> 3;

	int16_t readblocksize;
	int16_t rowreads;
	ap_uint1_t block_flag = 0;


	if( (*x_patchmin>=0) && (*x_patchmin<img_width) && (*y_patchmin>=0) && (*y_patchmin<img_height) )
	{
		block_flag=1;
	}

	if(block_flag)
	{
		if((*y_patchmin+BLOCKSIZE) > img_height)
			rowreads = img_height - *y_patchmin;
		else
			rowreads = BLOCKSIZE;

		_offset = (*y_patchmin * img_widthby8) + x_patchminby8;
		if( ((*x_patchmin>>3)+32) > img_widthby8)
			readblocksize = (img_widthby8 - x_patchminby8)<<3;
		else
			readblocksize = BLOCKSIZE;

		xFCopyBlockMemoryIn1<BLOCKSIZE, WORDWIDTH_SRC>(gmem+_offset,linebuf1,readblocksize);

		READ_OUTERLOOP:
		for(i=1;i< rowreads;i++)
		{
#pragma HLS loop_tripcount  min=1 max=256 avg=256

			if((i+ *y_patchmin) < img_height)
			{
				_offset = _offset + img_widthby8;
				if(!flag)
				{
					xFCopyBlockMemoryIn1<BLOCKSIZE, WORDWIDTH_SRC>(gmem+_offset,linebuf2,readblocksize);
					xFOrganizePackedData<WORDWIDTH_SRC>(linebuf1,*y_patchmin,*x_patchmin,y_min, x_min,y_max, x_max,i-1,bufs,readblocksize);
					flag = true;
				}
				else
				{
					xFCopyBlockMemoryIn1<BLOCKSIZE, WORDWIDTH_SRC>(gmem+_offset,linebuf1,readblocksize);
					xFOrganizePackedData<WORDWIDTH_SRC>(linebuf2,*y_patchmin,*x_patchmin,y_min, x_min,y_max, x_max,i-1,bufs,readblocksize);
					flag = false;
				}
			}else
				break;
		}
		if(flag)
			xFOrganizePackedData<WORDWIDTH_SRC>(linebuf2,*y_patchmin,*x_patchmin,y_min, x_min,y_max, x_max,i-1,bufs,readblocksize);
		else
			xFOrganizePackedData<WORDWIDTH_SRC>(linebuf1,*y_patchmin,*x_patchmin,y_min, x_min,y_max, x_max,i-1,bufs,readblocksize);
	}
}

/*
 * Affine Transform Kernel function:
 *
 *
 */
template<int ROWS, int COLS, int DEPTH,int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST, int WORDWIDTH_T>
void xFWarpAffine(unsigned long long int *source, unsigned long long int *dst, uint16_t img_height, uint16_t img_width, uint16_t interpolation, XF_PTNAME(WORDWIDTH_T) *transformation_matrix)
{


#pragma HLS inline
	assert(((interpolation == XF_INTERPOLATION_NN) || (interpolation == XF_INTERPOLATION_BILINEAR))
			&& "Interpolation supported are XF_INTERPOLATION_NN and XF_INTERPOLATION_BILINEAR");

	assert(((img_height <= ROWS ) && (img_width <= COLS)) && "ROWS and COLS should be greater than input image");

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
	int16_t x_min[4], y_min[4];
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

	ap_int<14> out_block_width, out_block_height;
	ap_uint<9> max_output_range=0;
	ap_uint1_t process_flag;
	ap_uint<9> write_count=0;

	ap_uint16_t tempreg;
	//////////////////////////////////////////////
	XF_SNAME(WORDWIDTH_SRC) bufs1[8][AF_BRAMSIZE],bufs2[8][AF_BRAMSIZE];
	//unsigned long long int bufs1[8][AF_BRAMSIZE],bufs2[8][AF_BRAMSIZE];
#pragma HLS ARRAY_PARTITION variable=bufs1 block factor=8 dim=1
#pragma HLS ARRAY_PARTITION variable=bufs2 block factor=8 dim=1

	ap_uint<13> imgheight = (ap_uint<13>)img_height;
	ap_uint<13> imgwidth = (ap_uint<13>)img_width;


	XF_PTNAME(XF_48SP) transform_matrix[6];
#pragma HLS ARRAY_PARTITION variable=transform_matrix complete dim=0

	for(ap_uint<3> i=0;i<6;i++)
		transform_matrix[i] = (XF_PTNAME(XF_48SP))(transformation_matrix[i] * TWO_POW_32);

	/* maximum output size calculation*/
	max_output_range = xFFindMaxOutSize<XF_48SP>(transform_matrix);

	assert(( max_output_range >= 48 )
			&& "Scaling factor in the transformation matrix should be greater than 0.25");

	max_out_blocks_col = (imgwidth/max_output_range);

	if((img_width-(max_out_blocks_col*max_output_range))>0)
		max_out_blocks_col += 1;

	max_out_blocks_row = (imgheight/max_output_range);
	if((img_height-(max_out_blocks_row*max_output_range))>0)
		max_out_blocks_row += 1;
	ap_uint<1> flag_X;

	for(ap_uint<4> j=0;j<8;j++)
	{
		for(ap_uint<13> i=0;i<AF_BRAMSIZE;i++)
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

	y_index = 0;

	ROWLOOP:
	for (processed_blocks_row = 0; processed_blocks_row < max_out_blocks_row; processed_blocks_row++)
	{
#pragma HLS loop_tripcount  min=5 max=5 avg=5
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
#pragma HLS loop_tripcount  min=10 max=10 avg=10
			if(img_width>=((write_count)*max_output_range+max_output_range))
				out_block_width = max_output_range;
			else
				out_block_width = imgwidth -((write_count)*max_output_range);


			if(processed_blocks_col == 0)
			{

				xFReadInputPatchBlocks_pingpong<ROWS,COLS,WORDWIDTH_SRC, XF_48SP>(source,transform_matrix,y_index, x_index,max_output_range,&y_patchmin,&x_patchmin, y_min, x_min, bufs1, imgheight, imgwidth);
			}
			else if(processed_blocks_col == 1) // One set of data is already read, hence it needs to be processed, and a new set of data has to be read
			{
				xFReadInputPatchBlocks_pingpong<ROWS,COLS,WORDWIDTH_SRC, XF_48SP>(source,transform_matrix,y_index, x_index,max_output_range,&y_patchmin,&x_patchmin, y_min, x_min, bufs2, imgheight, imgwidth);

				xFAffineProcessFunction<ROWS,COLS,NPC,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(transform_matrix, y_patchmin1,x_patchmin1,y_min1, x_min1,
						y_index, x_index1, lbuf_out1, max_output_range, bufs1, interpolation, imgheight, imgwidth, dst);
			}
			else if(processed_blocks_col == max_out_blocks_col+1 || processed_blocks_col == max_out_blocks_col ) // The last extra two iterations for flushing the already read data
			{
				if(flag_X == 0)
				{
					//Process
					xFAffineProcessFunction<ROWS,COLS,NPC,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(transform_matrix, y_patchmin1,x_patchmin1,y_min1, x_min1,
							y_index, x_index1, lbuf_out2, max_output_range, bufs2, interpolation, imgheight, imgwidth, dst);

					write_count++;
					// Write
					xFWriteOutPatchMaxoutsize<ROWS,COLS,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(lbuf_out1, dst, y_index, x_index2, max_output_range, out_block_height, out_block_width,imgheight, imgwidth);
				}
				else if(flag_X == 1)
				{
					//Process
					xFAffineProcessFunction<ROWS,COLS,NPC,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(transform_matrix, y_patchmin1,x_patchmin1,y_min1, x_min1,
							y_index, x_index1, lbuf_out1, max_output_range, bufs1, interpolation, imgheight, imgwidth, dst);

					write_count++;
					// Write
					xFWriteOutPatchMaxoutsize<ROWS,COLS,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(lbuf_out2, dst, y_index, x_index2, max_output_range, out_block_height, out_block_width,imgheight, imgwidth);
				}
			}
			else // For all the other cases, do read, process and then write in the respective order on the data available in ping pong fashion
			{
				if (flag_X == 0)
				{
					// Read
					xFReadInputPatchBlocks_pingpong<ROWS,COLS,WORDWIDTH_SRC, XF_48SP>(source,transform_matrix,y_index, x_index,max_output_range,&y_patchmin,&x_patchmin, y_min, x_min, bufs1, imgheight, imgwidth);

					//Process
					xFAffineProcessFunction<ROWS,COLS,NPC,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(transform_matrix, y_patchmin1,x_patchmin1,y_min1, x_min1,
							y_index, x_index1, lbuf_out2, max_output_range, bufs2, interpolation, imgheight, imgwidth, dst);

					write_count++;
					// Write
					xFWriteOutPatchMaxoutsize<ROWS,COLS,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(lbuf_out1, dst, y_index, x_index2, max_output_range, out_block_height, out_block_width,imgheight, imgwidth);
				}
				else if (flag_X == 1)
				{
					// Read
					xFReadInputPatchBlocks_pingpong<ROWS,COLS,WORDWIDTH_SRC, XF_48SP>(source,transform_matrix,y_index, x_index,max_output_range,&y_patchmin,&x_patchmin, y_min, x_min, bufs2, imgheight, imgwidth);

					//Process
					xFAffineProcessFunction<ROWS,COLS,NPC,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(transform_matrix, y_patchmin1,x_patchmin1,y_min1, x_min1,
							y_index, x_index1, lbuf_out1, max_output_range, bufs1, interpolation, imgheight, imgwidth, dst);

					write_count++;
					// Write
					xFWriteOutPatchMaxoutsize<ROWS,COLS,WORDWIDTH_SRC, WORDWIDTH_DST, XF_48SP>(lbuf_out2, dst, y_index, x_index2, max_output_range, out_block_height, out_block_width,imgheight, imgwidth);
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
	} // End of Row Loop

} // End of Affine Kernel Function

#pragma SDS data zero_copy("_src.data"[0:"_src.size"], "_dst.data"[0:"_src.size"])
#pragma SDS data access_pattern(transformation_matrix:SEQUENTIAL)
#pragma SDS data copy(transformation_matrix[0:6])

template<int INTERPOLATION_TYPE,int SRC_T,int ROWS, int COLS, int NPC=1>
void xFwarpAffine(xF::Mat<SRC_T, ROWS, COLS, XF_NPPC8> & _src, xF::Mat<SRC_T, ROWS, COLS, XF_NPPC8> & _dst, float* transformation_matrix)
{

	xFWarpAffine<ROWS,COLS,XF_DEPTH(SRC_T,XF_NPPC8),NPC,XF_WORDWIDTH(SRC_T,XF_NPPC8),XF_WORDWIDTH(SRC_T,XF_NPPC8),XF_32FP> ((unsigned long long int*)_src.data, (unsigned long long int*)_dst.data,_src.rows,_src.cols,INTERPOLATION_TYPE,transformation_matrix);

}
#endif
