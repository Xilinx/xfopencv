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
#include "xf_median_blur_config.h"

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_gray, out_img, ocv_ref, diff;

	unsigned short in_width,in_height;

	/*  reading in the color image  */
	in_gray = cv::imread(argv[1],0);

	if (in_gray.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}

	ocv_ref.create(in_gray.rows,in_gray.cols,in_gray.depth());
	out_img.create(in_gray.rows,in_gray.cols,in_gray.depth());
	diff.create(in_gray.rows,in_gray.cols,in_gray.depth());
	/////////////////    OpenCV reference  /////////////////
	cv::medianBlur(in_gray,ocv_ref,WINDOW_SIZE);

	in_width = in_gray.cols;
	in_height = in_gray.rows;

	static xf::Mat<TYPE, HEIGHT, WIDTH, NPxPC> imgInput(in_height,in_width);
	static xf::Mat<TYPE, HEIGHT, WIDTH, NPxPC> imgOutput(in_height,in_width);

	
	//imgInput = xf::imread<TYPE, HEIGHT, WIDTH, NPxPC>(argv[1], 0);
	imgInput.copyTo(in_gray.data);

	#if __SDSCC__
	perf_counter hw_ctr;		
	hw_ctr.start();
	#endif
	
	median_blur_accel(imgInput, imgOutput);
	
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

		// Write output image
		xf::imwrite("hls_out.jpg",imgOutput);

		cv::imwrite("ref_img.jpg", ocv_ref);  // reference image

		xf::absDiff(ocv_ref, imgOutput, diff);

	// Find minimum and maximum differences.
	double minval = 255, maxval = 0;
	int cnt = 0;
	for (int i = 0; i < in_gray.rows; i++)
	{
		for(int j = 0; j < in_gray.cols; j++)
		{
			uchar v = diff.at<uchar>(i,j);
			if(diff.at<uchar>(i,j) <= 0)
				diff.at<uchar>(i,j) = 0;
			else
				diff.at<uchar>(i,j) = 255;
			if (v>0)
				cnt++;
			if (minval > v )
				minval = v;
			if (maxval < v)
				maxval = v;
		}
	}
	cv::imwrite("diff_img.jpg",diff); // Save the difference image for debugging purpose
	float err_per = 100.0*(float)cnt/(in_gray.rows*in_gray.cols);
	fprintf(stderr,"Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error threshold = %f\n",minval,maxval,err_per);

	in_gray.~Mat();
	out_img.~Mat();
	ocv_ref.~Mat();
	in_gray.~Mat();
	diff.~Mat();

	if(err_per > 0.0f)
	{
		return 1;
	}

	return 0;
}
