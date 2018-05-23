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

#ifndef _XF_GAUSSIAN_FILTER_CONFIG_H_
//{
    #define _XF_GAUSSIAN_FILTER_CONFIG_H_

    #include "hls_stream.h"
    #include "common/xf_common.h"
    #include "common/xf_utility.h"
    #include "imgproc/xf_gaussian_filter.hpp"
    #include "xf_config_params.h"

    typedef unsigned short int  uint16_t;

    #define SCALE    ( 0.5f )

    #define ROWS_INP ( 1080 )
    #define COLS_INP ( 1920 )

    #define ROWS_OUT ( ROWS_INP / 2 )
    #define COLS_OUT ( COLS_INP / 2 )

    //----------------- Filters parameters -----------------//

    #define XF_RESIZE_INTERPOLATION XF_INTERPOLATION_NN          // Interpolation type for xf::resize() inside kernel
    #define CV_RESIZE_INTERPOLATION cv::INTER_NEAREST            // Interpolation type for cv::resize() called from testbench
                                                                    
    #define XF_GAUSSIAN_BORDER  XF_BORDER_CONSTANT               // Border type of xfopencv Gaussian filter inside kernel
    #define CV_GAUSSIAN_BORDER  cv::BORDER_CONSTANT              // Border type of   opencv Gaussian filter called from testbench

    #if FILTER_SIZE_3                                            // Set Gaussian filter parameters depending on constant defined in xf_config_params.h
    //{
        #define FILTER_WIDTH (  3  )
        #define SIGMA        ( 0.5f)
    //}
    #elif FILTER_SIZE_5
    //{
        #define FILTER_WIDTH (    5    )
        #define SIGMA        ( 0.8333f )
    //}
    #elif FILTER_SIZE_7
    //{
        #define FILTER_WIDTH (     7    )
        #define SIGMA        ( 1.16666f )
    //}
    #endif

    #define NPC1 XF_NPPC1

    void gaussian_filter_accel(xf::Mat<XF_8UC1, ROWS_INP, COLS_INP, NPC1> &img_inp, xf::Mat<XF_8UC1, ROWS_OUT, COLS_OUT, NPC1> &img_out, float sigma);
//}
#endif //_XF_GAUSSIAN_FILTER_CONFIG_H_
