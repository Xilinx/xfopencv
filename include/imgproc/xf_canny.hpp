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

#ifndef _XF_CANNY_HPP_
#define _XF_CANNY_HPP_

#ifndef __cplusplus
#error C++ is needed to use this file!
#endif


typedef unsigned short  uint16_t;
typedef unsigned char  uchar;

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

#include "core/xf_math.h"
#include "xf_canny_sobel.hpp"
#include "xf_averagegaussianmask.hpp"
#include "xf_magnitude.hpp"
#include "xf_canny_utils.hpp"


namespace xf{
/**
 *  xFDuplicate_rows
 */
template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH, int TC>
void xFDuplicate_rows(hls::stream< XF_SNAME(WORDWIDTH) > &_src_mat,
		hls::stream< XF_SNAME(WORDWIDTH) > &_src_mat1,
		hls::stream< XF_SNAME(WORDWIDTH) > &_dst1_mat,
		hls::stream< XF_SNAME(WORDWIDTH) > &_dst2_mat,
		hls::stream< XF_SNAME(WORDWIDTH) > &_dst1_out_mat,
		hls::stream< XF_SNAME(WORDWIDTH) > &_dst2_out_mat,
		uint16_t img_height, uint16_t img_width)
{
	img_width = img_width >> XF_BITSHIFT(NPC);

	ap_uint<13> row, col;
	Row_Loop:
	for(row = 0 ; row < img_height; row++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		Col_Loop:
		for(col = 0; col < img_width; col++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
			XF_SNAME(WORDWIDTH) tmp_src,tmp_src1;
			tmp_src1 = _src_mat1.read();
			tmp_src = _src_mat.read();
			_dst1_mat.write(tmp_src);
			_dst2_mat.write(tmp_src);
			_dst1_out_mat.write(tmp_src1);
			_dst2_out_mat.write(tmp_src1);
		}
	}
}

template<int ROWS, int COLS, int DEPTH_SRC,int DEPTH_DST, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST>
void xFPackNMS(hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src_mat,hls::stream< XF_SNAME(WORDWIDTH_DST)>& _dst_mat,uint16_t imgheight,uint16_t imgwidth)
{
	rowLoop:
	for(int i = 0; i < (imgheight); i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for(int j = 0; j < (imgwidth); j=j+4)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS/4 max=COLS/4
#pragma HLS pipeline

			ap_uint<8> P0,P1,P2,P3;
			ap_uint<8> val;

			P0 = _src_mat.read();
			P1 = _src_mat.read();
			P2 = _src_mat.read();
			P3 = _src_mat.read();

			val = ((ap_uint<8>) P0 << (0)) | ((ap_uint<8>)P1 << (2)) | ((ap_uint<8>)P2 << (4)) | ((ap_uint<8>)P3 << (6));

			_dst_mat.write(val);
		}
	}

}

// xFDuplicate_rows

