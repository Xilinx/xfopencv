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

#ifndef _XF_MAGNITUDE_HPP_
#define _XF_MAGNITUDE_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

typedef unsigned short  uint16_t;

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "core/xf_math.h"

/**	
 *  xFMagnitudeKernel : The Gradient Magnitude Computation Kernel.
 *  This kernel takes two gradients in AU_16SP format and computes the
 *  XF_16SP normalized magnitude.
 *  The Input arguments are src1, src2 and Norm.
 *  src1 --> Gradient X image from the output of sobel of depth XF_16SP.
 *  src2 --> Gradient Y image from the output of sobel of depth XF_16SP.
 *  _norm_type  --> Either XF_L1Norm or XF_L2Norm which are o and 1 respectively.
 *  _dst --> Magnitude computed image of depth AU_16SP.
 *  Depending on NPC, 16 or 8 pixels are read and gradient values are
 *  calculated.
 */
template <int ROWS, int COLS, int DEPTH_SRC,int DEPTH_DST, int NPC,
int WORDWIDTH_SRC, int WORDWIDTH_DST, int COLS_TRIP>
void xFMagnitudeKernel(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _src1,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _src2,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& _dst,
		int _norm_type,uint16_t &imgheight,uint16_t &imgwidth)
{

	XF_SNAME(WORDWIDTH_SRC) val_src1, val_src2;
	XF_SNAME(WORDWIDTH_DST) val_dst;

	int tempgx, tempgy, result_temp = 0;
	int16_t p, q;
	int16_t result;

	rowLoop:
	for(int i = 0; i < (imgheight); i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for(int j = 0; j < (imgwidth); j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline

			val_src1 = (XF_SNAME(WORDWIDTH_SRC)) (_src1.read());
			val_src2 = (XF_SNAME(WORDWIDTH_SRC)) (_src2.read());

			int proc_loop = XF_WORDDEPTH(WORDWIDTH_DST),
					step  = XF_PIXELDEPTH(DEPTH_DST);

			procLoop:
			for(int k = 0; k < proc_loop; k += step)
			{
#pragma HLS unroll

				p = val_src1.range(k + (step - 1), k);		// Get bits from certain range of positions.
				q = val_src2.range(k + (step - 1), k);		// Get bits from certain range of positions.
				p = __ABS(p);		q= __ABS(q);

				if(_norm_type == XF_L1NORM)
				{
					int16_t tmp = p + q;
					result = tmp;
				}
				else if (_norm_type == XF_L2NORM)
				{
					tempgx = p * p;
					tempgy = q * q;
					result_temp = tempgx + tempgy;
					int tmp1 = xf::Sqrt(result_temp);		// Square root of the gradient images
					result = (int16_t)tmp1;
				}
				val_dst.range(k + (step - 1), k) = result;
			}
			_dst.write(val_dst);		// writing into the output stream
		}
	}
}



/**
 * xFMagnitude: This function acts as a wrapper and calls the
 * Kernel function xFMagnitudeKernel.
 */
template <int ROWS, int COLS, int DEPTH_SRC,int DEPTH_DST, int NPC,
int WORDWIDTH_SRC, int WORDWIDTH_DST>
void xFMagnitude(
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& src1,
		hls::stream<XF_SNAME(WORDWIDTH_SRC)>& src2,
		hls::stream<XF_SNAME(WORDWIDTH_DST)>& _dst,
		int _norm_type,uint16_t imgheight,uint16_t imgwidth)
{

#pragma HLS inline

	imgwidth=imgwidth>>XF_BITSHIFT(NPC);

	xFMagnitudeKernel<ROWS,COLS,DEPTH_SRC,DEPTH_DST,NPC, WORDWIDTH_SRC,
	WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(src1,src2,_dst,_norm_type,imgheight,imgwidth);

}



#endif
