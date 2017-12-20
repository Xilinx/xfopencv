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
#include "xf_channel_combine_config.h"


int main(int argc, char **argv)
{
	if(argc != 5)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image1 path> <input image2 path> <input image3 path> <input image4 path>\n");
		return -1;
	}

	cv::Mat in_gray1, in_gray2;
	cv::Mat in_gray3, in_gray4;
	cv::Mat out_img, ref;
	cv::Mat diff;

	// Read images
	in_gray1 = cv::imread(argv[1], 0);
	in_gray2 = cv::imread(argv[2], 0);
	in_gray3 = cv::imread(argv[3], 0);
	in_gray4 = cv::imread(argv[4], 0);

	if ((in_gray1.data == NULL) | (in_gray2.data == NULL) | (in_gray3.data == NULL) | (in_gray4.data == NULL))
	{
		fprintf(stderr, "Cannot open image \n");
		return 0;
	}

	//creating memory for diff image
	diff.create(in_gray1.rows, in_gray1.cols, CV_8UC4);


	// image height and width
	uint16_t height = in_gray1.rows;
	uint16_t width = in_gray1.cols;


	xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> imgInput1(in_gray1.rows,in_gray1.cols);
	xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> imgInput2(in_gray2.rows,in_gray2.cols);
	xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> imgInput3(in_gray3.rows,in_gray3.cols);
	xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> imgInput4(in_gray4.rows,in_gray4.cols);
	xf::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> imgOutput(in_gray1.rows,in_gray1.cols);

	imgInput1 = xf::imread<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>(argv[1], 0);
	imgInput2 = xf::imread<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>(argv[2], 0);
	imgInput3 = xf::imread<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>(argv[3], 0);
	imgInput4 = xf::imread<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>(argv[4], 0);

#if __SDSCC__
	perf_counter hw_ctr;


	hw_ctr.start();
#endif
		channel_combine_accel(imgInput1, imgInput2, imgInput3, imgInput4, imgOutput);
#if __SDSCC__
	hw_ctr.stop();

	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	// Write output image
	xf::imwrite("hls_out.jpg",imgOutput);

	// OpenCV reference
	std::vector<cv::Mat> bgr_planes;
	cv::Mat merged;
	bgr_planes.push_back(in_gray3);
	bgr_planes.push_back(in_gray2);
	bgr_planes.push_back(in_gray1);
	bgr_planes.push_back(in_gray4);

	cv::merge(bgr_planes, merged);
	cv::imwrite("out_ocv.jpg", merged);

	// compute the absolute difference
	xf::absDiff(merged, imgOutput, diff);

	// write the difference image
	cv::imwrite("diff.jpg", diff);

	// Find minimum and maximum differences.
	double minval = 256, maxval = 0;
	int cnt = 0;
	for (int i = 0; i < diff.rows; i++)
	{
		for(int j = 0; j < diff.cols; j++)
		{
			int v = diff.at<int>(i,j);
			if (v > 0) cnt++;
			if (minval > v ) minval = v;
			if (maxval < v)  maxval = v;
		}
	}
	float err_per = 100.0*(float)cnt/(in_gray1.rows * in_gray1.cols);

	fprintf(stderr,"Minimum error in intensity = %f\n"
			"Maximum error in intensity = %f\n"
			"Percentage of pixels above error threshold = %f\n",
			minval, maxval, err_per);

	if(err_per > 0.0f)
		return (int)-1;

	return 0;
}