template <int ROWS, int COLS, int DEPTH_IN,int DEPTH_OUT, int NPC,
int WORDWIDTH_SRC, int WORDWIDTH_DST,int TC,int TC1,int FILTER_TYPE,bool USE_URAM>
void xFCannyKernel(
		hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src_mat,
		hls::stream< XF_SNAME(WORDWIDTH_DST)>& _dst_mat,
		unsigned char _lowthreshold, unsigned char _highthreshold, int _norm_type,uint16_t imgheight,uint16_t imgwidth)
{
	if(NPC == 8)
	{
		hls::stream< XF_SNAME(WORDWIDTH_SRC) > gaussian_mat;
		hls::stream< XF_SNAME(XF_128UW) >  gradx_mat;
		hls::stream< XF_SNAME(XF_128UW) >  gradx1_mat;
		hls::stream< XF_SNAME(XF_128UW) >  gradx2_mat;
		hls::stream< XF_SNAME(XF_128UW) >  grady_mat;
		hls::stream< XF_SNAME(XF_128UW) >  grady1_mat;
		hls::stream< XF_SNAME(XF_128UW) >  grady2_mat;

		hls::stream< XF_SNAME(XF_128UW) > magnitude_mat;
		hls::stream< XF_SNAME(XF_64UW) > phase_mat;

		if(_norm_type == 1){
#pragma HLS STREAM variable=&phase_mat depth=TC1
		}else{
#pragma HLS STREAM variable=&magnitude_mat depth=TC1
		}

#pragma HLS DATAFLOW
		xFAverageGaussianMask3x3<ROWS,COLS,DEPTH_IN, NPC, WORDWIDTH_SRC,(COLS>>XF_BITSHIFT(NPC))>(_src_mat,gaussian_mat,imgheight,imgwidth);
		xFSobel<ROWS,COLS,DEPTH_IN,XF_16SP,NPC,WORDWIDTH_SRC,XF_128UW,FILTER_TYPE,USE_URAM>(gaussian_mat,gradx_mat, grady_mat,XF_BORDER_REPLICATE, imgheight, imgwidth);
		xFDuplicate_rows<ROWS, COLS, XF_16SP, NPC, XF_128UW,TC>(gradx_mat,grady_mat,gradx1_mat,gradx2_mat, grady1_mat, grady2_mat,imgheight,imgwidth);
		xFMagnitude<ROWS, COLS, XF_16SP,XF_16SP, NPC, XF_128UW,XF_128UW>(gradx1_mat,grady1_mat,magnitude_mat,_norm_type,imgheight,imgwidth);
		xFAngle<ROWS, COLS, XF_16SP,XF_8UP, NPC, XF_128UW,XF_64UW>(gradx2_mat,grady2_mat,phase_mat,imgheight,imgwidth);
		xFSuppression3x3<ROWS,COLS,XF_16SP,XF_8UP,DEPTH_OUT,NPC,XF_128UW,XF_64UW,WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>
		(magnitude_mat, phase_mat, _dst_mat, _lowthreshold, _highthreshold, imgheight, imgwidth);
	}

	if(NPC == 1)
	{
		hls::stream< XF_SNAME(WORDWIDTH_SRC) > gaussian_mat;
		hls::stream< XF_SNAME(XF_16UW) >  gradx_mat;
		hls::stream< XF_SNAME(XF_16UW) >  gradx1_mat;
		hls::stream< XF_SNAME(XF_16UW) >  gradx2_mat;
		hls::stream< XF_SNAME(XF_16UW) >  grady_mat;
		hls::stream< XF_SNAME(XF_16UW) >  grady1_mat;
		hls::stream< XF_SNAME(XF_16UW) >  grady2_mat;

		hls::stream< XF_SNAME(XF_16UW) > magnitude_mat;
		hls::stream< XF_SNAME(XF_8UW) > phase_mat;
		hls::stream< XF_SNAME(XF_2UW) > nms_mat;

		if(_norm_type == 1){
#pragma HLS STREAM variable=&phase_mat depth=TC1
		}else{
#pragma HLS STREAM variable=&magnitude_mat depth=TC1
		}

#pragma HLS DATAFLOW
		xFAverageGaussianMask3x3<ROWS,COLS,DEPTH_IN, NPC, WORDWIDTH_SRC,(COLS>>XF_BITSHIFT(NPC))>(_src_mat,gaussian_mat,imgheight,imgwidth);
		xFSobel<ROWS,COLS,DEPTH_IN,XF_16SP,NPC,WORDWIDTH_SRC,XF_16UW,FILTER_TYPE,USE_URAM>(gaussian_mat,gradx_mat, grady_mat,XF_BORDER_REPLICATE, imgheight, imgwidth);
		xFDuplicate_rows<ROWS, COLS, XF_16SP, NPC, XF_16UW,	TC>(gradx_mat,grady_mat,gradx1_mat,gradx2_mat, grady1_mat, grady2_mat,imgheight,imgwidth);
		xFMagnitude<ROWS, COLS, XF_16SP,XF_16SP, NPC, XF_16UW,XF_16UW>(gradx1_mat,grady1_mat,magnitude_mat,_norm_type,imgheight,imgwidth);
		xFAngle<ROWS, COLS, XF_16SP,XF_8UP, NPC, XF_16UW,XF_8UW>(gradx2_mat,grady2_mat,phase_mat,imgheight,imgwidth);
		xFSuppression3x3<ROWS,COLS,XF_16SP,XF_8UP,XF_2UP,NPC,XF_16UW,XF_8UW,XF_2UW,(COLS>>XF_BITSHIFT(NPC))>(magnitude_mat,phase_mat,nms_mat,_lowthreshold,_highthreshold,imgheight,imgwidth);
		xFPackNMS<ROWS,COLS,XF_2UP,DEPTH_OUT,NPC,XF_2UW,WORDWIDTH_DST>(nms_mat,_dst_mat,imgheight,imgwidth);
	}

}


