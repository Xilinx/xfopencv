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

#ifndef _XF_STRUCTS_H_
#define _XF_STRUCTS_H_

#ifndef __cplusplus
#error C++ is needed to use this file!
#endif

#include <stdio.h>
#include <assert.h>
#include "xf_types.h"
#if __SDSCC__
#include "sds_lib.h"
#endif

namespace xF {

/*  LOCAL STEREO BLOCK MATCHING UTILITY  */
template<int WSIZE, int NDISP, int NDISP_UNIT>
class xFSBMState {
public:
	// pre-filtering (normalization of input images)
	int preFilterType; // =HLS_STEREO_BM_XSOBEL_TEST
	int preFilterSize; // averaging window size: ~5x5..21x21
	int preFilterCap; // the output of pre-filtering is clipped by [-preFilterCap,preFilterCap]

	// correspondence using Sum of Absolute Difference (SAD)
	int SADWindowSize; // ~5x5..21x21 // defined in macro
	int minDisparity;  // minimum disparity (can be negative)
	int numberOfDisparities; // maximum disparity - minimum disparity (> 0)

	// post-filtering
	int textureThreshold;  // the disparity is only computed for pixels
	// with textured enough neighborhood
	int uniquenessRatio;   // accept the computed disparity d* only if
	// SAD(d) >= SAD(d*)*(1 + uniquenessRatio/100.)
	// for any d != d*+/-1 within the search range.
	//int speckleWindowSize; // disparity variation window
	//int speckleRange; // acceptable range of variation in window

	int ndisp_unit;
	int sweepFactor;
	int remainder;

	xFSBMState() {
		preFilterType = XF_STEREO_PREFILTER_SOBEL_TYPE; // Default Sobel filter
		preFilterSize = WSIZE;
		preFilterCap = 31;
		SADWindowSize = WSIZE;
		minDisparity = 0;
		numberOfDisparities = NDISP;
		textureThreshold = 10;
		uniquenessRatio = 15;
		sweepFactor = (NDISP / NDISP_UNIT) + ((NDISP % NDISP_UNIT) != 0);
		ndisp_unit = NDISP_UNIT;
		remainder = NDISP_UNIT * sweepFactor - NDISP;
	}
};

/* Template class of Point_ */
template<typename T>
class Point_ {
public:
	Point_();
	Point_(T _x, T _y);
	Point_(const Point_& pt);
	~Point_();

	T x, y;
};

/* Member functions of Point_ class */
template<typename T> inline Point_<T>::Point_() {
}
template<typename T> inline Point_<T>::Point_(T _x, T _y) :
				x(_x), y(_y) {
}
template<typename T> inline Point_<T>::Point_(const Point_<T>& pt) :
				x(pt.x), y(pt.y) {
}
template<typename T> inline Point_<T>::~Point_() {
}

typedef Point_<int> Point;

/* Template class of Size_ */
template<typename T>
class Size_ {
public:
	Size_();
	Size_(T _width, T _height);
	Size_(const Size_<T>& sz);
	Size_(const Point_<T>& pt);
	T area();
	~Size_();

	T width, height;
};

/* Member functions of Size_ class */
template<typename T> inline Size_<T>::Size_() {
}
template<typename T> inline Size_<T>::Size_(T _width, T _height) :
				width(_width), height(_height) {
}
template<typename T> inline Size_<T>::Size_(const Size_<T>& sz) :
				width(sz.width), height(sz.height) {
}
template<typename T> inline Size_<T>::Size_(const Point_<T>& pt) :
				width(pt.x), height(pt.y) {
}
template<typename T> inline T Size_<T>::area() {
	return width * height;
}
template<typename T> inline Size_<T>::~Size_() {
}

typedef Size_<int> Size;

/* Template class of Rect_ */
template<typename T>
class Rect_ {
public:
	Rect_();
	Rect_(T _x, T _y, T _width, T _height);
	Rect_(const Rect_& rect);
	Rect_(const Point_<T>& pt, const Size_<T>& sz);
	T area();
	Size_<T> size();
	Point_<T> tl(); // top-left point(inside);
	Point_<T> tr(); // top-right point(outside);
	Point_<T> bl(); // bottom-left point(outside);
	Point_<T> br(); // bottom-right point(outside);
	bool bContains(const Point_<T>& pt);
	~Rect_();

