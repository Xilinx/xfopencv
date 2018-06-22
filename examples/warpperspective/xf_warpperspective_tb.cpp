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
#include "xf_warpperspective_config.h"


#define ERROR_THRESHOLD 1

/*			Reference function 			*/
void warp_perspective_cref(cv::Mat &in_gray, cv::Mat &out, float *transform_matrix,uint32_t interpolation)
{
	cv::Point2f inputQuad[4];
	// Output Quadilateral or World plane coordinates
	cv::Point2f outputQuad[4];

	double M[9];
	float Xfrac,Yfrac;
	int inIndex,outIndex;
	int Xoffset,Yoffset,offset;
	uchar pixel,pixel1,pixel2,pixel3,pixel4;
	cv::Mat lambda( 2, 4, CV_32FC1 );
	cv::Mat matM(3, 3, CV_64F, M);

	// Set the lambda matrix the same type and size as input
	lambda = cv::Mat::zeros( in_gray.rows, in_gray.cols, in_gray.type() );

	// The 4 points that select quadilateral on the input , from top-left in clockwise order
	// These four pts are the sides of the rect box used as input
	inputQuad[0] = cv::Point2f( XTL,YTL );
	inputQuad[1] = cv::Point2f( XTR,YTR);
	inputQuad[2] = cv::Point2f( XBR,YBR);
	inputQuad[3] = cv::Point2f( XBL,YBL  );
	// The 4 points where the mapping is to be done , from top-left in clockwise order
	outputQuad[0] = cv::Point2f( 0,0 );
	outputQuad[1] = cv::Point2f( in_gray.cols-1,0);
	outputQuad[2] = cv::Point2f( in_gray.cols-1,in_gray.rows-1);
	outputQuad[3] = cv::Point2f( 0,in_gray.rows-1  );

	// Get the Perspective Transform Matrix i.e. lambda
	lambda = getPerspectiveTransform( inputQuad, outputQuad );

	cv::Mat M0 = lambda;

	//CV_Assert( (M0.type() == CV_32F || M0.type() == CV_64F) && M0.rows == 3 && M0.cols == 3 );
	M0.convertTo(matM, matM.type());

	invert(matM, matM);

	//for(int i=0;i<9;i++)
	//	transform_matrix[i] = (float)M[i];


	transform_matrix[0] = M[0] =  0.86;
	transform_matrix[1] = M[1] =  0.5;
	transform_matrix[2] = M[2] = -141.38;
	transform_matrix[3] = M[3] = -0.5;
	transform_matrix[4] = M[4] =  0.86;
	transform_matrix[5] = M[5] =  552.34;
	transform_matrix[6] = M[6] =  0;
	transform_matrix[7] = M[7] =  0;
	transform_matrix[8] = M[8] =  1;





	for(int y=0;y<in_gray.rows;y++)
	{
		for(int x=0;x<in_gray.cols;x++)
		{
			float X0 = M[0]*x + M[1]*(y) + M[2];
			float Y0 = M[3]*x + M[4]*(y) + M[5];
			float W0 = M[6]*x + M[7]*(y) + M[8];

			float W = W0 ? (1.)/W0 : 0;

			float fX = std::max((float)INT_MIN, std::min((float)INT_MAX, (X0)*W));
			float fY = std::max((float)INT_MIN, std::min((float)INT_MAX, (Y0)*W));
			int inX = cv::saturate_cast<int>(fX);
			int inY = cv::saturate_cast<int>(fY);
			short int inX_short = cv::saturate_cast<short>(inX);
			short int inY_short = cv::saturate_cast<short>(inY);

			if(inX_short > 0 && inY_short >0 && inX_short+1 < in_gray.cols && inY_short+1 < in_gray.rows)
			{
				inIndex = inY_short * in_gray.cols + inX_short;
				outIndex = y* in_gray.cols + x;

				if(interpolation)
				{
					Xfrac = (fX - floor(fX));
					Yfrac = (fY - floor(fY));

					pixel1 = in_gray.data[inIndex];
					pixel2 = in_gray.data[inIndex + 1];
					pixel3 = in_gray.data[inIndex + in_gray.cols];
					pixel4 = in_gray.data[inIndex + in_gray.cols + 1];

					pixel = (uchar) ( (1-Yfrac)*(1-Xfrac)*pixel1 + Xfrac*(1-Yfrac)*pixel2 + Yfrac*(1-Xfrac)*pixel3 + Xfrac*Yfrac*pixel4 );


				}
				else
					pixel = in_gray.data[inIndex];

				out.data[outIndex] = pixel;
			}
			else if( (((inY_short+1)==in_gray.rows)&&(inX_short>=0)&&((inX_short)<in_gray.cols)) ||
					(((inX_short+1)==in_gray.cols)&&(inY_short>=0)&&(inY_short<in_gray.rows))  ||
					(((inY_short)==0)&&(inX_short>=0)&&((inX_short)<in_gray.cols)) ||
					(((inX_short)==0)&&(inY_short>=0)&&(inY_short<in_gray.rows))  )
			{
				inIndex = inY_short * in_gray.cols + inX_short;
				outIndex = y* in_gray.cols + x;

				pixel = out.data[outIndex] = in_gray.data[inIndex];
			}

		}

	}


}

