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

#ifndef _XF_CHANNEL_EXTRACT_HPP_
#define _XF_CHANNEL_EXTRACT_HPP_

#include "hls_stream.h"
#include "ap_int.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

namespace xf{

/*****************************************************************************
 * 	xFChannelExtract: Extracts one channel from a multiple _channel image
 *
 *	# Parameters
 *	_src	  :	 source image as stream
 *	_dst	  :	 destination image as stream
 * 	_channel :  enumeration specified in < xf_channel_extract_e >
 ****************************************************************************/
template <int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC, int TC>
void xfChannelExtractKernel(
		hls::stream< XF_TNAME(DEPTH_SRC, NPC) >&   _src,
		hls::stream< XF_TNAME(DEPTH_DST, NPC) >& _dst,
		uint16_t _channel,uint16_t height,uint16_t width)
{
#define XF_STEP 8

	ap_uint<13> i,j,k;
	XF_TNAME(DEPTH_SRC, NPC) in_pix;
	XF_TNAME(DEPTH_DST, NPC) out_pix;
	ap_uint<XF_PIXELDEPTH(DEPTH_DST)> result;
	int shift = 0;

	if(_channel==XF_EXTRACT_CH_0 | _channel==XF_EXTRACT_CH_R | _channel==XF_EXTRACT_CH_Y)
	{
		shift = 0;
	}
	else if(_channel==XF_EXTRACT_CH_1 | _channel==XF_EXTRACT_CH_G | _channel==XF_EXTRACT_CH_U)
	{
		shift = 8;
	}
	else if(_channel==XF_EXTRACT_CH_2 | _channel==XF_EXTRACT_CH_B | _channel==XF_EXTRACT_CH_V)
	{
		shift = 16;
	}
	else if(_channel==XF_EXTRACT_CH_3 | _channel==XF_EXTRACT_CH_A)
	{
		shift = 24;
	}

	RowLoop:
	for( i = 0; i < height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off
		ColLoop:
		for( j = 0; j < width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
			int y;
			in_pix = (XF_TNAME(DEPTH_SRC, NPC))(_src.read());

			ProcLoop:
			for( k = 0; k < (8<<XF_BITSHIFT(NPC)); k += XF_STEP)
			{
#pragma HLS unroll
				y = k << 2;
				result = in_pix.range(y+shift+7, y+shift);
				out_pix.range(k+(XF_STEP-1), k) = result;
			}

			_dst.write((XF_TNAME(DEPTH_DST, NPC))out_pix);
		}//ColLoop
	}//RowLoop
}
//xfChannelExtractKernel

template <int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC>
void xfChannelExtract(
		hls::stream< XF_TNAME(DEPTH_SRC, NPC) > & _src,
		hls::stream< XF_TNAME(DEPTH_DST, NPC) >& _dst,
		uint16_t _channel, uint16_t height, uint16_t width)
{
	assert(((_channel == XF_EXTRACT_CH_0) || (_channel == XF_EXTRACT_CH_1) || (_channel == XF_EXTRACT_CH_2) ||
		    (_channel == XF_EXTRACT_CH_3) || (_channel == XF_EXTRACT_CH_R) || (_channel == XF_EXTRACT_CH_G) ||
		    (_channel == XF_EXTRACT_CH_B) || (_channel == XF_EXTRACT_CH_A) || (_channel == XF_EXTRACT_CH_Y) ||
		    (_channel == XF_EXTRACT_CH_U) || (_channel == XF_EXTRACT_CH_V)) && "Invalid Channel Value. See xf_channel_extract_e enumerated type");

	width=width>>XF_BITSHIFT(NPC);

	if ((NPC==XF_NPPC4))
	{
		xfChannelExtractKernel<ROWS, COLS, DEPTH_SRC, DEPTH_DST, NPC,(COLS>>XF_BITSHIFT(NPC))>
		(_src, _dst, _channel, height, width);

	}
	else
	{
		xfChannelExtractKernel<ROWS, COLS, DEPTH_SRC, DEPTH_DST, NPC, (COLS>>XF_BITSHIFT(NPC))>
		(_src, _dst, _channel, height, width);
	}
}

//#pragma SDS data data_mover("_src_mat.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst_mat.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL)
#pragma SDS data access_pattern("_dst_mat.data":SEQUENTIAL)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"])
#pragma SDS data copy("_dst_mat.data"[0:"_dst_mat.size"])

template<int SRC_T, int DST_T, int ROWS, int COLS, int NPC=1>
void extractChannel(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat, xf::Mat<DST_T, ROWS, COLS, NPC> & _dst_mat, uint16_t _channel)
{
	hls::stream< XF_TNAME(SRC_T,NPC)> _src;
	hls::stream< XF_TNAME(DST_T,NPC)> _dst;

#pragma HLS INLINE OFF
#pragma HLS DATAFLOW

	Read_Mat_Loop:
	for(int i=0; i<_src_mat.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src_mat.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			_src.write( *(_src_mat.data + i*(_src_mat.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xfChannelExtract<ROWS, COLS, SRC_T, DST_T, NPC>(_src, _dst, _channel, _src_mat.rows, _src_mat.cols);

	Write_Mat_Loop:
	for(int i=0; i<_dst_mat.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst_mat.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			XF_TNAME(DST_T,NPC) outpix = _dst.read();
			*(_dst_mat.data + i*(_dst_mat.cols>>(XF_BITSHIFT(NPC))) +j) = outpix;
		}
	}
}
}

#endif//_XF_CHANNEL_EXTRACT_HPP_
