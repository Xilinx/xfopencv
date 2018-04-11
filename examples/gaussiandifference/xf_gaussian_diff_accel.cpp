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

#include "xf_gaussian_diff_config.h"


void gaussian_diff_accel(xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPC1> &imgInput,
		xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPC1> &imgin1,
		xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPC1> &imgin2,
		xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPC1> &imgin3,
		xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPC1> &imgin4,
		xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPC1> &imgin5,
		xf::Mat<XF_8UC1,HEIGHT,WIDTH,NPC1>&imgOutput,float sigma)
{

	xf::GaussianBlur<FILTER_WIDTH, XF_BORDER_CONSTANT, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgInput, imgin1, sigma);
	xf::Duplicatemats<XF_8UC1, HEIGHT, WIDTH, NPC1>(imgin1,imgin2,imgin3);
	xf::delayMat<MAXDELAY, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgin3,imgin5);
	xf::GaussianBlur<FILTER_WIDTH, XF_BORDER_CONSTANT, XF_8UC1, HEIGHT, WIDTH, NPC1>(imgin2, imgin4, sigma);
	xf::subtract<XF_CONVERT_POLICY_SATURATE, XF_8UC1, HEIGHT, WIDTH, NPC1 >(imgin5,imgin4,imgOutput);

}
