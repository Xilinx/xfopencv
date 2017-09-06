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
HOWEVER CxFSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ***************************************************************************/

#ifndef _XF_HARRIS_HPP_
#define _XF_HARRIS_HPP_

#ifndef __cplusplus
#error C++ is needed to use this file!
#endif


#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

#include "imgproc/xf_box_filter.hpp"
#include "imgproc/xf_sobel.hpp"
#include "xf_harris_utils.hpp"
#include "xf_max_suppression.hpp"
#include "xf_pack_corners.hpp"

namespace xf{

/************************************************************************
 * xFCornerHarrisDetector : CornerHarris function to find corners in the image
 ************************************************************************/
template<int ROWS, int COLS, int IN_DEPTH, int NPC, int IN_WW,
int OUT_WW, int MAXPNTS, int TC,int GRAD_WW, int DET_WW>
void xFCornerHarrisDetector(hls::stream < XF_SNAME(IN_WW) > &_src_mat,
		ap_uint<32>  _dst_list[MAXPNTS], uint16_t img_height, uint16_t img_width,
		uint16_t _filter_width, uint16_t _block_width,uint16_t _nms_radius, uint16_t _threshold, uint16_t k, uint32_t *nCorners)
{


	int scale;
	if(_filter_width == XF_FILTER_3X3)
		scale = 6 ;
	else if(_filter_width == XF_FILTER_5X5)
		scale = 12;
	else if (_filter_width == XF_FILTER_7X7)
		scale = 1;

	hls::stream< XF_SNAME(GRAD_WW) > gradx_2("gradx_2"), grady_2("grady_2"), gradxy("gradxy");
	hls::stream< XF_SNAME(GRAD_WW) > gradx2g("gradx2g"), grady2g("grady2g"), gradxyg("gradxyg");
	hls::stream< XF_SNAME(DET_WW) > score("score");
	hls::stream< XF_SNAME(DET_WW) > thresh("thresh");

	hls::stream< XF_SNAME(IN_WW) > maxsup("maxsup");
#pragma HLS DATAFLOW

	if (_filter_width == XF_FILTER_7X7){
		hls::stream< XF_SNAME(DET_WW) > gradx_mat, grady_mat;
		hls::stream< XF_SNAME(DET_WW) > gradx1_mat, grady1_mat;
		hls::stream< XF_SNAME(DET_WW) > gradx2_mat, grady2_mat;
		xFSobelFilter<ROWS, COLS, IN_DEPTH, XF_32SP, NPC, IN_WW, DET_WW>(_src_mat, gradx_mat, grady_mat, _filter_width,XF_BORDER_CONSTANT,img_height, img_width);

		xFDuplicate<ROWS, COLS, XF_32SP, NPC, DET_WW,
		TC>(gradx_mat, gradx1_mat, gradx2_mat, img_height,img_width);

		xFDuplicate<ROWS, COLS, XF_32SP, NPC, DET_WW,
		TC>(grady_mat, grady1_mat, grady2_mat, img_height, img_width);

		xFSquare<ROWS, COLS, XF_32SP, XF_16SP, NPC,
		DET_WW, GRAD_WW, TC>(gradx1_mat, gradx_2, scale, _filter_width, img_height, img_width);

		xFSquare<ROWS, COLS, XF_32SP, XF_16SP, NPC,
		DET_WW, GRAD_WW, TC>(grady1_mat, grady_2, scale, _filter_width, img_height, img_width);

		xFMultiply<ROWS, COLS, XF_32SP, XF_16SP, NPC,
		DET_WW, GRAD_WW, TC>(gradx2_mat, grady2_mat, gradxy, scale, _filter_width, img_height, img_width);
	}else{
		hls::stream< XF_SNAME(GRAD_WW) > gradx_mat, grady_mat;
		hls::stream< XF_SNAME(GRAD_WW) > gradx1_mat, grady1_mat;
		hls::stream< XF_SNAME(GRAD_WW) > gradx2_mat, grady2_mat;

		xFSobelFilter<ROWS, COLS, IN_DEPTH, XF_16SP, NPC, IN_WW, GRAD_WW>(_src_mat, gradx_mat, grady_mat, _filter_width, XF_BORDER_CONSTANT,img_height, img_width);

		xFDuplicate<ROWS, COLS, XF_16SP, NPC, GRAD_WW,
		TC>(gradx_mat, gradx1_mat, gradx2_mat, img_height, img_width);

		xFDuplicate<ROWS, COLS, XF_16SP, NPC, GRAD_WW,
		TC>(grady_mat, grady1_mat, grady2_mat, img_height, img_width);

		xFSquare<ROWS, COLS, XF_16SP, XF_16SP, NPC,
		GRAD_WW, GRAD_WW, TC>(gradx1_mat, gradx_2, scale, _filter_width, img_height, img_width);

		xFSquare<ROWS, COLS, XF_16SP, XF_16SP, NPC,
		GRAD_WW, GRAD_WW, TC>(grady1_mat, grady_2, scale, _filter_width, img_height, img_width);

		xFMultiply<ROWS, COLS, XF_16SP, XF_16SP, NPC,
		GRAD_WW, GRAD_WW, TC>(gradx2_mat, grady2_mat, gradxy, scale, _filter_width, img_height, img_width);
	}

	xFBoxFilterKernel<ROWS, COLS, XF_16SP, NPC, GRAD_WW, GRAD_WW>(gradx_2, gradx2g, _block_width, XF_BORDER_CONSTANT,img_height, img_width);
	xFBoxFilterKernel<ROWS, COLS, XF_16SP, NPC, GRAD_WW, GRAD_WW>(grady_2, grady2g,  _block_width, XF_BORDER_CONSTANT,img_height, img_width);
	xFBoxFilterKernel<ROWS, COLS, XF_16SP, NPC, GRAD_WW, GRAD_WW>(gradxy,  gradxyg,  _block_width, XF_BORDER_CONSTANT,img_height, img_width);

	xFComputeScore<ROWS, COLS, XF_16SP, XF_32SP, NPC,
	GRAD_WW, DET_WW, TC>(gradx2g, grady2g, gradxyg, score, img_height, img_width, k, _filter_width);



	xFThreshold<ROWS, COLS, XF_32SP, NPC, DET_WW, TC>(score, thresh, _threshold, img_height, img_width);

	xFMaxSuppression<ROWS, COLS, XF_32SP, XF_8UP, NPC,
	DET_WW, IN_WW>(thresh, maxsup, _nms_radius, img_height, img_width);

	xFWriteCornersToList<ROWS, COLS, IN_DEPTH, NPC, IN_WW, MAXPNTS,
	OUT_WW, TC>(maxsup, _dst_list, nCorners, img_height, img_width);


}
//
///**********************************************************************
// * xFCornerHarrisTop :  Calls the Main Function depends on requirements
// **********************************************************************/
template<int ROWS, int COLS, int DEPTH, int NPC, int IN_WW, int OUT_WW, int FILTERSIZE,int BLOCKWIDTH, int NMSRADIUS,int MAXPNTS>
void xFCornerHarrisDetection(hls::stream < XF_SNAME(IN_WW) > & _src_mat,
		ap_uint<32> _dst_list[MAXPNTS], uint16_t img_height, uint16_t img_width,
		uint16_t _threshold, uint16_t val, uint32_t *nCorners)
{
	assert(((FILTERSIZE == XF_FILTER_3X3) || (FILTERSIZE == XF_FILTER_5X5) ||
			(FILTERSIZE == XF_FILTER_7X7)) && "filter width must be 3, 5 or 7");

	assert(((BLOCKWIDTH == XF_FILTER_3X3) || (BLOCKWIDTH == XF_FILTER_5X5) ||
			(BLOCKWIDTH == XF_FILTER_7X7)) && "block width must be 3, 5 or 7");

	assert(((NMSRADIUS == XF_NMS_RADIUS_1) || (NMSRADIUS == XF_NMS_RADIUS_2))
			&& "radius size must be 1, 2");

	assert(((img_height <= ROWS ) && (img_width <= COLS)) && "ROWS and COLS should be greater than input image");

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
			" NPC must be 0 or 3 ");


