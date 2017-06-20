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

#ifndef _XF_STEREO_PIPELINE_HPP_
#define _XF_STEREO_PIPELINE_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "hls_video.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "imgproc/xf_stereoBM.hpp"
#include "imgproc/xf_remap.hpp"


#define _XF_INTER_BITS_ 5

template <typename FRAMET, typename FRAME2T, typename ROWT, typename COLT, typename ROWOUTT, typename COLOUTT, typename CM_T, int CM_SIZE, int N>
void xFComputeUndistortCoordinates(
		CM_T *cameraMatrix,
		CM_T *distCoeffs,
		CM_T *ir,
		int noRotation,
		ROWT i, COLT j,
		ROWOUTT &u, COLOUTT &v) {
	CM_T zo = 0;
	CM_T k1 = distCoeffs[0];
	CM_T k2 = distCoeffs[1];
	CM_T p1 = distCoeffs[2];
	CM_T p2 = distCoeffs[3];
	CM_T k3 = N>=5? distCoeffs[4] : zo;
	CM_T k4 = N>=8? distCoeffs[5] : zo;
	CM_T k5 = N>=8? distCoeffs[6] : zo;
	CM_T k6 = N>=8? distCoeffs[7] : zo;

	CM_T u0 = cameraMatrix[2];
	CM_T v0 = cameraMatrix[5];
	CM_T fx = cameraMatrix[0];
	CM_T fy = cameraMatrix[4];

	// FRAMET is the type of normalized coordinates.
	// If IR is float, then FRAMET will also be float
	// If IR is ap_fixed, then FRAMET will be some ap_fixed type with more integer bits
	//             typedef typename x_traits<typename x_traits<ROWT,ICMT>::MULT_T,
	//                                       typename x_traits<COLT,ICMT>::MULT_T >::ADD_T FRAMET;
	//    typedef ap_fixed<18,2, AP_TRN, AP_SAT> FRAMET; // Assume frame coordinates are in [-1,1)
	//    typedef CMT FRAMET;
	//typedef float FRAMET;

	FRAMET _x, _y, x, y;
	_x=i*ir[1] + j * ir[0] + ir[2];
	_y=i*ir[4] + j * ir[3] + ir[5];

	if(noRotation) {
		// A special case if there is no rotation: equivalent to cv::initUndistortMap
		x=_x;
		y=_y;
	} else {
		FRAMET w=i*ir[7] + j * ir[6] + ir[8];
		FRAMET winv = hls::one_over_x_approx(w);
		x = (FRAMET)(_x*winv);
		y = (FRAMET)(_y*winv);
	}

	typename hls::x_traits<FRAMET,FRAMET>::MULT_T x2t = x*x, y2t = y*y;  // Full precision result here.
	FRAME2T _2xy = 2*x*y;
	FRAME2T r2 = x2t + y2t;
	FRAME2T x2 = x2t, y2 = y2t;

	FRAMET kr = (1 + FRAMET(FRAMET(k3*r2 + k2)*r2 + k1)*r2);
	FRAME2T krd = FRAMET(FRAMET(k6*r2 + k5)*r2 + k4)*r2;

	if(N >5) kr = kr*hls::one_over_one_plus_x_approx(krd);

	u = fx*(FRAMET(x*kr) + FRAMET(p1*_2xy) + FRAMET(p2*(2*x2 + r2))) + u0;
	v = fy*(FRAMET(y*kr) + FRAMET(p1*(r2 + 2*y2)) + FRAMET(p2*_2xy)) + v0;
}

