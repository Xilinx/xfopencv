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

#include "xf_cvt_color_config.h"

#if RGBA2IYUV
	void cvtcolor_rgba2iyuv(xF::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgInput,xF::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0,xF::Mat<XF_8UC1, HEIGHT/4, WIDTH, XF_NPPC1> &imgOutput1,xF::Mat<XF_8UC1, 			  		HEIGHT/4,WIDTH,XF_NPPC1> &imgOutput2)
	{

		xFrgba2iyuv<XF_8UC4,XF_8UC1, HEIGHT, WIDTH,XF_NPPC1 >(imgInput,imgOutput0,imgOutput1,imgOutput2);
	}
#endif
#if RGBA2NV12
	void cvtcolor_rgba2nv12(xF::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgInput,xF::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0,xF::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, XF_NPPC1> &imgOutput1)
	{
		xFrgba2nv12<XF_8UC4,XF_8UC1,XF_8UC2,HEIGHT,WIDTH,XF_NPPC1>(imgInput,imgOutput0,imgOutput1);
	}
#endif
#if RGBA2NV21
	void cvtcolor_rgba2nv21(xF::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgInput,xF::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0,xF::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, XF_NPPC1> &imgOutput1)
	{
		xFrgba2nv21<XF_8UC4,XF_8UC1,XF_8UC2,HEIGHT,WIDTH,XF_NPPC1>(imgInput,imgOutput0,imgOutput1);
	}
#endif
#if RGBA2YUV4
	void cvtcolor_rgba2yuv4(xF::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgInput, xF::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0,xF::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgOutput1,xF::Mat<XF_8UC1, HEIGHT, WIDTH, 					XF_NPPC1>&imgOutput2)
	{
		xFrgba2yuv4<XF_8UC4,XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>(imgInput,imgOutput0,imgOutput1,imgOutput2);
	}
#endif

#if IYUV2NV12
	void cvtcolor_iyuv2nv12(xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xF::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput1,xF::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput2,xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> 					&imgOutput0,xF::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{	
		xFiyuv2nv12<XF_8UC1, XF_8UC2, HEIGHT, WIDTH, NPC1,NPC2>(imgInput0,imgInput1,imgInput2,imgOutput0,imgOutput1);
	}
#endif
#if IYUV2RGBA
	void cvtcolor_iyuv2rgba(xF::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgInput0,xF::Mat<XF_8UC1, HEIGHT/4, WIDTH, XF_NPPC1> &imgInput1,xF::Mat<XF_8UC1, HEIGHT/4, WIDTH, XF_NPPC1> &imgInput2,xF::Mat<XF_8UC4, HEIGHT, WIDTH, 				XF_NPPC1> &imgOutput0)
	{
		xFiyuv2rgba<XF_8UC1,XF_8UC4,HEIGHT,WIDTH,XF_NPPC1>(imgInput0,imgInput1,imgInput2,imgOutput0);
	}
#endif
#if IYUV2YUV4
	void cvtcolor_iyuv2yuv4(xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xF::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput1,xF::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgInput2,xF::Mat<XF_8UC1, HEIGHT, WIDTH, 					NPC1> &imgOutput0,xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput1,xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput2)
	{
		xFiyuv2yuv4<XF_8UC1,HEIGHT,WIDTH,NPC1>(imgInput0,imgInput1,imgInput2,imgOutput0,imgOutput1,imgOutput2);
	}
#endif

#if NV122IYUV
	void cvtcolor_nv122iyuv(xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xF::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xF::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> 				&imgOutput1,xF::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput2)
	{
		xFnv122iyuv<XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0,imgOutput1,imgOutput2);
	}
#endif

#if NV122RGBA
	void cvtcolor_nv122rgba(xF::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgInput0,xF::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, XF_NPPC1> &imgInput1,xF::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0)
	{
		xFnv122rgba<XF_8UC1,XF_8UC2,XF_8UC4,HEIGHT,WIDTH,XF_NPPC1>(imgInput0,imgInput1,imgOutput0);
	}
