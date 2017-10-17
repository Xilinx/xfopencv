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

#ifndef _XF_MIN_MAX_LOC_HPP_
#define _XF_MIN_MAX_LOC_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif


/**
 * xfMinMaxLoc_core : This kernal finds the minimum and maximum values in
 * an image and a location for each.
 * Inputs : _src of type XF_8UP, XF_16UP, XF_16SP, XF_32UP or XF_32SP.
 * Output :
 * _maxval and _minval --> minimum and maximum pixel values respectively
 * _minloc and _maxloc --> the row and column positions for min and max
 * 							respectively
 */
 #include "hls_stream.h"
 #include "common/xf_common.h"

namespace xf{

template <int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH,int COL_TRIP>
void xFMinMaxLoc_core(hls::stream<XF_SNAME(WORDWIDTH) >& _src,
		int &_minval1,  int &_maxval1,unsigned short int &_minlocx,unsigned short int &_minlocy,
		unsigned short int &_maxlocx,unsigned short int &_maxlocy,uint16_t height,uint16_t width)
{
//	unsigned short int  _minlocx,_minlocy, _maxlocx,_maxlocy;
	int   _minval,_maxval;

	ap_uint<13> i,j,k,l,col_ind = 0;
	XF_SNAME(WORDWIDTH) val_in;

	ap_uint<13> min_loc_tmp_x = 0, min_loc_tmp_y = 0;
	uint16_t max_loc_tmp_x = 0, max_loc_tmp_y = 0;

	ap_uint<6> step = XF_PIXELDEPTH(DEPTH);

	// initializing the minimum location with the maximum possible value
	//_minloc.x = ((1 << 16) - 1);
	//_minloc.y = ((1 << 16) - 1);
	_minlocx = ((1 << 16) - 1);
	_minlocy = ((1 << 16) - 1);

	// initializing the maximum location with the maximum possible value
	//	_maxloc.x = ((1 << 16) - 1);
	//	_maxloc.y = ((1 << 16) - 1);
	_maxlocx = ((1 << 16) - 1);
	_maxlocy = ((1 << 16) - 1);
	// initializing minval with the maximum value
	_minval = ((1 << (step-1)) - 1);

	// initializing maxval with the minimum value
	_maxval = -(1 << (step-1));

	// temporary arrays to hold the min and max vals
	int32_t  min_val_tmp[(1<<XF_BITSHIFT(NPC))+1], max_val_tmp[(1<<XF_BITSHIFT(NPC))+1];
	ap_uint<26> min_loc_tmp[(1<<XF_BITSHIFT(NPC))+1], max_loc_tmp[(1<<XF_BITSHIFT(NPC))+1];
#pragma HLS ARRAY_PARTITION variable=min_val_tmp complete
#pragma HLS ARRAY_PARTITION variable=max_val_tmp complete
#pragma HLS ARRAY_PARTITION variable=min_loc_tmp complete
#pragma HLS ARRAY_PARTITION variable=max_loc_tmp complete

	// creating an array for minimum and maximum values, depending upon the type of optimization
	fillTempBuf:
	for( i = 0; i < (1<<XF_BITSHIFT(NPC)); i++)
	{
#pragma HLS unroll
		min_val_tmp[i] = ((1 << (step-1)) - 1);
		max_val_tmp[i] = -(1 << (step-1));
	}

	rowLoop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		col_ind = 0;
		colLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COL_TRIP max=COL_TRIP
#pragma HLS pipeline

			// reading the data from the stream
			val_in = _src.read();

			XF_PTNAME(DEPTH) pixel_buf[(1<<XF_BITSHIFT(NPC))+1];
#pragma HLS ARRAY_PARTITION variable=pixel_buf complete dim=1

			ap_uint<9> k = 0;
			processLoop:
			for ( l = 0; l < (1 << XF_BITSHIFT(NPC)); l++)
			{
#pragma HLS unroll
				// packing the data from val_in to pixel buffer
				pixel_buf[l] = val_in.range(k + (step-1), k);

				if(pixel_buf[l] < min_val_tmp[l])
				{
					min_val_tmp[l] = pixel_buf[l];
					min_loc_tmp[l].range(12,0)  = i;
					min_loc_tmp[l].range(25,13) = col_ind;
				}
				if(pixel_buf[l] > max_val_tmp[l])
				{
					max_val_tmp[l] = pixel_buf[l];
					max_loc_tmp[l].range(12,0)  = i;
					max_loc_tmp[l].range(25,13) = col_ind;
				}
				k += step;

				// increment if RO and PO cases
				if(NPC)
				{
					col_ind++;
				}
			}

			// increment if NO case
			if(!NPC)
			{
				col_ind++;
			}
		}
	}

	// tracking the minimum and maximum from the resultant min and max buffers
	trackLoop:
	for( k = 0; k < (1<<XF_BITSHIFT(NPC)); k++)
	{
		// tracking the minimum value and the corresponding location
		if(min_val_tmp[k] < _minval)
		{
			_minval = min_val_tmp[k];
			_minlocx = /*(uint16_t)*/(min_loc_tmp[k].range(12,0));
			_minlocy = /*(uint16_t)*/(min_loc_tmp[k].range(25,13));

		}
		else if(min_val_tmp[k] <= _minval)
		{
			min_loc_tmp_x = /*(uint16_t)*/(min_loc_tmp[k].range(12,0));
			min_loc_tmp_y = /*(uint16_t)*/(min_loc_tmp[k].range(25,13));

			if(min_loc_tmp_x < _minlocx)
			{
				_minval = min_val_tmp[k];
				_minlocx = min_loc_tmp_x;
				_minlocy = min_loc_tmp_y;
			}

			else if(min_loc_tmp_x == _minlocx)
			{
				if(min_loc_tmp_y < _minlocy)
				{
					_minval = min_val_tmp[k];
					_minlocx = min_loc_tmp_x;
					_minlocy = min_loc_tmp_y;
				}
			}
		}


		// tracking the maximum value and the corresponding location
		if(max_val_tmp[k] > _maxval)
		{
			_maxval = max_val_tmp[k];
			_maxlocx = /*(uint16_t)*/(max_loc_tmp[k].range(12,0));
			_maxlocy = /*(uint16_t)*/(max_loc_tmp[k].range(25,13));
		}
		else if(max_val_tmp[k] >= _maxval)
		{
			max_loc_tmp_x = (uint16_t)(max_loc_tmp[k].range(12,0));
			max_loc_tmp_y = (uint16_t)(max_loc_tmp[k].range(25,13));

			if(max_loc_tmp_x < _maxlocx)
			{
				_maxval = max_val_tmp[k];
				_maxlocx = max_loc_tmp_x;
				_maxlocy = max_loc_tmp_y;
			}

			else if(max_loc_tmp_x == _maxlocx)
			{
				if(max_loc_tmp_y < _maxlocy)
				{
					_maxval = max_val_tmp[k];
					_maxlocx = max_loc_tmp_x;
					_maxlocy = max_loc_tmp_y;
				}
			}
		}
	}


	_minval1 =_minval;
	_maxval1=_maxval;
