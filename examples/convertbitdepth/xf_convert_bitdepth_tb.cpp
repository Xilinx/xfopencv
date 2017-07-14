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
#include "xf_convert_bitdepth_config.h"

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, diff;
	cv::Mat in_gray, input_img, ocv_ref;

	// reading in the color image
	in_img = cv::imread(argv[1], 1);

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n", argv[1]);
		return 0;
	}

	cvtColor(in_img,in_gray,CV_BGR2GRAY);

#if !(XF_CONVERT8UTO16S || XF_CONVERT8UTO16U || XF_CONVERT8UTO32S)
	in_gray.convertTo(input_img, OCV_INTYPE);
#endif

	// create memory for output image
	cv::Mat out_img(in_gray.rows,in_gray.cols, OCV_OUTTYPE);
	unsigned short int height=in_gray.rows;
	unsigned short int width=in_gray.cols;

	///////////////// 	Opencv  Reference  ////////////////////////
#if !(XF_CONVERT8UTO16S || XF_CONVERT8UTO16U || XF_CONVERT8UTO32S)
	input_img.convertTo(ocv_ref,OCV_OUTTYPE);


#else
	in_gray.convertTo(ocv_ref,OCV_OUTTYPE);
#endif
	cv::imwrite("out_ocv.png", ocv_ref);
	//////////////////////////////////////////////////////////////

	ap_int<4> _convert_type = CONVERT_TYPE;
	int shift = 0;

	xF::Mat<_SRC_T, HEIGHT, WIDTH, _NPC> imgInput(in_gray.rows,in_gray.cols);
	xF::Mat<_DST_T, HEIGHT, WIDTH, _NPC> imgOutput(in_gray.rows,in_gray.cols);


#if !(XF_CONVERT8UTO16S || XF_CONVERT8UTO16U || XF_CONVERT8UTO32S)
	imgInput.copyTo((IN_TYPE *)input_img.data);
#else
	imgInput.copyTo((IN_TYPE *)in_gray.data);
#endif

#if __SDSCC__
TIME_STAMP_INIT
#endif
	convert_bitdepth_accel(imgInput, imgOutput, _convert_type, shift);
#if __SDSCC__
TIME_STAMP
#endif
	out_img.data = (unsigned char *)imgOutput.copyFrom();

	imwrite("out_hls.png", out_img);

	//////////////////  Compute Absolute Difference ////////////////////
	absdiff(ocv_ref,out_img,diff);
	imwrite("out_err.jpg", diff);

	// Find minimum and maximum differences.
	double minval=256,maxval=0;
	int cnt = 0;
	for (int i=0; i<in_img.rows; i++)
	{
		for(int j=0; j<in_img.cols; j++)
		{
			uchar v = diff.at<uchar>(i,j);
			if (v>0)
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
