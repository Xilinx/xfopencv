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

#ifndef _XF_HISTOGRAM_HPP_
#define _XF_HISTOGRAM_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.h"
template<int SRC_T, int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH, int SRC_TC>
//void xFHistogramKernel(hls::stream< XF_SNAME(WORDWIDTH) >&  _src_mat, uint32_t* _dst, uint16_t &imgheight, uint16_t &imgwidth)
void xFHistogramKernel(xF::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat , uint32_t* _dst, uint16_t &imgheight, uint16_t &imgwidth)
{

	// Temporary array used while computing histogram
	uint32_t tmp_hist[(1<<XF_BITSHIFT(NPC))][256];
	uint32_t tmp_hist1[(1<<XF_BITSHIFT(NPC))][256];
#pragma HLS ARRAY_PARTITION variable=tmp_hist complete dim=1
#pragma HLS ARRAY_PARTITION variable=tmp_hist1 complete dim=1
	XF_SNAME(WORDWIDTH) in_buf,in_buf1, temp_buf;
	bool flag =0;

	HIST_INITIALIZE_LOOP:
	for(ap_uint<10> i=0; i<256; i++)//
	{
#pragma HLS PIPELINE
		for (ap_uint<5> j=0;j<(1<<XF_BITSHIFT(NPC));j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=256 max=256
			tmp_hist[j][i] = 0;
			tmp_hist1[j][i] = 0;
		}
	}

	HISTOGRAM_ROW_LOOP:
	for(ap_uint<13> row=0; row < imgheight; row++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
		HISTOGRAM_COL_LOOP:
		for(ap_uint<13> col=0; col < (imgwidth ) ; col = col+2)
		{
#pragma HLS PIPELINE
#pragma HLS LOOP_FLATTEN OFF
#pragma HLS LOOP_TRIPCOUNT min=SRC_TC max=SRC_TC
			in_buf = _src_mat.data[row*(imgwidth) + col];
			in_buf1 = _src_mat.data[row*(imgwidth) + col+1];

			EXTRACT_UPDATE:
			for(ap_uint<9> i=0,j=0; i < (8 << XF_BITSHIFT(NPC));j++, i+= 8)
			{
#pragma HLS DEPENDENCE variable=tmp_hist array intra false
#pragma HLS DEPENDENCE variable=tmp_hist1 array intra false
#pragma HLS UNROLL
				XF_PTNAME(DEPTH) val,val1;
				val = in_buf.range(i+7, i);
				val1 = in_buf1.range(i+7, i);
				uint32_t tmpval = tmp_hist[j][val];
				uint32_t tmpval1 = tmp_hist1[j][val1];
				tmp_hist[j][val] = tmpval+1;
				tmp_hist1[j][val1] = tmpval1+1;
			}
		}
	}
	uint32_t cnt;

	COPY_LOOP:
	for(ap_uint<10> i=0; i<256; i++)
	{
#pragma HLS pipeline
		cnt = 0;
		for(ap_uint<5> j=0;j<(1<<XF_BITSHIFT(NPC));j++)
		{
#pragma HLS UNROLL

			uint32_t value= tmp_hist[j][i] + tmp_hist1[j][i];
			cnt = cnt + value;
		}
		_dst[i]=cnt;
	}
}
template<int SRC_T, int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH>
void xFHistogram(xF::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat ,uint32_t* _dst, uint16_t imgheight, uint16_t imgwidth)
{



	imgwidth = imgwidth >> (XF_BITSHIFT(NPC));

	assert((DEPTH == XF_8UP) && " Input image must be of type AU_8UP ");

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8) )
			&& "NPC must be XF_NPPC1, XF_NPPC8 ");
	assert(((imgheight <= ROWS ) && (imgwidth <= COLS)) && "ROWS and COLS should be greater than input image");

	xFHistogramKernel<SRC_T, ROWS, COLS, DEPTH, NPC, WORDWIDTH, ((COLS>>(XF_BITSHIFT(NPC)))>>1)>(_src_mat, _dst ,imgheight,imgwidth);
}
#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
#pragma SDS data data_mover(histogram:AXIDMA_SIMPLE)

#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data access_pattern(histogram:SEQUENTIAL)

#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data copy(histogram[0:256])

template<int SRC_T,int ROWS, int COLS,int NPC=1>
void xFcalcHist(xF::Mat<SRC_T, ROWS, COLS, NPC> & _src, uint32_t *histogram)
{
	
	
#pragma HLS INLINE OFF

	xFHistogram<SRC_T, ROWS, COLS, XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC)>(_src, histogram, _src.rows, _src.cols);

}
#endif // _XF_HISTOGRAM_HPP_

