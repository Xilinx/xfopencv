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
#include "xf_colordetect_config.h"


int main(int argc, char **argv)
{
	cv::Mat in_img, img_rgba,out_img;



	in_img = cv::imread(argv[1],1);
	if(!in_img.data){
		return -1;
	}

	uint16_t height = in_img.rows;
	uint16_t width = in_img.cols;

	out_img.create(height,width,CV_8U);

	//cv::cvtColor(in_img, img_rgba, CV_BGR2RGBA);

#if __SDSCC__
	unsigned char * high_thresh = (unsigned char *)sds_alloc_non_cacheable(9* sizeof(unsigned char));
	unsigned char * low_thresh = (unsigned char *)sds_alloc_non_cacheable(9* sizeof(unsigned char));
#else
	unsigned char * high_thresh = (unsigned char *)malloc(9* sizeof(unsigned char));
	unsigned char * low_thresh = (unsigned char *)malloc(9* sizeof(unsigned char));
#endif
	low_thresh[0] = 22;
	low_thresh[1] = 150;
	low_thresh[2] = 60;

	high_thresh[0] = 38;
	high_thresh[1] = 255;
	high_thresh[2] = 255;

	low_thresh[3] = 38;
	low_thresh[4] = 150;
	low_thresh[5] = 60;

	high_thresh[3] = 75;
	high_thresh[4] = 255;
	high_thresh[5] = 255;

	low_thresh[6] = 160;
	low_thresh[7] = 150;
	low_thresh[8] = 60;

	high_thresh[6] = 179;
	high_thresh[7] = 255;
	high_thresh[8] = 255;

	printf("thresholds loaded");

	xf::Mat<XF_8UC3,HEIGHT,WIDTH,NPIX> imgInput(in_img.rows,in_img.cols);

	xf::Mat<XF_8UC3,HEIGHT,WIDTH,NPIX> hsvimage(in_img.rows,in_img.cols);
	xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPIX> imgrange(in_img.rows,in_img.cols);

	xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPIX> imgerode1(in_img.rows,in_img.cols);
	xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPIX> imgdilate1(in_img.rows,in_img.cols);
	xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPIX> imgdilate2(in_img.rows,in_img.cols);
	xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPIX> imgOutput(in_img.rows,in_img.cols);

	imgInput.copyTo(in_img.data);


	printf("image loaded");

#if __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif

	colordetect_accel(imgInput,hsvimage,imgrange,imgerode1,imgdilate1,imgdilate2,imgOutput, low_thresh, high_thresh);

#if __SDSCC__

	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

	out_img.data = imgOutput.copyFrom();


	imwrite("output.png", out_img); 
	imwrite("input.png", in_img); 

	return 0;
}