//	*_minlocx1=_minlocx;
//	*_minlocy1=_minlocy;
//	*_maxlocx1=_maxlocx;
//	*_maxlocy1=_maxlocy;
}

/**
 * xfMinMaxLocKernel: this function acts as a wrapper and calls the kernel function
 *  xfMinMaxLoc_core.
 */
template <int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH>
void xFMinMaxLocKernel(hls::stream < XF_SNAME(WORDWIDTH) >& _src,
		int &_minval, int &_maxval,unsigned short int &_minlocx,unsigned short int &_minlocy,
		unsigned short int &_maxlocx,unsigned short int &_maxlocy,uint16_t height, uint16_t width)
{
//#pragma HLS license key=IPAUVIZ_CV_BASIC
	assert(((DEPTH == XF_8UP) || (DEPTH == XF_16UP) || (DEPTH == XF_16SP) ||
			(DEPTH == XF_32UP) || (DEPTH == XF_32SP)) &&
			"Depth must be XF_8UP, XF_16UP, XF_16SP, XF_32UP or XF_32SP");
	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8) ) &&
			"NPC must be XF_NPPC1, XF_NPPC8 or XF_NPPC16");
	assert(
			((WORDWIDTH == XF_8UW) || (WORDWIDTH == XF_16UW)  ||
			(WORDWIDTH == XF_32UW)  || (WORDWIDTH == XF_64UW)  ||
			(WORDWIDTH == XF_128UW) || (WORDWIDTH == XF_256UW) ||
			(WORDWIDTH == XF_512UW)) &&
			"WORDWIDTH must be XF_8UW, XF_16UW, XF_32UW, XF_64UW, XF_128UW, XF_256UW or XF_512UW");

	assert(((height <= ROWS ) && (width <= COLS)) && "ROWS and COLS should be greater than input image");

	width=width>>XF_BITSHIFT(NPC);