#endif
#if NV122YUV4
	void cvtcolor_nv122yuv4(xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xF::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> 					&imgOutput1,xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput2)
	{
		xFnv122yuv4<XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0,imgOutput1,imgOutput2);
	}
#endif

#if NV212IYUV
	void cvtcolor_nv212iyuv(xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xF::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xF::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> 					&imgOutput1,xF::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput2)
	{
		xFnv212iyuv<XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0,imgOutput1,imgOutput2);
	}
#endif
#if NV212RGBA
	void cvtcolor_nv212rgba(xF::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> &imgInput0,xF::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, XF_NPPC1> &imgInput1,xF::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0)
	{
		xFnv212rgba<XF_8UC1,XF_8UC2,XF_8UC4,HEIGHT,WIDTH,XF_NPPC1>(imgInput0,imgInput1,imgOutput0);
	}
#endif
#if NV212YUV4
	void cvtcolor_nv212yuv4(xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgInput0,xF::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgInput1,xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> 					&imgOutput1,xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput2)
	{
		xFnv212yuv4<XF_8UC1,XF_8UC2,HEIGHT,WIDTH,NPC1,NPC2>(imgInput0,imgInput1,imgOutput0,imgOutput1,imgOutput2);
	}
#endif
#if UYVY2IYUV
	void cvtcolor_uyvy2iyuv(xF::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xF::Mat<XF_8UC1, HEIGHT / 4, WIDTH, NPC1> &imgOutput1,xF::Mat<XF_8UC1, HEIGHT / 4, WIDTH, 					NPC1> &imgOutput2)
	{
		xFuyvy2iyuv<XF_16UC1, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0, imgOutput1, imgOutput2);
	}
#endif
#if UYVY2NV12
	void cvtcolor_uyvy2nv12(xF::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xF::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{
		xFuyvy2nv12<XF_16UC1,XF_8UC1,XF_8UC2,HEIGHT, WIDTH, NPC1,NPC2>(imgInput,imgOutput0,imgOutput1);
	}
#endif
#if UYVY2RGBA
	void cvtcolor_uyvy2rgba(xF::Mat<XF_16UC1, HEIGHT, WIDTH, XF_NPPC1> &imgInput,xF::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0)
	{
		xFuyvy2rgba<XF_16UC1,XF_8UC4, HEIGHT, WIDTH, XF_NPPC1>(imgInput,imgOutput0);
	}
#endif
#if YUYV2IYUV
	void cvtcolor_yuyv2iyuv(xF::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xF::Mat<XF_8UC1, HEIGHT/4, WIDTH, NPC1> &imgOutput1,xF::Mat<XF_8UC1, HEIGHT/4, WIDTH, 					NPC1> &imgOutput2)
	{
		xFyuyv2iyuv<XF_16UC1,XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput,imgOutput0,imgOutput1,imgOutput2);
	}
#endif
#if YUYV2NV12
	void cvtcolor_yuyv2nv12(xF::Mat<XF_16UC1, HEIGHT, WIDTH, NPC1> &imgInput,xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> &imgOutput0,xF::Mat<XF_8UC2, HEIGHT/2, WIDTH/2, NPC2> &imgOutput1)
	{
		xFyuyv2nv12<XF_16UC1,XF_8UC1,XF_8UC2, HEIGHT, WIDTH, NPC1,NPC2>(imgInput,imgOutput0,imgOutput1);
	}
#endif
#if YUYV2RGBA
	void cvtcolor_yuyv2rgba(xF::Mat<XF_16UC1, HEIGHT, WIDTH, XF_NPPC1> &imgInput,xF::Mat<XF_8UC4, HEIGHT, WIDTH, XF_NPPC1> &imgOutput0)
	{
		xFyuyv2rgba<XF_16UC1,XF_8UC4, HEIGHT, WIDTH, XF_NPPC1>(imgInput,imgOutput0);
	}
#endif
