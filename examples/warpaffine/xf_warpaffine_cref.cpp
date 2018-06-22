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


#include "xf_headers.h"
#include "ap_int.h"

#define PI 3.141592654
#define TWO_POW_16	65536
typedef ap_int<24> int24_t;

void find_transform_matrix_tb(float x_rot, float y_rot, float x_expan, float y_expan, float x_move, float y_move,
		int x_offset, int y_offset, float *transform_matrix)
{
	float A1, B1, C1, D1, E1, F1;
	float Af, Bf, Cf, Df, Ef, Ff;
	float x_constf, y_constf;
	float det;

	A1 =  x_expan * cos(x_rot*PI/180.0);
	B1 = -y_expan * sin(y_rot*PI/180.0);
	C1 =  x_expan * sin(x_rot*PI/180.0);
	D1 =  y_expan * cos(y_rot*PI/180.0);
	E1 =  x_move;
	F1 =  y_move;

	/* determination of inverse affine transformation */
	det = A1*D1 - B1*C1;
	if (det == 0)
	{
		Af = 1.0;
		Bf = 0.0;
		Cf = 0.0;
		Df = 1.0;
		Ef = -E1;
		Ff = -F1;
	} else
	{
		Af =  D1/det;
		Bf = -B1/det;
		Cf = -C1/det;
		Df =  A1/det;
		Ef = -Af*E1-Bf*F1;
		Ff = -Cf*E1-Df*F1;
	}

	x_constf = Ef + x_offset - x_offset*Af - y_offset*Bf;
	y_constf = Ff + y_offset - x_offset*Cf - y_offset*Df;

	transform_matrix[0] = Af;
	transform_matrix[1] = Bf;
	transform_matrix[2] = x_constf;
	transform_matrix[3] = Cf;
	transform_matrix[4] = Df;
	transform_matrix[5] = y_constf;
}

void warpaffine_cref(cv::Mat &input, cv::Mat &output,float *M,uint32_t interpolation)
{
	uchar pixel,pixel1,pixel2,pixel3,pixel4;

	float fracX,fracY;
	int Xoffset,Yoffset,offset;

	unsigned char *testin_ptr=(unsigned char *)malloc(input.cols*input.rows*sizeof(unsigned char));
	unsigned char *testout_ptr=(unsigned char *)malloc(input.cols*input.rows*sizeof(unsigned char));
	for(int i=0;i<input.rows;i++)
	{
		for(int j=0;j<input.cols;j++)
		{
			testin_ptr[(i*input.cols)+j]=input.at<unsigned char>(i,j);
		}
	}




	for(int y=0;y<input.rows;y++)
	{
		for(int x=0;x<input.cols;x++)
		{
			float X0 = M[0]*x + M[1]*(y) + M[2] + 0.0001;
			float Y0 = M[3]*x + M[4]*(y) + M[5] + 0.0001;

			int inX = cv::saturate_cast<int>(X0) ;
			int inY = cv::saturate_cast<int>(Y0);
			short int inX_short = cv::saturate_cast<short>(inX);
			short int inY_short = cv::saturate_cast<short>(inY);

			if(inX > 0 && inY >0 && inX+1 < input.cols && inY+1 < input.rows)
			{
				int inIndex = inY_short * input.cols + inX_short;
				int outIndex = y* input.cols + x;

				if(interpolation)
				{
					fracX = (X0 - floor(X0));
					fracY = (Y0 - floor(Y0));

					if(X0 < inX_short)
					{
						Xoffset = -1;
					}else
						Xoffset = 0;

					if(Y0 < inY_short)
					{
						Yoffset = -input.cols;
					}else
						Yoffset = 0;

					offset = Xoffset + Yoffset;

					/*	pixel1 = input.data[inIndex + offset];
					pixel2 = input.data[inIndex + offset + 1];
					pixel3 = input.data[inIndex + offset + input.cols];
					pixel4 = input.data[inIndex + offset + input.cols + 1];*/


					pixel1 = testin_ptr[inIndex + offset];
					pixel2 = testin_ptr[inIndex + offset + 1];
					pixel3 = testin_ptr[inIndex + offset + input.cols];
					pixel4 = testin_ptr[inIndex + offset + input.cols + 1];




					pixel = (uchar) ( (1-fracY)*(1-fracX)*pixel1 + fracX*(1-fracY)*pixel2 + fracY*(1-fracX)*pixel3 + fracX*fracY*pixel4 );
				}
				else
					//pixel = input.data[inIndex];
					pixel = testin_ptr[inIndex];
				//output.data[outIndex] = pixel;
				testout_ptr[outIndex] = pixel;
			}else if( (((inY+1)==input.rows)&&(inX>=0)&&((inX)<input.cols)) ||
					(((inX+1)==input.cols)&&(inY>=0)&&(inY<input.rows))  ||
					(((inY)==0)&&(inX>=0)&&((inX)<input.cols)) ||
					(((inX)==0)&&(inY>=0)&&(inY<input.rows))  )
			{
				int inIndex = inY_short * input.cols + inX_short;
				int outIndex = y* input.cols + x;
				pixel = testout_ptr[outIndex] = testin_ptr[inIndex];
			}
		}
	}

	for(int i=0;i<output.rows;i++)
	{
		for(int j=0;j<output.cols;j++)
		{
			output.at<unsigned char>(i,j)=testout_ptr[(i*output.cols)+j];
		}
	}






}
