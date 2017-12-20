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

#include "xf_headers.h"
#include "xf_warpaffine_config.h"



#define ERROR_THRESHOLD 1

/*			Referance function 			*/
void warpaffine_cref(cv::Mat &, cv::Mat &,float *,uint32_t);
void find_transform_matrix_tb(float ,float ,float ,float ,float ,float ,int , int , float *);

int main(int argc,char **argv)
{

	uint32_t interpolation=1;
	cv::Mat in_img,img_gray, hls_out_img,diff_img,ocv_out_img;
	float *transform_matrix;//[6];
	char output_image[300];
	int x_move, y_move;
	float x_rot, y_rot,x_expan, y_expan;
	int16_t x_offset,y_offset;
	unsigned char *ptr_GRAY,*ptr_result;

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


	hls_out_img.create(in_img.rows,in_img.cols,in_img.depth());
	ocv_out_img.create(in_img.rows,in_img.cols,in_img.depth());
	diff_img.create(in_img.rows,in_img.cols,in_img.depth());


	x_rot   = y_rot = ANGLE;
	x_expan = y_expan = SCALEVAL;
	x_move  =1;//  -960;
	y_move =1;// 540;
	y_offset = (LINES>>1);
	x_offset = (WIDTH>>1);

	
#if __SDSCC__
		transform_matrix=(float*)sds_alloc(6*sizeof(float));
#else
		transform_matrix=(float*)malloc(6*sizeof(float));
#endif
	
	
	interpolation = BILINEAR?XF_INTERPOLATION_BILINEAR:XF_INTERPOLATION_NN;
#if BILINEAR
#define INTERPOLATION XF_INTERPOLATION_BILINEAR
#else
#define INTERPOLATION XF_INTERPOLATION_NN
#endif
	find_transform_matrix_tb(x_rot, y_rot, x_expan, y_expan, x_move, y_move, x_offset, y_offset, transform_matrix);

	warpaffine_cref(in_img, ocv_out_img,transform_matrix,interpolation);

		xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC8> imgInput(in_img.rows,in_img.cols);
		xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC8> imgOutput(in_img.rows,in_img.cols);

		//imgInput.copyTo(img_gray.data);
		imgInput = xf::imread<XF_8UC1, HEIGHT, WIDTH, XF_NPPC8>(argv[1], 0);	

#if __SDSCC__
		perf_counter hw_ctr;
 hw_ctr.start();
#endif

		warpaffine_accel(imgInput, imgOutput, transform_matrix);

#if __SDSCC__
hw_ctr.stop();
uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	
		//hls_out_img.data = imgOutput.copyFrom();
		// Write output image
		xf::imwrite("hls_out.jpg",imgOutput);



		//imwrite("testcase_hls.png", hls_out_img);
		imwrite("testcase_ocv.png", ocv_out_img);
		

		//uchar* out_ptrcmp = (uchar *)hls_out_img.data;
		uchar* out_ptrcmp = (uchar *)imgOutput.data;
		uchar* ocv_ref_ptrcmp = (uchar *)ocv_out_img.data;
		uchar* diff_ptrcmp= (uchar *)diff_img.data;
		uchar diff1;

		int pix_cnt = 0;
		float per_error;

		uchar max_error = 0,min_error = 255;

		for( int j = 0; j < in_img.rows ; j++ ){
			for( int i = 0; i < in_img.cols; i++ ){

				diff1 = abs(*out_ptrcmp++ - *ocv_ref_ptrcmp++);

				if(diff1 > ERROR_THRESHOLD)
				{
					

					*diff_ptrcmp++ = abs(diff1);
					pix_cnt++;
					if (diff1 > max_error)
						max_error = diff1;
					if (diff1 < min_error)
						min_error = diff1;
				}
				else
				{
					*diff_ptrcmp++ = 0;
				}
			}
		}

		imwrite("testcase_diff.png", diff_img);
		per_error = 100*(float)pix_cnt/(in_img.rows*in_img.cols);
		printf("Min Error = %d Max Error = %d\n",min_error,max_error);
		printf("Percentage of pixels above error threshold = %f pixels:%d\n",per_error,pix_cnt);



	if(per_error > 1){
		printf("Test Failed.../n");
		return -1;
	}

	return 0;
}

