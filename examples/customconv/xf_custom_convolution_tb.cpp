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
#include "xf_custom_convolution_config.h"

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, in_gray, in_conv_img, out_img, ocv_ref, diff, filter;

	in_img = cv::imread(argv[1],1); // reading in the color image

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}

	/*  convert to gray  */
	cvtColor(in_img,in_gray,CV_BGR2GRAY);   // converting input image to gray

	/*  convert to specific types  */
	in_gray.convertTo(in_conv_img,CV_8U);			//Size conversion


	unsigned short height  = in_gray.rows;
	unsigned short width  = in_gray.cols;
	unsigned char shift = SHIFT;

	//////////////////  Creating the kernel ////////////////
	filter.create(FILTER_HEIGHT,FILTER_WIDTH,CV_32F);

	/////////	Filling the Filter coefficients  /////////
	for(int i = 0; i < FILTER_HEIGHT; i++)
	{
		for(int j = 0; j < FILTER_WIDTH; j++)
		{
			filter.at<float>(i,j) = (float)0.00006103515625;
			//filter_ptr[i*FILTER_WIDTH+j] = 1;
		}
	}

		/////////////////    OpenCV reference   /////////////////
#if   OUT_8U
	out_img.create(in_conv_img.rows,in_conv_img.cols,CV_8U); // create memory for output image
	diff.create(in_conv_img.rows,in_conv_img.cols,CV_8U);    // create memory for difference image
#elif OUT_16S
	out_img.create(in_conv_img.rows,in_conv_img.cols,CV_16S); // create memory for output image
	diff.create(in_conv_img.rows,in_conv_img.cols,CV_16S);	  // create memory for difference image
#endif

	cv::Point anchor = cv::Point( -1, -1 );

#if OUT_8U
	cv::filter2D(in_conv_img, ocv_ref, CV_8U , filter, anchor, 0, cv::BORDER_CONSTANT);
#elif OUT_16S
	cv::filter2D(in_conv_img, ocv_ref, CV_16S ,filter, anchor, 0, cv::BORDER_CONSTANT );
#endif

#if __SDSCC__
	short int *filter_ptr=(short int*)sds_alloc_non_cacheable(FILTER_WIDTH*FILTER_HEIGHT*sizeof(short int));
#else
	short int *filter_ptr=(short int*)malloc(FILTER_WIDTH*FILTER_HEIGHT*sizeof(short int));
#endif
	for(int i = 0; i < FILTER_HEIGHT; i++)
	{
		for(int j = 0; j < FILTER_WIDTH; j++)
		{
			filter_ptr[i*FILTER_WIDTH+j] = 2;
		}
	}




	xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput(in_gray.rows,in_gray.cols);
	xF::Mat<OUTTYPE,HEIGHT,WIDTH,NPC1> imgOutput(in_gray.rows,in_gray.cols);

	imgInput.copyTo(in_gray.data);

	#if __SDSCC__
	TIME_STAMP_INIT
	#endif

	//xFfilter2D<XF_BORDER_CONSTANT,FILTER_WIDTH,FILTER_HEIGHT,XF_8UC1,OUTTYPE,HEIGHT, WIDTH,NPC1>(imgInput,imgOutput,filter_ptr,shift);
	Filter2d_accel(imgInput,imgOutput,filter_ptr,shift);

	#if __SDSCC__
	TIME_STAMP
	#endif
	out_img.data = (unsigned char *)imgOutput.copyFrom();




	imwrite("out_img.jpg", out_img);

	imwrite("ref_img.jpg", ocv_ref);  // reference image
	absdiff(ocv_ref,out_img,diff);    // Compute absolute difference image
	imwrite("diff_img.jpg",diff);     // Save the difference image for debugging purpose

	
	double minval=256,maxval=0;
	int cnt = 0;
	for (int i=0;i<in_img.rows;i++)
	{
		for(int j=0;j<in_img.cols;j++)
		{
#if OUT_8U
			unsigned char v1 = ocv_ref.at<unsigned char>(i,j);
			unsigned char v2 = out_img.at<unsigned char>(i,j);
			unsigned char v = diff.at<unsigned char>(i,j);
#elif OUT_16S
			short int v1 = ocv_ref.at<short int>(i,j);
			short int v2 = out_img.at<short int>(i,j);
			short int v = diff.at<short int>(i,j);
#endif
			if (v>2)
				cnt++;
			if (minval > v )
				minval = v;
			if (maxval < v)
				maxval = v;
		}
	}
	float err_per = 100.0*(float)cnt/(in_img.rows*in_img.cols);
	fprintf(stderr,"Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error threshold = %f\n",minval,maxval,err_per);

	in_gray.~Mat();
	in_conv_img.~Mat();

	ocv_ref.~Mat();
	diff.~Mat();
	int ret  = 0;
	if(err_per > 0.0f)
		return 1;
	in_img.~Mat();
	out_img.~Mat();
	return 0;
}

