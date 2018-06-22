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
#ifndef _XF_CVT_COLOR_CONFIG_H_
#define _XF_CVT_COLOR_CONFIG_H_

#include"hls_stream.h"
#include "ap_int.h"
#include "xf_config_params.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "imgproc/xf_cvt_color.hpp"
#include "imgproc/xf_cvt_color_1.hpp"

// Has to be set when synthesizing
#define _XF_SYNTHESIS_ 1


// Image Dimensions
#define WIDTH 	1920
#define HEIGHT 	1080

#ifdef IYUV2NV12
#if NO
#define NPC1 XF_NPPC1
#define NPC2 XF_NPPC1
#endif
#if RO
#define NPC1 XF_NPPC8
#define NPC2 XF_NPPC4
#endif
#endif

#ifdef IYUV2YUV4
#if NO
#define NPC1 XF_NPPC1
#endif
#if RO
#define NPC1 XF_NPPC8
#endif
#endif

#ifdef NV122IYUV || NV212IYUV
#if NO
#define NPC1 XF_NPPC1
#define NPC2 XF_NPPC1
#endif
#if RO
#define NPC1 XF_NPPC8
#define NPC2 XF_NPPC4
#endif
#endif
#ifdef NV122YUV4 || NV212YUV4
#if NO
#define NPC1 XF_NPPC1
#define NPC2 XF_NPPC1
#endif
#if RO
#define NPC1 XF_NPPC8
#define NPC2 XF_NPPC4
#endif
#endif

#ifdef UYVY2IYUV || YUYV2IYUV
#if NO
#define NPC1 XF_NPPC1
#endif
#if RO
#define NPC1 XF_NPPC8
#endif
#endif

#ifdef UYVY2NV12 || YUYV2NV12
#if NO
#define NPC1 XF_NPPC1
#define NPC2 XF_NPPC1
#endif
#if RO
#define NPC1 XF_NPPC8
#define NPC2 XF_NPPC4
#endif
#endif

void cvtcolor_rgba2iyuv(xf::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, XF_NPPC1> &imgOutput1,xf::Mat<XF_8UC1,HEIGHT/4,WIDTH,XF_NPPC1> &imgOutput2);
void cvtcolor_rgba2nv12(xf::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, XF_NPPC1> &imgOutput1);
void cvtcolor_rgba2nv21(xf::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, XF_NPPC1> &imgOutput1);
void cvtcolor_rgba2yuv4(xf::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgInput, xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>&imgOutput2);
void cvtcolor_iyuv2nv12(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput2,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> 					&imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);
void cvtcolor_iyuv2rgba(xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgInput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, XF_NPPC1> &imgInput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, XF_NPPC1> &imgInput2,xf::Mat<XF_8UC4, HEIGHT, WIDTH, 				XF_NPPC1> &imgOutput0);
void cvtcolor_iyuv2yuv4(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput2,xf::Mat<XF_8UC1, HEIGHT, WIDTH, 					NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput2);
void cvtcolor_nv122iyuv(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> 				&imgOutput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput2);
void cvtcolor_nv122rgba(xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, XF_NPPC1> &imgInput1,xf::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0);
void cvtcolor_nv122yuv4(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> 					&imgOutput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput2);
void cvtcolor_nv212iyuv(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> 					&imgOutput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput2);
void cvtcolor_nv212rgba(xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, XF_NPPC1> &imgInput1,xf::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0);
void cvtcolor_nv212yuv4(xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> 					&imgOutput1,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput2);
void cvtcolor_uyvy2iyuv(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT / 4, WIDTH, 					NPC1> &imgOutput2);
void cvtcolor_uyvy2nv12(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);
void cvtcolor_uyvy2rgba(xf::Mat<XF_16UC1, HEIGHT, WIDTH, XF_NPPC1> &imgInput,xf::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0);
void cvtcolor_yuyv2iyuv(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput1,xf::Mat<XF_8UC1, HEIGHT/4, WIDTH, 					NPC1> &imgOutput2);
void cvtcolor_yuyv2nv12(xf::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xf::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1);
void cvtcolor_yuyv2rgba(xf::Mat<XF_16UC1, HEIGHT, WIDTH, XF_NPPC1> &imgInput,xf::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0);




#endif //_XF_CVT_COLOR_CONFIG_H_