	if(NPC == XF_NPPC8)
	{
		xFCornerHarrisDetector<ROWS, COLS, DEPTH, NPC, IN_WW, OUT_WW,
		MAXPNTS, (COLS>>XF_BITSHIFT(NPC)), XF_128UW, XF_256UW>(_src_mat, _dst_list, img_height, img_width, FILTERSIZE, BLOCKWIDTH, NMSRADIUS, _threshold, val, nCorners);
	}
	else if(NPC == XF_NPPC1)
	{
		xFCornerHarrisDetector<ROWS, COLS, DEPTH,	NPC, IN_WW, OUT_WW,
		MAXPNTS, (COLS>>XF_BITSHIFT(NPC)), XF_16UW, XF_32UW>(_src_mat, _dst_list, img_height, img_width, FILTERSIZE, BLOCKWIDTH, NMSRADIUS, _threshold, val, nCorners);
	}
}
// xFCornerHarrisTop
//
//#pragma SDS data data_mover("src.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("src.data":SEQUENTIAL)
#pragma SDS data access_pattern(points:SEQUENTIAL)
#pragma SDS data copy ("src.data"[0:"src.size"])
#pragma SDS data mem_attribute("src.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)

template<int MAXPNTS,int FILTERSIZE,int BLOCKWIDTH, int NMSRADIUS,int SRC_T,int ROWS, int COLS,int NPC=1>
void cornerHarris(xf::Mat<SRC_T, ROWS, COLS, NPC> & src,ap_uint<32> points[MAXPNTS],uint16_t threshold, uint16_t k, uint32_t *nCorners)
{

#pragma HLS interface ap_fifo port=points

#pragma HLS inline off

	
	hls::stream< XF_TNAME(SRC_T,NPC)> _src;

#pragma HLS DATAFLOW

	for(int i=0; i<src.rows;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src.cols)>>(XF_BITSHIFT(NPC));j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
#pragma HLS LOOP_FLATTEN off
#pragma HLS PIPELINE
			_src.write( *(src.data + i*(src.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFCornerHarrisDetection<ROWS, COLS, XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_32UW, FILTERSIZE,BLOCKWIDTH,NMSRADIUS, MAXPNTS>(_src, points, src.rows, src.cols, threshold, k,nCorners);

}
}

#endif // _XF_HARRIS_HPP_
