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

#ifndef _XF_STEREO_PIPELINE_CONFIG_H_
#define _XF_STEREO_PIPELINE_CONFIG_H_

#include "hls_stream.h"

#include "common/xf_common.h"
#include "common/xf_utility.h"

#include "xf_config_params.h"


/* config width and height */
#define XF_HEIGHT  720
#define XF_WIDTH   1280

#define XF_CAMERA_MATRIX_SIZE 9
#define XF_DIST_COEFF_SIZE 5


void stereo_pipeline_accel
  (
    //                      Left                              |                       Right
    xf::Mat<XF_8UC1 , XF_HEIGHT, XF_WIDTH, XF_NPPC1> &xf_img_l, xf::Mat<XF_8UC1 , XF_HEIGHT, XF_WIDTH, XF_NPPC1> &xf_img_r, 
                                                              
    xf::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &xf_img_s,

    xf::xFSBMState<SAD_WINDOW_SIZE,NO_OF_DISPARITIES,PARALLEL_UNITS> &bm_state, 
    
    ap_fixed<32,12> *cameraMA_l_fix                           , ap_fixed<32,12> *cameraMA_r_fix, 
    ap_fixed<32,12> *distC_l_fix                              , ap_fixed<32,12> *distC_r_fix   , 
    ap_fixed<32,12> *irA_l_fix                                , ap_fixed<32,12> *irA_r_fix     , 
    
    int cm_size, 
    int dc_size
  );


#endif // _XF_STEREO_PIPELINE_CONFIG_H_

