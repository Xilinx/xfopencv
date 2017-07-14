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
#include "xf_hist_equalize_config.h"



int main(int argc, char** argv)
{

	if(argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, in_gray, out_img, ocv_ref, diff;

	// reading in the color image
	in_img = cv::imread(argv[1], 1);

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image\n");
		return 0;
	}

	cvtColor(in_img, in_gray, CV_BGR2GRAY);

	// create memory for output images
	out_img.create(in_gray.rows, in_gray.cols, in_gray.depth());
	ocv_ref.create(in_gray.rows, in_gray.cols, in_gray.depth());

	///////////////// 	Opencv  Reference  ////////////////////////
	cv::equalizeHist(in_gray, ocv_ref);
	imwrite("out_ocv.jpg", ocv_ref);



	xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX> imgInput(in_gray.rows,in_gray.cols);
	xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX> imgInput1(in_gray.rows,in_gray.cols);
	xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPIX> imgOutput(in_gray.rows,in_gray.cols);

	imgInput.copyTo(in_gray.data);
	imgInput1.copyTo(in_gray.data);
	#if __SDSCC__
		TIME_STAMP_INIT
	#endif
		//xFequalizeHist< XF_8UC1, HEIGHT, WIDTH, NPIX >(imgInput,imgInput1,imgOutput);

		equalizeHist_accel(imgInput,imgInput1,imgOutput);
	#if __SDSCC__
		TIME_STAMP
	#endif
		out_img.data = imgOutput.copyFrom();


	imwrite("out_hls.jpg", out_img);

	//////////////////  Compute Absolute Difference ////////////////////
	absdiff(ocv_ref, out_img, diff); // Compute absolute difference image

	imwrite("out_error.jpg", diff);

	// Find minimum and maximum differences.
	double minval=256,maxval=0;
	int cnt = 0;
	for (int i=0; i<in_img.rows; i++)
	{
		for(int j=0; j<in_img.cols; j++)
		{
			uchar v = diff.at<uchar>(i,j);
			if (v>1)
				cnt++;
			if (minval > v )
				minval = v;
			if (maxval < v)
				maxval = v;
		}
	}
	float err_per = 100.0*(float)cnt/(in_img.rows*in_img.cols);
	fprintf(stderr,"Minimum error in intensity = %f\n Maximum error in intensity = %f\n Percentage of pixels above error threshold = %f\n",minval,maxval,err_per);


	if(err_per > 0.0f){
		printf("\nTest Failed\n");
		return -1;
	}
	return 0;
}

