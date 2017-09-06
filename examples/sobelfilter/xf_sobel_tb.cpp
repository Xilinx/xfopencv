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

#include <stdio.h>
#include <stdlib.h>

#include "xf_headers.h"
#include "xf_sobel_config.h"


int main(int argc, char** argv)
{
	if(argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}
	char in[100], in1[100], out_hlsx[100], out_ocvx[100];
	char out_errorx[100], out_hlsy[100], out_ocvy[100], out_errory[100];

	cv::Mat in_img, in_gray, diff, hlsx_32F, hlsy_32F;
	cv::Mat c_grad_x, c_grad_y;
	cv::Mat c_grad_x_1, c_grad_y_1;
	cv::Mat hls_grad_x, hls_grad_y;
	cv::Mat diff_grad_x, diff_grad_y;

	// reading in the color image
	in_img = cv::imread(argv[1], 1);

	if(in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n", in);
		return 0;
	}
	cvtColor(in_img, in_gray, CV_BGR2GRAY);

	///////////////// 	Opencv  Reference  ////////////////////////
	int scale = 1;
	int delta = 0;

#if (FILTER_WIDTH != 7)
	// create memory for output images
	hls_grad_x.create(in_gray.rows,in_gray.cols,CV_16S);
	hls_grad_y.create(in_gray.rows,in_gray.cols,CV_16S);
	diff_grad_x.create(in_gray.rows,in_gray.cols,CV_16S);
	diff_grad_y.create(in_gray.rows,in_gray.cols,CV_16S);
	int ddepth = CV_16S;
	typedef int16_t TYPE;
#endif

#if (FILTER_WIDTH == 7)
	// create memory for output images
	hls_grad_x.create(in_gray.rows,in_gray.cols,CV_32S);
	hls_grad_y.create(in_gray.rows,in_gray.cols,CV_32S);
	diff_grad_x.create(in_gray.rows,in_gray.cols,CV_32S);
	diff_grad_y.create(in_gray.rows,in_gray.cols,CV_32S);
	int ddepth = CV_32F;
	typedef  int TYPE;
#endif

	cv::Sobel( in_gray, c_grad_x_1, ddepth, 1, 0, FILTER_WIDTH, scale, delta, cv::BORDER_CONSTANT );
	cv::Sobel( in_gray, c_grad_y_1, ddepth, 0, 1, FILTER_WIDTH, scale, delta, cv::BORDER_CONSTANT );

	imwrite("out_ocvx.jpg", c_grad_x_1);
	imwrite("out_ocvy.jpg", c_grad_y_1);

	//////////////////	HLS TOP Function Call  ////////////////////////
#if (FILTER_WIDTH == 3 | FILTER_WIDTH == 5)
	ap_uint<8> *src_ptr = (ap_uint<8> *)in_gray.data;
	ap_uint<16> *dst_ptrx = (ap_uint<16> *)hls_grad_x.data;
	ap_uint<16>  *dst_ptry = (ap_uint<16>  *)hls_grad_y.data;
#elif (FILTER_WIDTH == 7)
	ap_uint<8> *src_ptr = (ap_uint<8> *)in_gray.data;
	ap_uint<32> *dst_ptrx = (ap_uint<32> *)hls_grad_x.data;
	ap_uint<32>  *dst_ptry = (ap_uint<32>  *)hls_grad_y.data;
#endif
	unsigned short height = in_gray.rows;
	unsigned short width = in_gray.cols;


	xf::Mat<IN_TYPE,HEIGHT,WIDTH,NPC1> imgInput(in_gray.rows,in_gray.cols);
	xf::Mat<OUT_TYPE,HEIGHT,WIDTH,NPC1> imgOutputx(in_gray.rows,in_gray.cols);
	xf::Mat<OUT_TYPE,HEIGHT,WIDTH,NPC1> imgOutputy(in_gray.rows,in_gray.cols);

	imgInput.copyTo(in_gray.data);

	#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
	#endif

	sobel_accel(imgInput, imgOutputx,imgOutputy);

	#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	hls_grad_x.data = (unsigned char *)imgOutputx.copyFrom();
	hls_grad_y.data = (unsigned char *)imgOutputy.copyFrom();



	imwrite("out_hlsx.jpg", hls_grad_x);
	imwrite("out_hlsy.jpg", hls_grad_y);


	//////////////////  Compute Absolute Difference ////////////////////
#if (FILTER_WIDTH == 3 | FILTER_WIDTH == 5)
	absdiff(c_grad_x_1, hls_grad_x, diff_grad_x);
	absdiff(c_grad_y_1, hls_grad_y, diff_grad_y);
#endif

#if (FILTER_WIDTH == 7)
	c_grad_x_1.convertTo(c_grad_x, CV_32S);
	c_grad_y_1.convertTo(c_grad_y, CV_32S);
	absdiff(c_grad_x, hls_grad_x, diff_grad_x);
	absdiff(c_grad_y, hls_grad_y, diff_grad_y);
#endif

	imwrite("out_errorx.jpg", diff_grad_x);
	imwrite("out_errory.jpg", diff_grad_y);

	// Find minimum and maximum differences.
	double minval=256,maxval=0;
	double minval1=256,maxval1=0;
	int cnt = 0, cnt1 =0;

	for(int i=0;i<in_img.rows;i++)
	{
		for(int j=0;j<in_img.cols;j++)
		{
			TYPE v = diff_grad_y.at<TYPE>(i,j);
			TYPE v1 = diff_grad_x.at<TYPE>(i,j);
			if (v>0)
				cnt++;
			if (minval > v )
				minval = v;
			if (maxval < v)
				maxval = v;
			if (v1>0)
				cnt1++;
			if (minval1 > v1 )
				minval1 = v1;
			if (maxval1 < v1)
				maxval1 = v1;
		}
	}
	float err_per = 100.0*(float)cnt/(in_img.rows*in_img.cols);
	float err_per1 = 100.0*(float)cnt1/(in_img.rows*in_img.cols);

	fprintf(stderr,"Minimum error in intensity = %f\n Maximum error in intensity = %f\n Percentage of pixels above error threshold = %f\n",minval,maxval,err_per);
	fprintf(stderr,"Minimum error in intensity = %f\n Maximum error in intensity = %f\n Percentage of pixels above error threshold = %f\n",minval1,maxval1,err_per1);

	in_img.~Mat();
	in_gray.~Mat();
	c_grad_x.~Mat();
	c_grad_y.~Mat();;
	hls_grad_x.~Mat();
	hls_grad_y.~Mat();
	diff_grad_x.~Mat();
	diff_grad_y.~Mat();
	int ret=0;
	if(err_per > 0.0f)
	{
		printf("Test failed .... !!!\n");
		ret = 1;
	}else
	{
		printf("Test Passed .... !!!\n");
		ret = 0;
	}

	return ret;
}