	T x, y, width, height;
};

/* Member functions of Rect_ class */
template<typename T> inline Rect_<T>::Rect_() {
}
template<typename T> inline Rect_<T>::Rect_(T _x, T _y, T _width, T _height) :
				x(_x), y(_y), width(_width), height(_height) {
}
template<typename T> inline Rect_<T>::Rect_(const Rect_<T>& rect) :
				x(rect.x), y(rect.y), width(rect.width), height(rect.height) {
}
template<typename T> inline Rect_<T>::Rect_(const Point_<T>& pt,
		const Size_<T>& sz) :
				x(pt.x), y(pt.y), width(sz.width), height(sz.height) {
}
template<typename T> inline T Rect_<T>::area() {
	return width * height;
}
template<typename T> inline Size_<T> Rect_<T>::size() {
	return Size_<T>(width, height);
}
template<typename T> inline Point_<T> Rect_<T>::tl() {
	return Point_<T>(x, y);
}
template<typename T> inline Point_<T> Rect_<T>::tr() {
	return Point_<T>(x + width, y);
}
template<typename T> inline Point_<T> Rect_<T>::bl() {
	return Point_<T>(x, y + height);
}
template<typename T> inline Point_<T> Rect_<T>::br() {
	return Point_<T>(x + width, y + height);
}
template<typename T> inline bool Rect_<T>::bContains(const Point_<T>& pt) {
	return (pt.x >= x && pt.x < x + width && pt.y >= y && pt.y < y + height);
}
template<typename T> inline Rect_<T>::~Rect_() {
}

typedef Rect_<int> Rect;

/* Template class of Scalar */
template<int N, typename T>
class Scalar {
public:
	Scalar() {
#pragma HLS ARRAY_PARTITION variable=val dim=1 complete
		assert(N > 0);
	}
	Scalar(T v0) {
#pragma HLS ARRAY_PARTITION variable=val dim=1 complete
		assert(N >= 1 && "Scalar must have enough channels for constructor.");
		val[0] = v0;
	}
	Scalar(T v0, T v1) {
#pragma HLS ARRAY_PARTITION variable=val dim=1 complete
		assert(N >= 2 && "Scalar must have enough channels for constructor.");
		val[0] = v0;
		val[1] = v1;
	}
	Scalar(T v0, T v1, T v2) {
#pragma HLS ARRAY_PARTITION variable=val dim=1 complete
		assert(N >= 3 && "Scalar must have enough channels for constructor.");
		val[0] = v0;
		val[1] = v1;
		val[2] = v2;
	}
	Scalar(T v0, T v1, T v2, T v3) {
#pragma HLS ARRAY_PARTITION variable=val dim=1 complete
		assert(N >= 4 && "Scalar must have enough channels for constructor.");
		val[0] = v0;
		val[1] = v1;
		val[2] = v2;
		val[3] = v3;
	}

	void operator =(T value);
	Scalar<N, T> operator +(T value);
	Scalar<N, T> operator +(Scalar<N, T> s);
	Scalar<N, T> operator -(T value);
	Scalar<N, T> operator -(Scalar<N, T> s);
	Scalar<N, T> operator *(T value);
	Scalar<N, T> operator *(Scalar<N, T> s);
	Scalar<N, T> operator /(T value);
	Scalar<N, T> operator /(Scalar<N, T> s);