//
//	if((NPC==XF_NPPC8) && ((DEPTH == XF_16UP)||(DEPTH == XF_16SP)))
//	{
//		hls::stream< ap_uint<128> > tmp;
//
//#pragma HLS DATAFLOW
//		Convert64To128<ROWS,COLS,NPC,WORDWIDTH,XF_128UW,(COLS>>XF_BITSHIFT(NPC))>(_src, tmp,height,width);
//
//		auMinMaxLocKernel<ROWS,COLS,DEPTH,NPC,XF_128UW,(COLS>>XF_BITSHIFT(NPC))>(tmp, _minval,_maxval,_minlocx,_minlocy,_maxlocx,_maxlocy,height,width);
//
//
//	}
//	else if((NPC==XF_NPPC8) &&((DEPTH == XF_32UP)||(DEPTH == XF_32SP)))
//	{
//		hls::stream< ap_uint<256> > tmp;
//#pragma HLS DATAFLOW
//		Convert64To256<ROWS,COLS,NPC,WORDWIDTH,XF_256UW,(COLS>>XF_BITSHIFT(NPC))>(_src, tmp,height,width);
//
//		auMinMaxLocKernel<ROWS,COLS,DEPTH,NPC,XF_256UW,(COLS>>XF_BITSHIFT(NPC))>(tmp,_minval,_maxval,_minlocx,_minlocy,_maxlocx,_maxlocy,height,width);
//	}
//
//	else

	xFMinMaxLoc_core<ROWS,COLS,DEPTH,NPC,WORDWIDTH,(COLS>>XF_BITSHIFT(NPC))>(_src,_minval,_maxval,_minlocx,_minlocy,_maxlocx,_maxlocy,height,width);

}
//#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
template<int SRC_T,int ROWS,int COLS,int NPC=0>
void minMaxLoc(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,int32_t *min_value, int32_t *max_value,uint16_t *_minlocx, uint16_t *_minlocy, uint16_t *_maxlocx, uint16_t *_maxlocy )
{


#pragma HLS inline off


#pragma HLS dataflow
	hls::stream<XF_TNAME(SRC_T, NPC)> _src_stream;
	int TC = (ROWS * COLS >> XF_BITSHIFT(NPC));
	Read_Mat_Loop: for (int i = 0; i < (_src.rows * (_src.cols >> XF_BITSHIFT(NPC))); i++) {
#pragma HLS pipeline ii=1
#pragma HLS LOOP_TRIPCOUNT min=1 max=TC
		_src_stream.write(*(_src.data + i));

	}
	uint16_t _min_locx,_min_locy,_max_locx,_max_locy;
	int32_t _min_val,_max_val;

	xFMinMaxLocKernel<ROWS, COLS, XF_DEPTH(SRC_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC)>(_src_stream,_min_val,_max_val,_min_locx,_min_locy,_max_locx,_max_locy,_src.rows,_src.cols);

	  *min_value =_min_val;
	  *max_value=_max_val;
	  *_minlocx=_min_locx;
	  *_minlocy=_min_locy;
	  *_maxlocx=_max_locx;
	  *_maxlocy=_max_locy;
}
}
#endif