template< int ROWS, int COLS, int CM_SIZE, typename CM_T, int N, typename MAP_T >
void xFInitUndistortRectifyMapInverse (
		CM_T *cameraMatrix,
		CM_T *distCoeffs,
		CM_T *ir,
		hls::stream< MAP_T >  &map1,
		hls::stream< MAP_T >  &map2,
		uint16_t rows, uint16_t cols,
		int noRotation=false)
{
#pragma HLS INLINE OFF

	CM_T cameraMatrixHLS[CM_SIZE];
	CM_T distCoeffsHLS[N];
	CM_T iRnewCameraMatrixHLS[CM_SIZE];
#pragma HLS ARRAY_PARTITION variable=cameraMatrixHLS complete dim=0
#pragma HLS ARRAY_PARTITION variable=distCoeffsHLS complete dim=0
#pragma HLS ARRAY_PARTITION variable=iRnewCameraMatrixHLS complete dim=0

	memcpy(cameraMatrixHLS,cameraMatrix,4*CM_SIZE);
	memcpy(distCoeffsHLS,distCoeffs,4*N);
	memcpy(iRnewCameraMatrixHLS,ir,4*CM_SIZE);

	MAP_T mx;
	MAP_T my;

	assert(rows <= ROWS);
	assert(cols <= COLS);

	static hls::RangeAnalyzer<float> rau, rav;
	static hls::RangeAnalyzer<float> rauerr, raverr;

	loop_height: for(int i=0; i< rows; i++) {
		loop_width: for(int j=0; j< cols; j++) {
#pragma HLS PIPELINE II=1
			typedef ap_uint<BitWidth<ROWS>::Value> ROWT;
			typedef ap_uint<BitWidth<COLS>::Value> COLT;
			ROWT ifixed = i;
			COLT jfixed = j;

			ap_fixed<1+BitWidth<COLS>::Value+_XF_INTER_BITS_, 1+BitWidth<COLS>::Value, AP_RND, AP_SAT> u;
			ap_fixed<1+BitWidth<ROWS>::Value+_XF_INTER_BITS_, 1+BitWidth<ROWS>::Value, AP_RND, AP_SAT> v;
			xFComputeUndistortCoordinates
			<typename hls::InitUndistortRectifyMap_traits<CM_T>::FRAMET, typename hls::InitUndistortRectifyMap_traits<CM_T>::FRAME2T,
			ROWT,COLT,ap_fixed<1+BitWidth<COLS>::Value+_XF_INTER_BITS_, 1+BitWidth<COLS>::Value, AP_RND, AP_SAT>,ap_fixed<1+BitWidth<ROWS>::Value+_XF_INTER_BITS_, 1+BitWidth<ROWS>::Value, AP_RND, AP_SAT>,
			CM_T,CM_SIZE,N>
			(cameraMatrixHLS,distCoeffsHLS,iRnewCameraMatrixHLS,noRotation,ifixed,jfixed,u,v);

			mx = u;
			my = v;

			map1 << mx;
			map2 << my;
		}
	}
}


template <int ROWS, int COLS, int REMAP_WIN_ROWS,typename PARAM_T, int SRC_T, int DST_T, int NPC, int CM_SIZE, int DC_SIZE>
void xFStereoRectificationKernel(
		XF_TNAME(SRC_T,NPC) *_left_ptr,
		XF_TNAME(SRC_T,NPC) *_right_ptr,
		PARAM_T *cameraMatrixLeft,
		PARAM_T *cameraMatrixRight,
		PARAM_T *distCoeffsLeft,
		PARAM_T *distCoeffsRight,
		PARAM_T *iRnewCameraMatrixLeft,
		PARAM_T *iRnewCameraMatrixRight,
		hls::stream< XF_TNAME(DST_T,NPC)>  &_remapped_left_strm,
		hls::stream< XF_TNAME(DST_T,NPC)>  &_remapped_right_strm,
		uint16_t rows, uint16_t cols)
{
#pragma HLS INLINE

	hls::stream< float > mapx_left;
	hls::stream< float > mapy_left;
	hls::stream< float > mapx_right;
	hls::stream< float > mapy_right;


	hls::stream< XF_TNAME(SRC_T,NPC)> _left_strm;
	hls::stream< XF_TNAME(SRC_T,NPC)> _right_strm;

#pragma HLS dataflow

	int TC=(ROWS*COLS);
	for (int i = 0; i < rows*cols; i++)
	{
#pragma HLS pipeline ii=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=TC
		_left_strm.write(*(_left_ptr + i));
		_right_strm.write(*(_right_ptr + i));
	}


	xFInitUndistortRectifyMapInverse<ROWS,COLS,CM_SIZE,PARAM_T,DC_SIZE,float>(cameraMatrixLeft, distCoeffsLeft, iRnewCameraMatrixLeft, mapx_left, mapy_left, rows, cols);
	xFRemapKernel<REMAP_WIN_ROWS,ROWS,COLS>(_left_strm, _remapped_left_strm, mapx_left, mapy_left, XF_INTERPOLATION_BILINEAR, rows, cols);
	xFInitUndistortRectifyMapInverse<ROWS,COLS,CM_SIZE,PARAM_T,DC_SIZE,float>(cameraMatrixRight, distCoeffsRight, iRnewCameraMatrixRight, mapx_right, mapy_right, rows, cols);
	xFRemapKernel<REMAP_WIN_ROWS,ROWS,COLS>(_right_strm, _remapped_right_strm, mapx_right, mapy_right, XF_INTERPOLATION_BILINEAR, rows, cols);
}


