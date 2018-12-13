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
#include "xf_resize_config.h"
	
int main(int argc,char **argv){
	cv::Mat img, result_ocv,error;

	if(argc != 2){
		printf("Usage : <executable> <input image> \n");
		return -1;
	}

	img = cv::imread(argv[1],0);

	if(!img.data){
		return -1;
	}

	cv::imwrite("input.png",img);

	unsigned short in_width,in_height;
	unsigned short out_width,out_height;

	in_width = img.cols;
	in_height = img.rows;
	out_height = NEWHEIGHT;
	out_width = NEWWIDTH;

	result_ocv.create(cv::Size(NEWWIDTH, NEWHEIGHT),img.depth());
	error.create(cv::Size(NEWWIDTH, NEWHEIGHT),img.depth());

	static xf::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput(in_height,in_width);
	static xf::Mat<TYPE, NEWHEIGHT, NEWWIDTH, NPC1> imgOutput(out_height,out_width);
	
	imgInput.copyTo(img.data);

	#ifdef __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
	#endif
	resize_accel(imgInput, imgOutput);
		
	#ifdef __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif
		
	/*OpenCV resize function*/
#if INTERPOLATION==0
	cv::resize(img,result_ocv,cv::Size(NEWWIDTH,NEWHEIGHT),0,0,CV_INTER_NN);
#endif
#if INTERPOLATION==1
	cv::resize(img,result_ocv,cv::Size(NEWWIDTH,NEWHEIGHT),0,0,CV_INTER_LINEAR);
#endif
#if INTERPOLATION==2
	cv::resize(img,result_ocv,cv::Size(NEWWIDTH,NEWHEIGHT),0,0,CV_INTER_AREA);
#endif

	xf::absDiff(result_ocv,imgOutput,error);
	int cnt = 0;
	double minval=256,maxval=0;
	for (int i=0;i<error.rows;i++){
		for(int j=0;j<error.cols;j++){
			uchar v = error.at<uchar>(i,j);
			if (v>1)
			{
				cnt++;
				error.at<unsigned char>(i,j) = 255;
			}
			if (minval > v )
				minval = v;
			if (maxval < v)
				maxval = v;
		}
	}
	float err_per = 100.0*(float)cnt/(error.rows*error.cols);
	fprintf(stderr,"Minimum error in intensity = %f\n Maximum error in intensity = %f\n Percentage of pixels above error threshold = %f\n",minval,maxval,err_per);
	xf::imwrite("hls_out.png",imgOutput);
	cv::imwrite("resize_ocv.png", result_ocv);
	cv::imwrite("error.png", error);

	if(maxval>10){
		printf("\nTest Failed\n");
		return -1;
	}

	return 0;
}
