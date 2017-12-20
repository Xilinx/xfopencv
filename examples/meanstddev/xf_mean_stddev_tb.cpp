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
#include "xf_mean_stddev_config.h"



float xmean(cv::Mat& img)
{
	unsigned long Sum = 0;
	int i, j;
	/* Sum of All Pixels */
	for(i = 0; i < img.rows; ++i)
	{
		for(j = 0; j < img.cols; ++j)
		{
			Sum += img.at<uchar>(i,j); //imag.data[i]
		}
	}
	return ((float)Sum /(float)(img.rows * img.cols));
}

void variance(cv::Mat& Img, float& mean, double &var)
{
	double sum = 0.0;
	for(int i = 0; i < Img.rows; i++)
	{
		for(int j = 0; j < Img.cols; j++)
		{
			double x = (double)mean - ((double)Img.at<uint8_t>(i,j)) ;
			sum = sum + pow(x, (double)2.0);
		}
	}
	var = (sum / (double)(Img.rows * Img.cols));
}


int main(int argc, char** argv)
{

	if(argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, in_gray;

	// reading in the color image
	in_img = cv::imread(argv[1], 0);
	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image\n");
		return 0;
	}
//	cvtColor(in_img, in_gray, CV_BGR2GRAY);


	unsigned short mean;
	unsigned short stddev;

	xf::Mat<XF_8UC1, HEIGHT, WIDTH, _NPPC> imgInput(in_img.rows,in_img.cols);
	//imgInput.copyTo(in_img.data);
	imgInput = xf::imread<XF_8UC1, HEIGHT, WIDTH, _NPPC>(argv[1], 0);
	
#if __SDSCC__
perf_counter hw_ctr;
hw_ctr.start();
#endif
	mean_stddev_accel(imgInput,mean, stddev);
#if __SDSCC__
hw_ctr.stop();uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif
	

	//////////////// 	Opencv  Reference  ////////////////////////
	float mean_c, stddev_c;
	double var_c;

	/* Two Pass Mean and Variance */
	mean_c = xmean(in_img);
	variance(in_img, mean_c, var_c);
	stddev_c = sqrt(var_c);

	float mean_hls =(float)mean/256;
	float stddev_hls = (float)stddev/256;

	float diff_mean,diff_stddev;

	diff_mean = mean_c - mean_hls ;
	diff_stddev = stddev_c - stddev_hls;

	fprintf(stderr,"Ref. Mean     = %f\t Result = %f\tERROR = %f \n",mean_c, mean_hls, diff_mean);
	fprintf(stderr,"Ref. Std.Dev. = %f\t Result = %f\tERROR = %f \n",stddev_c, stddev_hls, diff_stddev);

	if(diff_mean > 0.1f | diff_stddev > 0.1f){
		printf("\nTest Failed\n");
		return -1;
	}
	return 0;
}
