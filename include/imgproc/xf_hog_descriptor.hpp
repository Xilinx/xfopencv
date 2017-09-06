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

#ifndef _XF_HOG_DESCRIPTOR_WRAPPER_HPP_
#define _XF_HOG_DESCRIPTOR_WRAPPER_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

#ifdef __SDSVHLS_SYNTHESIS__
	#include "xf_hog_descriptor_kernel.hpp"
#endif

namespace xf {
//#pragma SDS data data_mover("_in_mat.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_desc_mat.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_in_mat.data":SEQUENTIAL)
#pragma SDS data access_pattern("_desc_mat.data":SEQUENTIAL)
#pragma SDS data copy("_in_mat.data"[0:"_in_mat.size"])
#pragma SDS data copy("_desc_mat.data"[0:"_desc_mat.size"])
template<int WIN_HEIGHT, int WIN_WIDTH, int WIN_STRIDE, int BLOCK_HEIGHT, int BLOCK_WIDTH, int CELL_HEIGHT, int CELL_WIDTH, int NOB, int ROWS, int COLS, int SRC_T, int DST_T, int DESC_SIZE, int NPC = XF_NPPC1, int IMG_COLOR, int OUTPUT_VARIANT>
void HOGDescriptor(xf::Mat<SRC_T, ROWS, COLS, NPC> &_in_mat, xf::Mat<DST_T, 1, DESC_SIZE, NPC> &_desc_mat)
{

#ifdef __SDSVHLS_SYNTHESIS__
	#include "xf_hog_descriptor_body.inc"
#endif

}
}

#endif   // _XF_HOG_DESCRIPTOR_WRAPPER_HPP_