/**********************************************************************
 * xFCanny :  Calls the Main Function depends on requirements
 **********************************************************************/
template<int ROWS, int COLS, int DEPTH_IN,int DEPTH_OUT, int NPC,
int WORDWIDTH_SRC, int WORDWIDTH_DST,int FILTER_TYPE,bool USE_URAM>
void xFCannyEdgeDetector(hls::stream< XF_SNAME(WORDWIDTH_SRC)>&   _src_mat,
		hls::stream< XF_SNAME(WORDWIDTH_DST)>& out_strm,
		unsigned char _lowthreshold, unsigned char _highthreshold,int _norm_type, uint16_t imgheight,uint16_t imgwidth)
{

	assert(((_norm_type == XF_L1NORM) || (_norm_type == XF_L2NORM)) &&
			"The _norm_type must be 'XF_L1NORM' or'XF_L2NORM'");


	xFCannyKernel<ROWS, COLS, DEPTH_IN, DEPTH_OUT, NPC, WORDWIDTH_SRC, WORDWIDTH_DST,
	(COLS>>XF_BITSHIFT(NPC)),((COLS>>XF_BITSHIFT(NPC))*3),FILTER_TYPE,USE_URAM>(_src_mat, out_strm, _lowthreshold, _highthreshold,_norm_type,imgheight,imgwidth);

}


#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL, "_dst_mat.data":SEQUENTIAL)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"], "_dst_mat.data"[0:"_dst_mat.size"])

template<int FILTER_TYPE,int NORM_TYPE,int SRC_T,int DST_T, int ROWS, int COLS,int NPC,int NPC1,bool USE_URAM=false>
void Canny(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,xf::Mat<DST_T, ROWS, COLS, NPC1> & _dst_mat,unsigned char _lowthreshold,unsigned char _highthreshold)
{


	hls::stream< XF_TNAME(SRC_T,NPC)> _src;
	hls::stream< XF_TNAME(DST_T,NPC1)> _dst;
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW

	for(int i=0; i<_src_mat.rows;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src_mat.cols)>>(XF_BITSHIFT(NPC));j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
#pragma HLS PIPELINE
#pragma HLS LOOP_FLATTEN off
			_src.write( *(_src_mat.data + i*(_src_mat.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFCannyEdgeDetector<ROWS,COLS,XF_DEPTH(SRC_T,NPC),XF_DEPTH(DST_T,NPC1),NPC,XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC1),FILTER_TYPE,USE_URAM>(_src,_dst,_lowthreshold,_highthreshold,NORM_TYPE,_src_mat.rows,_src_mat.cols);

	for(int i=0; i<_dst_mat.rows;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst_mat.cols)>>(XF_BITSHIFT(NPC1));j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC1
#pragma HLS PIPELINE
#pragma HLS LOOP_FLATTEN off
			*(_dst_mat.data + i*(_dst_mat.cols>>(XF_BITSHIFT(NPC1))) +j) = _dst.read();
		}
	}
}
}

#endif
