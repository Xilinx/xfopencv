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

#ifndef _XF_MEAN_SHIFT_WRAPPER_HPP_
#define _XF_MEAN_SHIFT_WRAPPER_HPP_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"

#ifdef __SDSVHLS_SYNTHESIS__
	#include "xf_mean_shift_kernel.hpp"
#endif

namespace xf{
#pragma SDS data zero_copy("_in_mat.data"[0:"_in_mat.size"])
#pragma SDS data zero_copy(x1[0:max_obj])
#pragma SDS data zero_copy(y1[0:max_obj])
#pragma SDS data zero_copy(height[0:max_obj])
#pragma SDS data zero_copy(width[0:max_obj])
#pragma SDS data zero_copy(dx[0:max_obj])
#pragma SDS data zero_copy(dy[0:max_obj])
#pragma SDS data zero_copy(status[0:max_obj])
template <int ROWS, int COLS, int OBJ_ROWS, int OBJ_COLS, int MAXOBJ, int MAXITERS, int SRC_T, int NPC>
void MeanShift(xf::Mat<SRC_T, ROWS, COLS, NPC> &_in_mat, uint16_t* x1, uint16_t* y1,
		uint16_t* obj_height, uint16_t* obj_width, uint16_t* dx, uint16_t* dy, uint16_t* status,
		uint8_t frame_status, uint8_t no_objects, uint8_t max_obj, uint8_t no_iters )
{
	
#ifdef __SDSVHLS_SYNTHESIS__
	#include "xf_mean_shift_body.inc"
#endif
}
}

#endif  // _XF_MEAN_SHIFT_WRAPPER_HPP_

