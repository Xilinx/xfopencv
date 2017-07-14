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
#include "xf_scharr_config.h"


int main(int argc, char** argv)
{
	if(argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, in_gray;
	cv::Mat c_grad_x, c_grad_y;
	cv::Mat hls_grad_x, hls_grad_y;
	cv::Mat diff_grad_x, diff_grad_y;

	// reading in the color image
	in_img = cv::imread(argv[1],1);

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image\n");
		return 0;
	}

	cvtColor(in_img, in_gray, CV_BGR2GRAY);

	// create memory for output images
	c_grad_x.create(in_gray.rows,in_gray.cols,CV_16S);
	c_grad_y.create(in_gray.rows,in_gray.cols,CV_16S);
	hls_grad_x.create(in_gray.rows,in_gray.cols,CV_16S);
	hls_grad_y.create(in_gray.rows,in_gray.cols,CV_16S);
	diff_grad_x.create(in_gray.rows,in_gray.cols,CV_16S);
	diff_grad_y.create(in_gray.rows,in_gray.cols,CV_16S);

	////////////    Opencv Reference    //////////////////////
	int scale = 1;
	int delta = 0;
	int ddepth = 3;

	Scharr(in_gray, c_grad_x, ddepth, 1, 0, scale, delta, cv::BORDER_CONSTANT );
	Scharr(in_gray, c_grad_y, ddepth, 0, 1, scale, delta, cv::BORDER_CONSTANT );

	imwrite("out_ocvx.jpg", c_grad_x);
	imwrite("out_hlsy.jpg", c_grad_y);

	//////////////////	HLS TOP Function Call  ////////////////////////


	unsigned short height = in_gray.rows;
	unsigned short width = in_gray.cols;




	xF::Mat<XF_8UC1,HEIGHT,WIDTH,NPC1> imgInput(in_gray.rows,in_gray.cols);
	xF::Mat<XF_16UC1,HEIGHT,WIDTH,NPC1> imgOutputx(in_gray.rows,in_gray.cols);
	xF::Mat<XF_16UC1,HEIGHT,WIDTH,NPC1> imgOutputy(in_gray.rows,in_gray.cols);

	imgInput.copyTo(in_gray.data);

	#if __SDSCC__
	TIME_STAMP_INIT
	#endif

	//xFScharr<XF_BORDER_CONSTANT,XF_8UC1,XF_16UC1,HEIGHT, WIDTH,NPC1>(imgInput, imgOutputx,imgOutputy);
	scharr_accel(imgInput, imgOutputx,imgOutputy);

	#if __SDSCC__
	TIME_STAMP
	#endif


	hls_grad_x.data = (unsigned char*)imgOutputx.copyFrom();
	hls_grad_y.data = (unsigned char*) imgOutputy.copyFrom();


	imwrite("out_hlsx.jpg", hls_grad_x);
	imwrite("out_hlsy.jpg", hls_grad_y);


	//////////////////  Compute Absolute Difference ////////////////////
	absdiff(c_grad_x, hls_grad_x, diff_grad_x);
	absdiff(c_grad_y, hls_grad_y, diff_grad_y);

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
			int16_t v = diff_grad_y.at<int16_t>(i,j);
			int16_t v1 = diff_grad_x.at<int16_t>(i,j);
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
	int ret  = 0;
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


