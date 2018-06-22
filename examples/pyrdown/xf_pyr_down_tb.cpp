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
 HOWEVER CXFSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/
#include "xf_headers.h"
#include "xf_pyr_down_config.h"
int main(int argc, char *argv[]){

	cv::Mat input_image, output_image, output_xf, output_diff_xf_cv;


	input_image = cv::imread(argv[1],0);

	unsigned short input_height = input_image.rows;
	unsigned short input_width = input_image.cols;

	unsigned short output_height = (input_image.rows + 1) >> 1;
	unsigned short output_width = (input_image.cols + 1) >> 1;

	output_xf.create(output_height,output_width,CV_8UC1);
	output_diff_xf_cv.create(output_height,output_width,CV_8UC1);


	std::cout << "Input Height " << input_height << " Input_Width " << input_width << std::endl;
	std::cout << "Output Height " << output_height << " Output_Width " << output_width << std::endl;

	cv::pyrDown(input_image, output_image, cv::Size(output_width,output_height),cv::BORDER_REPLICATE);

	cv::imwrite("opencv_image.png",output_image);

	
	static xf::Mat<TYPE, HEIGHT, WIDTH, XF_NPPC1> imgInput(input_image.rows,input_image.cols);
	static xf::Mat<TYPE, HEIGHT, WIDTH, XF_NPPC1> imgOutput(output_height,output_width);

	imgInput = xf::imread<TYPE, HEIGHT, WIDTH, XF_NPPC1>(argv[1], 0);
	
	
	#if __SDSCC__
	perf_counter hw_ctr;		
	hw_ctr.start();
	#endif
	
	pyr_down_accel(imgInput, imgOutput);
	
	#if __SDSCC__
		hw_ctr.stop();
		uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif
	
	output_xf.data = imgOutput.copyFrom();

	//xf::imwrite("hls_out.jpg",imgOutput);

	int num_errors_xf = 0;
	unsigned char max_error = 0;
	unsigned char min_error = 255;
	
	for(int i=0;i<output_height;i++)
	{
		for(int j=0;j<output_width;j++)
		{
			if(output_xf.at<unsigned char>(i,j) == output_image.at<unsigned char>(i,j))
			{
				output_diff_xf_cv.at<unsigned char>(i,j) = 0;
			}
			else
			{
				output_diff_xf_cv.at<unsigned char>(i,j) = 255;
				
				unsigned char temp1 = output_xf.at<unsigned char>(i,j);
				unsigned char temp2 = output_image.at<unsigned char>(i,j);
				unsigned char temp;
				temp = std::abs(temp1-temp2);
				if(temp>max_error)
				{
					max_error = temp;
				}
				else if(temp < min_error)
				{
					min_error = temp;
				}
				if(temp>0)
				{
					num_errors_xf++;
				}
			}
		}
	}
	std::cout << "number of differences between opencv and xf: " << num_errors_xf << std::endl;
	std::cout << "Max Error between opencv and xf: " << (unsigned int)max_error << std::endl;
	std::cout << "Min Error between opencv and xf: " << (unsigned int)min_error << std::endl;
	cv::imwrite("xf_cv_diff_image.png", output_diff_xf_cv);
	cv::imwrite("xf_image.png", output_xf);
	if(max_error > 0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
