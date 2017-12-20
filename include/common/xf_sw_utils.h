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
 HOWEVER CXFSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ***************************************************************************/
#ifndef _XF_SW_UTILS_H_
#define _XF_SW_UTILS_H_

#include "xf_common.h"

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

		int _PTYPE_CV;
		if(_PTYPE == XF_8UC3)
		{
			_PTYPE_CV = CV_8UC3;
		}
		else if(_PTYPE == XF_8UC1)
		{
			_PTYPE_CV = CV_8UC1;
		}
		else if(_PTYPE == XF_16UC1|| _PTYPE == XF_8UC2)
		{
			_PTYPE_CV = CV_16UC1;
		}
		else if(_PTYPE == XF_16UC3)
		{
			_PTYPE_CV = CV_16UC3;
		}
		else if(_PTYPE == XF_16SC1)
		{
			_PTYPE_CV = CV_16SC1;
		}
		else if(_PTYPE == XF_8UC4 )
		{
			_PTYPE_CV = CV_8UC4;
		}
		else if(_PTYPE == XF_32UC1)
		{
			_PTYPE_CV = CV_32SC1;
		}
		else if(_PTYPE == XF_32SC1)
		{
			_PTYPE_CV = CV_32SC1;
		}
		else if(_PTYPE == XF_32FC1)
		{
			_PTYPE_CV = CV_32FC1;
		}

			cv::Mat input(output.rows, output.cols, _PTYPE_CV);
			input.data = output.copyFrom();
			cv::imwrite(str,input);
	}

	template <int _PTYPE, int _ROWS, int _COLS, int _NPC>
	void absDiff(cv::Mat &cv_img, xf::Mat<_PTYPE, _ROWS, _COLS, _NPC>& xf_img, cv::Mat &diff_img ){

		assert((cv_img.rows == xf_img.rows) && (cv_img.cols == xf_img.cols) && "Sizes of cv and xf images should be same");
		assert((xf_img.rows == diff_img.rows) && (xf_img.cols == diff_img.cols) && "Sizes of xf and diff images should be same");
		assert(((_NPC == XF_NPPC8) || (_NPC == XF_NPPC4) || (_NPC == XF_NPPC1)) && "Only XF_NPPC1, XF_NPPC4, XF_NPPC8 are supported");

		int STEP = XF_PIXELDEPTH(XF_DEPTH(_PTYPE, _NPC));

		for(int i=0; i<cv_img.rows;++i){
			XF_TNAME(_PTYPE,_NPC) diff_pix = 0;
			XF_TNAME(_PTYPE,_NPC) xf_pix = 0;
			for(int j=0,l=0,shift=0; j<cv_img.cols;++j){

				if(_NPC == XF_NPPC8 ){	//If 8PPC
					if( j%8==0){		//then read only once for 8 pixels
						xf_pix = xf_img.data[(i*(xf_img.cols>>(XF_BITSHIFT(_NPC))))+l]; //reading 8 pixels at a time
						l++;			//j loops till 1920, hence using variable l as we need  only 240 iterations in 8PPC mode
						shift=0;
					}
					int cv_pix = cv_img.at<ap_uint<XF_PIXELDEPTH(XF_DEPTH(_PTYPE, _NPC))>>(i,j);//cv_img.data[i*cv_img.cols+j];
					diff_img.at<ap_uint<XF_PIXELDEPTH(XF_DEPTH(_PTYPE, _NPC))>>(i,j) = abs(cv_pix - xf_pix.range(shift+STEP-1, shift));
					//diff_img.data[i*diff_img.cols+j] = abs(cv_pix - xf_pix.range(shift+STEP-1, shift));
					shift = shift+STEP;
				}

				else if(_NPC == XF_NPPC4 ){	//If 4PPC
					if( j%4==0){		//then read only once for 4 pixels
						xf_pix = xf_img.data[(i*(xf_img.cols>>(XF_BITSHIFT(_NPC))))+l]; //reading 4 pixels at a time
						l++;			//j loops till 1920, hence using variable l as we need  only 480 iterations in 4PPC mode
						shift=0;
					}
					ap_uint<XF_PIXELDEPTH(XF_DEPTH(_PTYPE, _NPC))> cv_pix = cv_img.at<ap_uint<XF_PIXELDEPTH(XF_DEPTH(_PTYPE, _NPC))>>(i,j);//cv_img.data[i*cv_img.cols+j];
					//diff_img.data[i*diff_img.cols+j] = abs(cv_pix - xf_pix.range(shift+STEP-1, shift));
					diff_img.at<ap_uint<XF_PIXELDEPTH(XF_DEPTH(_PTYPE, _NPC))>>(i,j) = abs(cv_pix - xf_pix.range(shift+STEP-1, shift));
					shift = shift+STEP;
				}

				else if(_NPC == XF_NPPC1){
					xf_pix = xf_img.data[i*xf_img.cols+j];
					XF_TNAME(_PTYPE,_NPC) cv_pix = cv_img.at<XF_TNAME(_PTYPE,_NPC)>(i,j);//data[i*cv_img.cols+j];

					diff_pix.range(STEP-1,0) = abs(cv_pix.range(STEP-1,0) - xf_pix.range(STEP-1,0));

					//diff_img.data[i*diff_img.cols+j]= diff_pix;
					diff_img.at<XF_TNAME(_PTYPE,_NPC)>(i,j)= diff_pix;
				}

			}
		}
	}


}

#endif
