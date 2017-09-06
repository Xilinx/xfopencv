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

#ifndef _XF_SVM_H_
#define _XF_SVM_H_

#include "common/xf_common.h"

/****************************************************************
 *  xFSVM: SVM core computation (dot product function)
 *  ------
 *
 *  Inputs:
 *  -------
 *  in_1: Input array 1
 *  in_2: Input array 2
 *  idx1: Starting index of Input array 1
 *  idx2: Starting index of Input array 2
 *  frac1: Fractional bits in the Input array 1
 *  frac2: Fractional bits in the Input array 2
 *  n: number of kernel operations
 *
 *  Output:
 *  -------
 *  out_frac: Fractional bits in the output result
 *
 ***************************************************************/
namespace xf {

template<int SRC1_T, int SRC2_T, int DST_T, int NPC, int N>
ap_int<XF_PIXELDEPTH(DST_T)> xfSVM(ap_int<XF_DTPIXELDEPTH(SRC1_T, NPC)>* in_1, ap_int<XF_PIXELDEPTH(SRC2_T)>* in_2, uint16_t idx1,
		uint16_t idx2, uchar_t frac1, uchar_t frac2, uint16_t n,
		uchar_t *out_frac)
{


#pragma HLS INLINE OFF

	// temporary result
	ap_int<XF_PIXELDEPTH(DST_T)> result = 0;

	svmCoreLoop:
	for(int i = 0; i < n; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=N max=N avg=N
#pragma HLS PIPELINE

		// Dot product operation
		ap_int<XF_PIXELDEPTH(DST_T)> tmp_svm = (ap_int<XF_PIXELDEPTH(DST_T)>)(in_1[idx1+i] * in_2[idx2+i]);
		result += tmp_svm;
	}

	*out_frac = frac1 + frac2;
	return result;
}
/*
#pragma SDS data zero_copy(in_1[0:IN_ARRAY_SIZE_1])
#pragma SDS data zero_copy(in_2[0:IN_ARRAY_SIZE_2])
#pragma SDS data mem_attribute (in_1:NON_CACHEABLE|PHYSICAL_CONTIGUOUS, in_2:NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
template<int I1, int I2, int O, int N>
void xFSVM(ap_int<I1>* in_1, ap_int<I2>* in_2, uint16_t idx1,
		uint16_t idx2, uchar_t frac1, uchar_t frac2, uint16_t n,
		uchar_t out_frac, ap_int<O> *result){

	ap_int<O> svm_res = xfSVM<I1,I2,O,N>(in_1, in_2, idx1, idx2, frac1, frac2, n, &out_frac);
	
	*result = svm_res;

}
*/
#pragma SDS data copy("in_1.data"[0:"in_1.size"])
#pragma SDS data copy("in_2.data"[0:"in_2.size"])
//#pragma SDS data data_mover("in_1.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("in_2.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("in_1.data":SEQUENTIAL)
#pragma SDS data access_pattern("in_2.data":SEQUENTIAL)
#pragma SDS data mem_attribute ("in_1.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS, "in_2.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
template<int SRC1_T, int SRC2_T, int DST_T, int ROWS1, int COLS1, int ROWS2, int COLS2, int NPC=1, int N>
void SVM(xf::Mat<SRC1_T, ROWS1, COLS1, NPC> &in_1, xf::Mat<SRC2_T, ROWS2, COLS2, NPC> &in_2, uint16_t idx1,
		uint16_t idx2, uchar_t frac1, uchar_t frac2, uint16_t n,
		uchar_t *out_frac, ap_int<XF_PIXELDEPTH(DST_T)> *result){

	ap_int<XF_PIXELDEPTH(DST_T)> svm_res = xfSVM<SRC1_T, SRC2_T, DST_T, NPC, N>((ap_int<XF_DTPIXELDEPTH(SRC1_T, NPC)>*)in_1.data, (ap_int<XF_DTPIXELDEPTH(SRC2_T, NPC)>*)in_2.data, idx1, idx2, frac1, frac2, n, out_frac);
	
	*result = svm_res;

}
}
#endif