#pragma SDS data data_mover("_left_mat.data":AXIDMA_SIMPLE,"_right_mat.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_left_mat.data":SEQUENTIAL,"_right_mat.data":SEQUENTIAL)
#pragma SDS data data_mover("_disp_mat.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_disp_mat.data":SEQUENTIAL)
#pragma SDS data copy("_left_mat.data"[0:"_left_mat.size"])
#pragma SDS data copy("_right_mat.data"[0:"_right_mat.size"])
#pragma SDS data copy("_disp_mat.data"[0:"_disp_mat.size"])

#pragma SDS data zero_copy(cameraMatrixLeft[0:_cm_size])
#pragma SDS data zero_copy(cameraMatrixRight[0:_cm_size])
#pragma SDS data zero_copy(iRnewCameraMatrixLeft[0:_cm_size])
#pragma SDS data zero_copy(iRnewCameraMatrixRight[0:_cm_size])
#pragma SDS data zero_copy(distCoeffsLeft[0:_dc_size])
#pragma SDS data zero_copy(distCoeffsRight[0:_dc_size])

template <int REMAP_WIN_ROWS, int ROWS, int COLS, int SRC_T, int DST_T, int NPC = XF_NPPC1,
		int CM_SIZE, int DC_SIZE, int WSIZE, int NDISP, int NDISP_UNIT>
void xFStereoPipeline(
		xF::Mat<SRC_T, ROWS, COLS, NPC> &_left_mat,
		xF::Mat<SRC_T, ROWS, COLS, NPC> &_right_mat,
		xF::Mat<DST_T, ROWS, COLS, NPC> &_disp_mat,
		xF::xFSBMState<WSIZE,NDISP,NDISP_UNIT> &sbmstate,
		ap_fixed<32,12> *cameraMatrixLeft,
		ap_fixed<32,12> *cameraMatrixRight,
		ap_fixed<32,12> *distCoeffsLeft,
		ap_fixed<32,12> *distCoeffsRight,
		ap_fixed<32,12> *iRnewCameraMatrixLeft,
		ap_fixed<32,12> *iRnewCameraMatrixRight,
		int _cm_size, int _dc_size)
{
#pragma HLS INLINE OFF

	int rows = _left_mat.rows;
	int cols = _left_mat.cols;

	hls::stream<XF_TNAME(SRC_T,NPC)> _left_rect_strm;
	hls::stream<XF_TNAME(SRC_T,NPC)> _right_rect_strm;

#pragma HLS dataflow

	xFStereoRectificationKernel<ROWS,COLS,REMAP_WIN_ROWS,ap_fixed<32,12>,SRC_T,SRC_T,NPC,CM_SIZE,DC_SIZE >(_left_mat.data, _right_mat.data, cameraMatrixLeft, cameraMatrixRight, distCoeffsLeft, distCoeffsRight,
			iRnewCameraMatrixLeft, iRnewCameraMatrixRight, _left_rect_strm, _right_rect_strm, rows, cols);

	xFFindStereoCorrespondenceLBM_pipeline<ROWS,COLS,SRC_T,DST_T,NPC,WSIZE,NDISP,NDISP_UNIT>(_left_rect_strm,_right_rect_strm,_disp_mat.data,sbmstate,
			_left_mat.rows,_left_mat.cols);
}

#endif  // _XF_STEREO_PIPELINE_HPP_


