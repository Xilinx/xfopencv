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

#ifndef _XF_RESIZE_
#define _XF_RESIZE_

#include "xf_resize_headers.h"

/**
 * Image resizing function.
 */
namespace xf {
template<int SRC_ROWS,int SRC_COLS,int DEPTH,int NPC,int WORDWIDTH_SRC,int DST_ROWS,int DST_COLS>
void xFresize(hls::stream <XF_TNAME(DEPTH,NPC)> &_in_mat,
		hls::stream <XF_TNAME(DEPTH,NPC)> &_out_mat,
		int _interpolation_type, unsigned short height, unsigned short width, unsigned short out_height, unsigned short out_width)
{
#pragma HLS INLINE OFF
	width = width >> XF_BITSHIFT(NPC);
	out_width = out_width >> XF_BITSHIFT(NPC);

	assert(((_interpolation_type == XF_INTERPOLATION_BILINEAR) ||
			(_interpolation_type == XF_INTERPOLATION_NN) ||
			(_interpolation_type == XF_INTERPOLATION_AREA))
			&& "Incorrect parameters interpolation type");

	assert( ((NPC == XF_NPPC8) || (NPC == XF_NPPC1) )  && "Supported Operation Modes XF_NPPC8 and XF_NPPC1");

	assert( ( (!(height & 7)) || (!(width & 7)) )
			&& "Input and ouput image widths should be multiples of 8");

	assert(((height <= SRC_ROWS ) && (width <= SRC_COLS)) && "SRC_ROWS and SRC_COLS should be greater than input image");

	assert(((out_height <= DST_ROWS ) && (out_width <= DST_COLS)) && "DST_ROWS and DST_COLS should be greater than output image");

	if (_interpolation_type == XF_INTERPOLATION_NN)
	{
		if ((SRC_ROWS < DST_ROWS) || (SRC_COLS < DST_COLS)){
			xFResizeNNUpScale<SRC_ROWS,SRC_COLS,DEPTH,NPC,WORDWIDTH_SRC,   \
			DST_ROWS,DST_COLS,(SRC_COLS>>XF_BITSHIFT(NPC)),(DST_COLS>>XF_BITSHIFT(NPC))> \
			(_in_mat,_out_mat, height, width, out_height, out_width);
		}
		else if ((SRC_ROWS > DST_ROWS) || (SRC_COLS > DST_COLS)){
			xFResizeNNDownScale<SRC_ROWS,SRC_COLS,DEPTH,NPC,WORDWIDTH_SRC, \
			DST_ROWS,DST_COLS,(SRC_COLS>>XF_BITSHIFT(NPC)),(DST_COLS>>XF_BITSHIFT(NPC))> \
			(_in_mat,_out_mat, height, width, out_height, out_width);
		}
	}
	else if (_interpolation_type == XF_INTERPOLATION_BILINEAR)
	{
		if ((SRC_ROWS < DST_ROWS) || (SRC_COLS < DST_COLS)){
			xFResizeBilinearUpscale<SRC_ROWS,SRC_COLS,DEPTH,NPC,WORDWIDTH_SRC,   \
			DST_ROWS,DST_COLS,(SRC_COLS>>XF_BITSHIFT(NPC)),(DST_COLS>>XF_BITSHIFT(NPC))> \
			(_in_mat,_out_mat, height, width, out_height, out_width);
		}
		else if ((SRC_ROWS > DST_ROWS) || (SRC_COLS > DST_COLS)){
			xFResizeBilinearDownScale<SRC_ROWS,SRC_COLS,DEPTH,NPC,WORDWIDTH_SRC, \
			DST_ROWS,DST_COLS,(SRC_COLS>>XF_BITSHIFT(NPC)),(DST_COLS>>XF_BITSHIFT(NPC))> \
			(_in_mat,_out_mat, height, width, out_height, out_width);
		}
	}
	else if (_interpolation_type == XF_INTERPOLATION_AREA)
	{
		if ((SRC_ROWS < DST_ROWS) || (SRC_COLS < DST_COLS)){
			xFResizeAreaUpScale<SRC_ROWS,SRC_COLS,DEPTH,NPC,WORDWIDTH_SRC,   \
			DST_ROWS,DST_COLS,(SRC_COLS>>XF_BITSHIFT(NPC)),(DST_COLS>>XF_BITSHIFT(NPC))> \
			(_in_mat,_out_mat, height, width, out_height, out_width);
		}
		else if ((SRC_ROWS > DST_ROWS) || (SRC_COLS > DST_COLS)){
			xFResizeAreaDownScale<SRC_ROWS,SRC_COLS,DEPTH,NPC,WORDWIDTH_SRC, \
			DST_ROWS,DST_COLS,(SRC_COLS>>XF_BITSHIFT(NPC)),(DST_COLS>>XF_BITSHIFT(NPC))> \
			(_in_mat,_out_mat, height, width, out_height, out_width);
		}
	}
}



#pragma SDS data mem_attribute("_src.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_dst.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data access_pattern("_src.data":SEQUENTIAL, "_dst.data":SEQUENTIAL)
//#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("_dst.data":AXIDMA_SIMPLE)
#pragma SDS data copy("_src.data"[0:"_src.size"], "_dst.data"[0:"_dst.size"])
template<int INTERPOLATION_TYPE, int TYPE, int SRC_ROWS, int SRC_COLS, int DST_ROWS, int DST_COLS, int NPC> 
void resize (xf::Mat<TYPE, SRC_ROWS, SRC_COLS, NPC> & _src, xf::Mat<TYPE, DST_ROWS, DST_COLS, NPC> & _dst)
{
#pragma HLS INLINE OFF
#pragma HLS DATAFLOW
	unsigned short input_height = _src.rows;
	unsigned short input_width = _src.cols;
	unsigned short output_height = _dst.rows;
	unsigned short output_width = _dst.cols;
			
	hls::stream< XF_TNAME(TYPE,NPC) > in_image;
	hls::stream< XF_TNAME(TYPE,NPC) > out_image;

	for(int i=0;i<input_height;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=SRC_ROWS
		for(int j=0;j<input_width>>XF_BITSHIFT(NPC);j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=SRC_COLS
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_FLATTEN OFF
			in_image.write( *(_src.data + i*(input_width>>XF_BITSHIFT(NPC)) + j) );
		}
	}
	
	xFresize< SRC_ROWS, SRC_COLS, TYPE, NPC, XF_WORDWIDTH(TYPE,NPC), DST_ROWS, DST_COLS>(in_image, out_image, INTERPOLATION_TYPE, input_height, input_width, output_height, output_width);
		
	for(int i=0;i<output_height;i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=1 max=DST_ROWS
		for(int j=0;j<output_width>>XF_BITSHIFT(NPC);j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=1 max=DST_COLS
#pragma HLS PIPELINE II=1
#pragma HLS LOOP_FLATTEN OFF
			*(_dst.data + i*(output_width>>XF_BITSHIFT(NPC)) + j) = out_image.read();
		}
	}
	return;
}
}
#endif
