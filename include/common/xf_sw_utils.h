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
#ifndef _XF_SW_UTILS_H_
#define _XF_SW_UTILS_H_

#include "xf_common.h"
#include <iostream>

#if __SDSCC__
#include "sds_lib.h"


class perf_counter
{
public:
     uint64_t tot, cnt, calls;
     perf_counter() : tot(0), cnt(0), calls(0) {};
     inline void reset() { tot = cnt = calls = 0; }
     inline void start() { cnt = sds_clock_counter(); calls++; };
     inline void stop() { tot += (sds_clock_counter() - cnt); };
     inline uint64_t avg_cpu_cycles() { std::cout << "elapsed time "<< ((tot+(calls>>1)) / calls) << std::endl; return ((tot+(calls>>1)) / calls); };
};
#endif

namespace xf{

	template <int _PTYPE, int _ROWS, int _COLS, int _NPC>
	xf::Mat<_PTYPE, _ROWS, _COLS, _NPC> imread(char *img,  int type){

		cv::Mat img_load = cv::imread(img,type);

		xf::Mat<_PTYPE, _ROWS, _COLS, _NPC> input(img_load.rows, img_load.cols);

		if(img_load.data == NULL){
			std::cout << "\nError : Couldn't open the image at " << img << "\n" << std::endl;
			exit(-1);
		}

		input.copyTo(img_load.data);

		return input;
	}

	template <int _PTYPE, int _ROWS, int _COLS, int _NPC>
	void imwrite(const char *str, xf::Mat<_PTYPE, _ROWS, _COLS, _NPC> &output){

		int list_ptype[] = {CV_8UC1, CV_16UC1, CV_16SC1, CV_32SC1, CV_32FC1,  CV_32SC1, CV_16UC1, CV_32SC1, CV_8UC1, CV_8UC3, CV_16UC3, CV_16SC3};
		int _PTYPE_CV = list_ptype[_PTYPE];

			cv::Mat input(output.rows, output.cols, _PTYPE_CV);
			input.data = output.copyFrom();
			cv::imwrite(str,input);
	}

	template <int _PTYPE, int _ROWS, int _COLS, int _NPC>
	void absDiff(cv::Mat &cv_img, xf::Mat<_PTYPE, _ROWS, _COLS, _NPC>& xf_img, cv::Mat &diff_img ){

		assert((cv_img.rows == xf_img.rows) && (cv_img.cols == xf_img.cols) && "Sizes of cv and xf images should be same");
		assert((xf_img.rows == diff_img.rows) && (xf_img.cols == diff_img.cols) && "Sizes of xf and diff images should be same");
		assert(((_NPC == XF_NPPC8) || (_NPC == XF_NPPC4) || (_NPC == XF_NPPC1)) && "Only XF_NPPC1, XF_NPPC4, XF_NPPC8 are supported");
		//assert(((_PTYPE == XF_8UC3) ) && (_NPC == XF_NPPC8) && "Multi-pixel parallelism not supported for multi-channel images");
		assert((cv_img.channels() == XF_CHANNELS(_PTYPE, _NPC)) && "Number of channels of cv and xf images does not match");

		int STEP = XF_PIXELDEPTH(XF_DEPTH(_PTYPE, _NPC));

		for(int i=0; i<cv_img.rows;++i){
			XF_TNAME(_PTYPE,_NPC) diff_pix = 0;
			XF_TNAME(_PTYPE,_NPC) xf_pix = 0;
			for(int j=0,l=0,byteindex1=0,byteindex2=0,shift=0; j<cv_img.cols;++j){

				if(_NPC == XF_NPPC8 ){	//If 8PPC
					if( j%8==0){		//then read only once for 8 pixels
						xf_pix = xf_img.data[(i*(xf_img.cols>>(XF_BITSHIFT(_NPC))))+l]; //reading 8 pixels at a time
						l++;			//j loops till 1920, hence using variable l as we need  only 240 iterations in 8PPC mode
						shift=0;
					}
					int cv_pix = cv_img.at< ap_uint<XF_PIXELDEPTH(XF_DEPTH(_PTYPE, _NPC))> >(i,j);
					diff_img.at< ap_uint<XF_PIXELDEPTH(XF_DEPTH(_PTYPE, _NPC))> >(i,j) = std::abs(cv_pix - xf_pix.range(shift+STEP-1, shift));
					shift = shift+STEP;
				}

				else if(_NPC == XF_NPPC4 ){	//If 4PPC
					if( j%4==0){		//then read only once for 4 pixels
						xf_pix = xf_img.data[(i*(xf_img.cols>>(XF_BITSHIFT(_NPC))))+l]; //reading 4 pixels at a time
						l++;			//j loops till 1920, hence using variable l as we need  only 480 iterations in 4PPC mode
						shift=0;
					}
					ap_uint<XF_PIXELDEPTH(XF_DEPTH(_PTYPE, _NPC))> cv_pix = cv_img.at< ap_uint<XF_PIXELDEPTH(XF_DEPTH(_PTYPE, _NPC))> >(i,j);
					diff_img.at< ap_uint<XF_PIXELDEPTH(XF_DEPTH(_PTYPE, _NPC))> >(i,j) = std::abs(cv_pix - xf_pix.range(shift+STEP-1, shift));
					shift = shift+STEP;
				}

				else if(_NPC == XF_NPPC1){
									xf_pix = xf_img.data[i*xf_img.cols+j];
									XF_TNAME(_PTYPE,_NPC) cv_pix = 0;
									int mul_val=0, nbytes =0;

									if(cv_img.depth()==CV_8U){
										mul_val = 8;
										nbytes = 1;
									}
									else if(cv_img.depth()==CV_16S || cv_img.depth()==CV_16U){
										mul_val = 16;
										nbytes = 2;
									}
									else if(cv_img.depth()==CV_32S || cv_img.depth()==CV_32F){
										mul_val = 32;
										nbytes = 4;
									}
									else{
										printf("OpenCV image's depth not supported");
										return;
									}

									for(int k=0; k< XF_CHANNELS(_PTYPE, _NPC)*mul_val; ++byteindex1,k+=8){

										unsigned char pix_temp = cv_img.data[i*cv_img.cols*XF_CHANNELS(_PTYPE, _NPC)*nbytes+byteindex1];
										 cv_pix.range(k+7,k) = pix_temp;
										 pix_temp = 0;
									}

									diff_pix = std::abs(cv_pix - xf_pix);

									for(int k=0; k< XF_CHANNELS(_PTYPE, _NPC)*mul_val; ++byteindex2,k+=8){
										diff_img.data[i*cv_img.cols*XF_CHANNELS(_PTYPE, _NPC)*nbytes+byteindex2] = diff_pix.range(k+7,k);
									}

								}

			}
		}
	}


}

#endif
