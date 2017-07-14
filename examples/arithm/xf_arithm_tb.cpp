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
#include "xf_arithm_config.h"


int main(int argc, char** argv)
{
	if (argc != 3)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path1> <input image path2> \n");
		return -1;
	}

	cv::Mat in_img1, in_img2, in_gray1, in_gray2, in_gray1_16, in_gray2_16,out_img, ocv_ref, diff;

	/*  reading in the color image  */
	in_img1 = cv::imread(argv[1],1);
	in_img2 = cv::imread(argv[2],1);

	if (in_img1.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}

	if (in_img2.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[2]);
		return 0;
	}

	/*  convert to gray  */
	cvtColor(in_img1,in_gray1,CV_BGR2GRAY);
	cvtColor(in_img2,in_gray2,CV_BGR2GRAY);

	/*  convert to 16S type  */
	in_gray1.convertTo(in_gray1_16,CV_16SC1);
	in_gray2.convertTo(in_gray2_16,CV_16SC1);


#if T_8U

	out_img.create(in_gray1.rows,in_gray1.cols,in_gray1.depth());
	ocv_ref.create(in_gray2.rows,in_gray1.cols,in_gray1.depth());
	diff.create(in_gray1.rows,in_gray1.cols,in_gray1.depth());

	xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput1(in_gray1.rows,in_gray1.cols);
	xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput2(in_gray1.rows,in_gray1.cols);

	xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgOutput(in_gray1.rows,in_gray1.cols);

	imgInput1.copyTo(in_gray1.data);
	imgInput2.copyTo(in_gray2.data);
	#if __SDSCC__
	TIME_STAMP_INIT
	#endif
	
	arithm_accel(imgInput1,imgInput2,imgOutput);

	#if __SDSCC__
	TIME_STAMP
	#endif
	out_img.data = imgOutput.copyFrom();

	///////////// OpenCV reference  /////////////
    	//cv::absdiff(in_gray1,in_gray2,ocv_ref);
	//cv::add(in_gray1,in_gray2,ocv_ref,cv::noArray(),-1);
	//cv::subtract(in_gray1,in_gray2,ocv_ref,cv::noArray(),-1);
	//cv::bitwise_and(in_gray1,in_gray2,ocv_ref);
	//bitwise_or(in_gray1,in_gray2,ocv_ref);
	//cv::bitwise_not(in_gray1,ocv_ref);
	//cv::bitwise_xor(in_gray1,in_gray2,ocv_ref);
	cv::multiply(in_gray1,in_gray2,ocv_ref,0.05,-1);

#endif

#if T_16S

	out_img.create(in_gray1_16.rows,in_gray1_16.cols,in_gray1_16.depth());
	ocv_ref.create(in_gray1_16.rows,in_gray1_16.cols,in_gray1_16.depth());
	diff.create(in_gray1_16.rows,in_gray1_16.cols,in_gray1_16.depth());

	xF::Mat<XF_16SC1, HEIGHT, WIDTH, NPC1> imgInput1(in_gray1.rows,in_gray1.cols);
	xF::Mat<XF_16SC1, HEIGHT, WIDTH, NPC1> imgInput2(in_gray1.rows,in_gray1.cols);

	xF::Mat<XF_16SC1, HEIGHT, WIDTH, NPC1> imgOutput(in_gray1.rows,in_gray1.cols);

	imgInput1.copyTo((short int*)in_gray1_16.data);
	imgInput2.copyTo((short int*)in_gray2_16.data);

	#if __SDSCC__
	TIME_STAMP_INIT
	#endif

	arithm_accel(imgInput1,imgInput2,imgOutput);

	#if __SDSCC__
	TIME_STAMP
	#endif

	out_img.data = (unsigned char*)imgOutput.copyFrom();

	///////////// OpenCV reference  /////////////
	//cv::add(in_gray1_16,in_gray2_16,ocv_ref,cv::noArray(),-1);
	//cv::subtract(in_gray1_16,in_gray2_16,ocv_ref,cv::noArray(),-1);
	cv::multiply(in_gray1_16,in_gray2_16,ocv_ref,0.05,-1);
#endif



	imwrite("out_img.jpg", out_img);  // save the output image
	imwrite("ref_img.jpg",ocv_ref);   // save the reference image

	absdiff(ocv_ref,out_img,diff);	  // Compute absolute difference image
	imwrite("diff_img.jpg",diff);            // Save the difference image for debugging purpose


	// Find minimum and maximum differences.
	double minval=256,maxval=0;
	int cnt = 0;
	for (int i=0;i<in_img1.rows;i++)
	{
		for(int j=0;j<in_img1.cols;j++)
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
	float err_per = 100.0*(float)cnt/(in_img1.rows*in_img1.cols);
	fprintf(stderr,"Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error threshold = %f\n",minval,maxval,err_per);

	in_img1.~Mat();
	in_img2.~Mat();
	in_gray1.~Mat();
	in_gray2.~Mat();
	in_gray1_16.~Mat();
	in_gray2_16.~Mat();
	out_img.~Mat();
	ocv_ref.~Mat();
	diff.~Mat();

	if(err_per > 0.0f)
	{
		return 1;
	}

	return 0;
}