int main(int argc,char **argv){
	cv::Mat in_img,img_gray,hls_out_img,ocv_out_img, diff_img;
	unsigned short int img_height,img_width;
	uint32_t interpolation;

	if(argc != 2){
		printf("Usage : <executable> <input image> \n");
		return -1;
	}
	in_img = cv::imread(argv[1],0);
	if(!in_img.data)
	{
		printf("Failed to load the image ... %s\n!", argv[1]);
		return -1;
	}
	//cvtColor(in_img,img_gray,CV_BGR2GRAY);
	imwrite("input.png",in_img);
	hls_out_img.create(in_img.rows,in_img.cols,in_img.depth());
	ocv_out_img.create(in_img.rows,in_img.cols,in_img.depth());
	diff_img.create(in_img.rows,in_img.cols,in_img.depth());

	float *transform_matrix;

#if __SDSCC__
	transform_matrix=(float*)sds_alloc(9*sizeof(float));
#else
	transform_matrix=(float*)malloc(9*sizeof(float));
#endif

	interpolation = BILINEAR?XF_INTERPOLATION_BILINEAR:XF_INTERPOLATION_NN;
	warp_perspective_cref(in_img,ocv_out_img,transform_matrix,interpolation);



	static xf::Mat<XF_8UC1,HEIGHT,WIDTH,XF_NPPC8> imgInput(in_img.rows,in_img.cols);
	static xf::Mat<XF_8UC1,HEIGHT,WIDTH,XF_NPPC8> imgOutput(in_img.rows,in_img.cols);

	imgInput.copyTo(in_img.data);




#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif

	perspective_accel(imgInput, imgOutput,transform_matrix);

#if __SDSCC__

	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	//hls_out_img.data = imgOutput.copyFrom();
	// Write output image
	xf::imwrite("hls_out.jpg",imgOutput);

	uchar* out_ptr = (uchar *)imgOutput.data;
	uchar* ocv_ref_ptr = (uchar *)ocv_out_img.data;
	uchar* diff_ptr = (uchar *)diff_img.data;
	uchar diff;

	int pix_cnt = 0;
	float per_error;

	uchar max_error = 0,min_error = 255;

	for( int j = 0; j < in_img.rows ; j++ ){
		for( int i = 0; i < in_img.cols; i++ ){

			diff = abs(*out_ptr++ - *ocv_ref_ptr++);

			if(diff>ERROR_THRESHOLD)
			{
				*diff_ptr++ = abs(diff);
				pix_cnt++;
				if (diff > max_error)
					max_error = diff;
				if (diff < min_error)
					min_error = diff;
			}
			else
			{
				*diff_ptr++ = 0;
			}
		}
	}
	per_error = 100*(float)pix_cnt/(in_img.rows*in_img.cols);
	printf("\nMin Error = %d Max Error = %d\n",min_error,max_error);
	printf("Percentage of pixels above error threshold = %f\n",per_error);



	//imwrite("testcase_hls.png", hls_out_img);
	imwrite("testcase_ocv.png", ocv_out_img);
	imwrite("testcase_diff.png", diff_img);

	if(per_error > 2.0f)
		return 1;

	return 0;
}