	T val[N];
};

template<int N, typename T>
void Scalar<N, T>::operator =(T value) {
#pragma HLS inline
	for (int k = 0; k < N; k++) {
#pragma HLS unroll
		val[k] = value;
	}
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator +(T value) {
#pragma HLS inline
	Scalar<N, T> res;
	for (int k = 0; k < N; k++) {
#pragma HLS unroll
		res.val[k] = val[k] + value;
	}
	return res;
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator +(Scalar<N, T> s) {
#pragma HLS inline
	Scalar<N, T> res;
	for (int k = 0; k < N; k++) {
#pragma HLS unroll
		res.val[k] = val[k] + s.val[k];
	}
	return res;
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator -(T value) {
#pragma HLS inline
	Scalar<N, T> res;
	for (int k = 0; k < N; k++) {
#pragma HLS unroll
		res.val[k] = val[k] - value;
	}
	return res;
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator -(Scalar<N, T> s) {
#pragma HLS inline
	Scalar<N, T> res;
	for (int k = 0; k < N; k++) {
#pragma HLS unroll
		res.val[k] = val[k] - s.val[k];
	}
	return res;
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator *(T value) {
#pragma HLS inline
	Scalar<N, T> res;
	for (int k = 0; k < N; k++) {
#pragma HLS unroll
		res.val[k] = val[k] * value;
	}
	return res;
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator *(Scalar<N, T> s) {
#pragma HLS inline
	Scalar<N, T> res;
	for (int k = 0; k < N; k++) {
#pragma HLS unroll
		res.val[k] = val[k] * s.val[k];
	}
	return res;
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator /(T value) {
#pragma HLS inline
	Scalar<N, T> res;
	for (int k = 0; k < N; k++) {
#pragma HLS unroll
		res.val[k] = val[k] / value;
	}
	return res;
}

template<int N, typename T>
Scalar<N, T> Scalar<N, T>::operator /(Scalar<N, T> s) {
#pragma HLS inline
	Scalar<N, T> res;
	for (int k = 0; k < N; k++) {
#pragma HLS unroll
		res.val[k] = val[k] / s.val[k];
	}
	return res;
}

/* Template class of Mat */
template<int T, int ROWS, int COLS, int NPC>
class Mat {
public:
	Mat();                         // default constructor

	Mat(int _rows, int _cols);
	Mat(int _size, int _rows, int _cols);
	Mat(int _rows, int _cols, void *_data);
	~Mat();

	void init(int _rows, int _cols);

	void copyTo(XF_PTSNAME(T,NPC)* fromData);
	XF_PTSNAME(T,NPC)* copyFrom();

	int rows, cols,size;               // actual image size

#ifndef __SYNTHESIS__
	XF_TNAME(T,NPC)*data;
#else
	XF_TNAME(T,NPC) data[ROWS*(COLS>> (XF_BITSHIFT(NPC)))];
#endif

};
//Mat

/* Member functions of Mat class */
template<int T, int ROWS, int COLS, int NPPC>
inline Mat<T, ROWS, COLS, NPPC>::Mat() {
#pragma HLS inline
	init(ROWS, COLS);
	size = ROWS * (COLS >> (XF_BITSHIFT(NPPC)));
}

template<int T, int ROWS, int COLS, int NPPC>
inline Mat<T, ROWS, COLS, NPPC>::Mat(int _rows, int _cols, void *_data) {
#pragma HLS inline
	rows = _rows;
	cols = _cols;
	size = _rows * (_cols >> (XF_BITSHIFT(NPPC)));
	data = (XF_TNAME(T,NPPC) *)_data;
}

template<int T, int ROWS, int COLS, int NPPC>
inline Mat<T, ROWS, COLS, NPPC>::Mat(int _rows, int _cols) {
#pragma HLS inline
	init(_rows, _cols);
	rows = _rows;
	cols = _cols;
	size = _rows * (_cols >> (XF_BITSHIFT(NPPC)));
}

template<int T, int ROWS, int COLS, int NPPC>
inline Mat<T, ROWS, COLS, NPPC>::Mat(int _sz, int _rows, int _cols) {
#pragma HLS inline
	//init(_sz.height, _sz.width);
	rows = _rows;
	cols = _cols;
	size = _rows * (_cols >> (XF_BITSHIFT(NPPC)));
}

template<int T, int ROWS, int COLS, int NPPC>
inline void Mat<T, ROWS, COLS, NPPC>::copyTo(XF_PTSNAME(T,NPPC)*_input) {

#pragma HLS inline

	//checking if the number of bytes to copy is a multiple of 8, to use memcpy to copy from _input to data
	if( (rows*(cols>>XF_BITSHIFT(NPPC))*(sizeof(XF_TNAME(T,NPPC))))%8 == 0 )
	{
		memcpy(data, _input, rows*(cols>>XF_BITSHIFT(NPPC))*(sizeof(XF_TNAME(T,NPPC))));
	}
	else
	{
		XF_TNAME(T,NPPC) *input_pointer;
		input_pointer = (XF_TNAME(T,NPPC) *) _input;
		for(short i=0;i<(rows*(cols>>(XF_BITSHIFT(NPPC))));i++)
		{
			data[i] = input_pointer[i];
		}
	}
}

template<int T, int ROWS, int COLS, int NPPC>
inline XF_PTSNAME(T,NPPC)* Mat<T, ROWS, COLS, NPPC>::copyFrom() {
#pragma HLS inline

	return (XF_PTSNAME(T,NPPC)*)data;

}

template<int T, int ROWS, int COLS, int NPPC>
inline void Mat<T, ROWS, COLS, NPPC>::init(int _rows, int _cols) {
#pragma HLS inline
	assert(
			(_rows > 0) && (_rows <= ROWS) && (_cols > 0) && (_cols <= COLS)
			&& "The number of rows and columns must be less than the template arguments.");
	rows = _rows;
	cols = _cols;
#ifndef __SYNTHESIS__
#ifdef __SDSCC__
	data = (XF_TNAME(T,NPPC)*)sds_alloc_non_cacheable(rows*(cols>>(XF_BITSHIFT(NPPC)))*(sizeof(XF_TNAME(T,NPPC))));
#else
	data = (XF_TNAME(T,NPPC)*)malloc(rows*(cols>>(XF_BITSHIFT(NPPC)))*(sizeof(XF_TNAME(T,NPPC))));
#endif

#endif
	if (data == NULL) {
		fprintf(stderr, "\nFailed to allocate memory\n");
	}
}

template<int SRC_T, int ROWS, int COLS, int NPC>
inline Mat<SRC_T, ROWS, COLS, NPC>::~Mat() {

#ifndef __SYNTHESIS__
	if (data != NULL) {
#ifdef __SDSCC__
		sds_free(data);
#else
		free(data);
#endif
	}
#endif
}

}
;
//xF
#endif//_XF_STRUCTS_H_
