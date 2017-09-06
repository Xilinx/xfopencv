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

#ifndef _XF_ARITHM_HPP_
#define _XF_ARITHM_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif
#include "hls_stream.h"
#include "common/xf_common.h"
/**
 * xFAbsDiff: Computes the absolute difference between two images
 * Inputs: _src1, _src2
 * Output: _dst
 */
namespace xf {
template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,
		int WORDWIDTH_DST, int COLS_TRIP>
void xFAbsDiffKernel(hls::stream< XF_SNAME(WORDWIDTH_SRC)>& _src1, hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src2, hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _dst, uint16_t image_height, uint16_t image_width)
{
	image_width=image_width>>XF_BITSHIFT(NPC);
	ap_uint <13> i,j,k;

	XF_SNAME(WORDWIDTH_SRC) val_src1, val_src2;
	XF_SNAME(WORDWIDTH_DST) val_dst;
	uchar_t result, p, q;

	rowLoop:
	for( i = 0; i < image_height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for( j = 0; j < image_width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline

			val_src1 = (XF_SNAME(WORDWIDTH_SRC)) (_src1.read()); // reading the data from the first stream
			val_src2 = (XF_SNAME(WORDWIDTH_SRC)) (_src2.read());// reading the data from the second stream

			procLoop:
			for( k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
			k += XF_PIXELDEPTH(DEPTH))
			{
#pragma HLS unroll
				p = val_src1.range(k + (XF_PIXELDEPTH(DEPTH)-1), k); // Get bits from certain range of positions.
				q = val_src2.range(k + (XF_PIXELDEPTH(DEPTH)-1), k);// Get bits from certain range of positions.
				result = __ABS(p - q);// performing absolute difference for the input pixels
				val_dst.range(k + (XF_PIXELDEPTH(DEPTH)-1), k) = result;// Set bits in a range of positions.
			}
			_dst.write(val_dst);    // writing data to the output stream
		}
	}
}

/**
 * xFAdd: Adds the pixels of two input XF_8UP or XF_16SP images and generates the
 * 		  resultant image.
 * Inputs: _src1, _src2, _policytype
 * Output: _dst
 */
template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST, int COLS_TRIP>
void xFAddKernel(hls::stream< XF_SNAME(WORDWIDTH_SRC)>& _src1,
hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src2,
hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _dst,
int _policytype,uint16_t image_height,uint16_t image_width)
{
//	image_width=image_width>>XF_BITSHIFT(NPC);
	ap_uint<13> i,j,k;
	XF_SNAME(WORDWIDTH_SRC) val_src1, val_src2;
	XF_SNAME(WORDWIDTH_DST) val_dst;
	XF_PTNAME(DEPTH) result, p ,q;

	rowLoop:
	for(i = 0; i < image_height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for(j = 0; j < image_width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline
			val_src1 = (XF_SNAME(WORDWIDTH_SRC)) (_src1.read()); // reading the data from the first stream
			val_src2 = (XF_SNAME(WORDWIDTH_SRC)) (_src2.read());// reading the data from the second stream

			procLoop:
			for(k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
			k += XF_PIXELDEPTH(DEPTH))
			{
#pragma HLS unroll
				p = val_src1.range(k + (XF_PIXELDEPTH(DEPTH) - 1), k); // Get bits from certain range of positions.
				q = val_src2.range(k + (XF_PIXELDEPTH(DEPTH) - 1), k);// Get bits from certain range of positions.

				// for the input type of 8U
				if(DEPTH == XF_8UP)
				{
					ap_uint<(XF_PIXELDEPTH(DEPTH)+1)> result_temp;
					result_temp = p + q; // perform the addition operation on the input pixels
					if(_policytype == XF_CONVERT_POLICY_SATURATE &&
					result_temp > 255)// handling the overflow
					{
						result_temp = 255;
					}
					result = (XF_PTNAME(DEPTH)) result_temp;
				}

				// for the input type of 16S
				else if(DEPTH == XF_16SP)
				{
					ap_int<(XF_PIXELDEPTH(DEPTH)+1)> result_temp;
					result_temp = p + q; // perform the addition operation on the input pixels
					if(_policytype == XF_CONVERT_POLICY_SATURATE &&
					result_temp > 32767)// handling the overflow
					{
						result_temp = 32767;
					}
					else if(_policytype == XF_CONVERT_POLICY_SATURATE &&
					result_temp < -32768)		// handling the overflow
					{
						result_temp = -32768;
					}
					result = (XF_PTNAME(DEPTH)) result_temp;
				}
				val_dst.range(k + (XF_PIXELDEPTH(DEPTH) - 1), k) = result; // Set bits in a range of positions.
			}
			_dst.write(val_dst);			// writing data to the output stream
		}
	}
}

/**
 * xFSub: Subtracts the pixels of two input XF_8UP or XF_16SP images and generates the
 * 		  resultant image.
 * Inputs: _src1, _src2, _policytype
 * Output: _dst
 */
template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,
		int WORDWIDTH_DST, int COLS_TRIP>
void xFSubKernel(hls::stream< XF_SNAME(WORDWIDTH_SRC)>& _src1,
hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src2,
hls::stream< XF_SNAME(WORDWIDTH_DST) >& _dst,
int _policytype,uint16_t image_height,uint16_t image_width)
{

	ap_uint<13> i,j,k;
	XF_SNAME(WORDWIDTH_SRC) val_src1, val_src2;
	XF_SNAME(WORDWIDTH_DST) val_dst;
	XF_PTNAME(DEPTH) result, p ,q;

	rowLoop:
	for(i = 0; i < image_height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for(j = 0; j < image_width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline
			val_src1 = (XF_SNAME(WORDWIDTH_SRC)) (_src1.read());// reading the data from the first stream
			val_src2 = (XF_SNAME(WORDWIDTH_SRC)) (_src2.read());// reading the data from the second stream

			procLoop:
			for(k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
			k += XF_PIXELDEPTH(DEPTH))
			{
#pragma HLS unroll
				p = val_src1.range(k + (XF_PIXELDEPTH(DEPTH) - 1), k);// Get bits from certain range of positions.
				q = val_src2.range(k + (XF_PIXELDEPTH(DEPTH) - 1), k);// Get bits from certain range of positions.

				// for the input type of 8U
				if(DEPTH == XF_8UP)
				{
					ap_int<(XF_PIXELDEPTH(DEPTH)+1)> result_temp;
					result_temp = p - q;// perform the subtraction operation on the input pixels
					if(_policytype == XF_CONVERT_POLICY_SATURATE &&
					result_temp < 0)// handling the overflow
					{
						result_temp = 0;
					}
					result = (XF_PTNAME(DEPTH)) result_temp;
				}

				// for the input type of 16S
				else if(DEPTH == XF_16SP)
				{
					ap_int<(XF_PIXELDEPTH(DEPTH)+1)> result_temp;
					result_temp = p - q;// perform the subtraction operation on the input pixels
					if(_policytype==XF_CONVERT_POLICY_SATURATE &&
					result_temp > 32767)// handling the overflow
					{
						result_temp = 32767;
					}
					else if(_policytype==XF_CONVERT_POLICY_SATURATE &&
					result_temp < -32768)		// handling the overflow
					{
						result_temp = -32768;
					}
					result = (XF_PTNAME(DEPTH)) result_temp;
				}
				val_dst.range(k + (XF_PIXELDEPTH(DEPTH) - 1), k) = result; // Set bits in a range of positions.
			}
			_dst.write(val_dst);  // writing data to the stream
		}
	}
}

/**
 * xFBitwiseAND: Performs bitwise ���������AND��������� between two XF_8UP images
 * Inputs: _src1, _src2
 * Output: _dst
 */
template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST, int COLS_TRIP>
void xFBitwiseANDKernel(hls::stream< XF_SNAME(WORDWIDTH_SRC)>& _src1, hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src2, hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _dst,uint16_t image_height,uint16_t image_width)
{

	ap_uint <13> i,j,k;
	XF_SNAME(WORDWIDTH_SRC) val_src1, val_src2;
	XF_SNAME(WORDWIDTH_DST) val_dst;
	XF_PTNAME(DEPTH) result, p, q;

	rowLoop:
	for( i = 0; i < image_height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for( j = 0; j < image_width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline

			val_src1 = (XF_SNAME(WORDWIDTH_SRC)) (_src1.read()); // reading the data from the first stream
			val_src2 = (XF_SNAME(WORDWIDTH_SRC)) (_src2.read());// reading the data from the second stream

			procLoop:
			for( k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
			k += XF_PIXELDEPTH(DEPTH))
			{
#pragma HLS unroll
				p = val_src1.range(k + (XF_PIXELDEPTH(DEPTH)-1), k); // Get bits from certain range of positions.
				q = val_src2.range(k + (XF_PIXELDEPTH(DEPTH)-1), k);// Get bits from certain range of positions.
				result = p & q;// performing the bitwiseAND operation
				val_dst.range(k + (XF_PIXELDEPTH(DEPTH)-1), k) = result;// Set bits in a range of positions.
			}
			_dst.write(val_dst);		// writing data to the stream
		}
	}
}

///**
// * xFBitwiseOR: Performs bitwise ���������OR��������� between two XF_8UP images
// * Inputs: _src1, _src2
// * Output: _dst
// */
template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,
		int WORDWIDTH_DST, int COLS_TRIP>
void xFBitwiseORKernel(hls::stream< XF_SNAME(WORDWIDTH_SRC)>& _src1,
hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src2,
hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _dst,uint16_t image_height,uint16_t image_width)
{

	ap_uint<13> i,j,k;
	XF_SNAME(WORDWIDTH_SRC) val_src1, val_src2;
	XF_SNAME(WORDWIDTH_DST) val_dst;
	XF_PTNAME(DEPTH) result, p, q;

	rowLoop:
	for( i = 0; i < image_height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for( j = 0; j < image_width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline
			val_src1 = (XF_SNAME(WORDWIDTH_SRC)) (_src1.read());// reading the data from the first stream
			val_src2 = (XF_SNAME(WORDWIDTH_SRC)) (_src2.read());// reading the data from the second stream

			procLoop:
			for( k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
			k += XF_PIXELDEPTH(DEPTH))
			{
#pragma HLS unroll
				p = val_src1.range(k + (XF_PIXELDEPTH(DEPTH)-1), k);// Get bits from certain range of positions.
				q = val_src2.range(k + (XF_PIXELDEPTH(DEPTH)-1), k);// Get bits from certain range of positions.
				result = p | q;// performing the bitwiseOR operation
				val_dst.range(k + (XF_PIXELDEPTH(DEPTH)-1), k) = result;// Set bits in a range of positions.
			}
			_dst.write(val_dst);  		// write data to the stream
		}
	}
}

/**
 * xFBitwiseNOT: Performs bitwise ���������NOT��������� for a XF_8UP image
 * Inputs: _src
 * Output: _dst
 */
template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST, int COLS_TRIP>
void xFBitwiseNOTKernel(hls::stream< XF_SNAME(WORDWIDTH_SRC)>& _src,
hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _dst,uint16_t image_height,uint16_t image_width)
{

	ap_uint<13> i,j,k;
	XF_SNAME(WORDWIDTH_SRC) val_src;
	XF_SNAME(WORDWIDTH_DST) val_dst;
	XF_PTNAME(DEPTH) result, p;

	rowLoop:
	for( i = 0; i < image_height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for( j = 0; j < image_width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline
			val_src = (XF_SNAME(WORDWIDTH_SRC)) (_src.read()); // reading the data from the stream

			procLoop:
			for( k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
			k += XF_PIXELDEPTH(DEPTH))
			{
#pragma HLS unroll
				p = val_src.range(k + (XF_PIXELDEPTH(DEPTH)-1), k);	// Get bits from certain range of positions.
				result = ~p;// performing the bitwiseNOT operation
				val_dst.range(k + (XF_PIXELDEPTH(DEPTH)-1), k) = result;// Set bits in a range of positions.
			}
			_dst.write(val_dst);			// write data to the stream
		}
	}
}

/**
 * xFBitwiseXOR: Performs bitwise ���������XOR��������� between two XF_8UP images
 * Inputs: _src1, _src2
 * Output: _dst
 */
template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST, int COLS_TRIP>
void xFBitwiseXORKernel(hls::stream< XF_SNAME(WORDWIDTH_SRC)>& _src1,
hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src2,
hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _dst,uint16_t image_height,uint16_t image_width)
{

	ap_uint <13> i,j,k;
	XF_SNAME(WORDWIDTH_SRC) val_src1, val_src2;
	XF_SNAME(WORDWIDTH_DST) val_dst;
	XF_PTNAME(DEPTH) result, p, q;

	rowLoop:
	for( i = 0; i < image_height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for( j = 0; j < image_width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline
			val_src1 = (XF_SNAME(WORDWIDTH_SRC)) (_src1.read());// reading the data from the first stream
			val_src2 = (XF_SNAME(WORDWIDTH_SRC)) (_src2.read());// reading the data from the second stream

			procLoop:
			for( k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
			k += XF_PIXELDEPTH(DEPTH))
			{
#pragma HLS unroll
				p = val_src1.range(k + (XF_PIXELDEPTH(DEPTH)-1), k);// Get bits from certain range of positions.
				q = val_src2.range(k + (XF_PIXELDEPTH(DEPTH)-1), k);// Get bits from certain range of positions.
				result = p ^ q;// performing the bitwise XOR operation
				val_dst.range(k + (XF_PIXELDEPTH(DEPTH)-1), k) = result;// Set bits in a range of positions.
			}
			_dst.write(val_dst);  	  	// write data to the stream
		}
	}
}

/**
 * xFMul : Performs element-wise multiplication between two images and a scalar value
 * Inputs: _src1, _src2, _policytype, _scale_val
 * Output: _dst
 */
template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,
		int WORDWIDTH_DST, int COLS_TRIP>
void xFMulKernel(hls::stream< XF_SNAME(WORDWIDTH_SRC)>& _src1,
hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src2,
hls::stream< XF_SNAME(WORDWIDTH_DST) >& _dst,
int _policytype, float _scale_val,uint16_t image_height,uint16_t image_width)
{

	ap_uint <13> i,j,k;
	XF_SNAME(WORDWIDTH_SRC) val_src1, val_src2;
	XF_SNAME(WORDWIDTH_DST) val_dst;

	XF_PTNAME(DEPTH) result, p ,q;
	int64_t result_temp;
	uint16_t scale_value_8;
	uint32_t scale_value_16;
	if(DEPTH == XF_8UP)
	{
		scale_value_8 = (_scale_val * ( (1 << 15) -1 )); // floating point value taken in fixed point format (Q1.15)
	}
	else if(DEPTH == XF_16SP)
	{
		scale_value_16 = (_scale_val * ( (1 << 24) -1 )); // floating point value taken in fixed point format (Q1.24)
	}

	rowLoop:
	for( i = 0; i < image_height; i++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS
#pragma HLS LOOP_FLATTEN off

		colLoop:
		for( j = 0; j < image_width; j++)
		{
#pragma HLS LOOP_TRIPCOUNT min=COLS_TRIP max=COLS_TRIP
#pragma HLS pipeline

			val_src1 = (XF_SNAME(WORDWIDTH_SRC)) (_src1.read()); // reading the data from the first stream
			val_src2 = (XF_SNAME(WORDWIDTH_SRC)) (_src2.read());// reading the data from the second stream

			procLoop:
			for( k = 0; k < (XF_WORDDEPTH(WORDWIDTH_SRC));
			k += XF_PIXELDEPTH(DEPTH))
			{
#pragma HLS unroll
				p = val_src1.range(k + (XF_PIXELDEPTH(DEPTH) - 1), k); // Get bits from certain range of positions.
				q = val_src2.range(k + (XF_PIXELDEPTH(DEPTH) - 1), k);// Get bits from certain range of positions.

				// for the input type of 8U
				if(DEPTH == XF_8UP)
				{
					result_temp = (scale_value_8 * p * q ) >> 15; // performing pixel-wise multiplication with scale value
					if(_policytype == XF_CONVERT_POLICY_SATURATE &&
					result_temp > 255)// handling the overflow
					{
						result_temp = 255;
					}
					result = (uchar_t) result_temp;
				}

				// for the input type of 16S
				else if(DEPTH == XF_16SP)
				{
					result_temp = (scale_value_16 * p * q ) >> 24; // performing pixel-wise multiplication with scale value
					if(_policytype==XF_CONVERT_POLICY_SATURATE &&
					result_temp > 32767)// handling the overflow
					{
						result_temp = 32767;
					}
					else if(_policytype==XF_CONVERT_POLICY_SATURATE &&
					result_temp < -32768)
					{
						result_temp = -32768;
					}
					result = (int16_t) result_temp;
				}
				val_dst.range(k + (XF_PIXELDEPTH(DEPTH) - 1), k) = result; // Set bits in a range of positions.
			}
			_dst.write(val_dst);		// write data to the stream
		}
	}
}


template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST>
void xFAbsDiff(hls::stream< XF_SNAME(WORDWIDTH_SRC)>& _src1, hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src2, hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _dst, uint16_t image_height, uint16_t image_width)
{

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
	"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert((DEPTH == XF_8UP) && "Depth must be XF_8UP");
	assert(((WORDWIDTH_SRC == XF_8UW) || (WORDWIDTH_SRC == XF_16UW) ||
			(WORDWIDTH_SRC == XF_64UW) || (WORDWIDTH_SRC == XF_128UW)) &&
	"WORDWIDTH_SRC must be XF_8UW, XF_16UW, XF_64UW or XF_128UW ");
	assert(((WORDWIDTH_DST == XF_8UW) || (WORDWIDTH_DST == XF_16UW) ||
			(WORDWIDTH_DST == XF_64UW) || (WORDWIDTH_DST == XF_128UW)) &&
	"WORDWIDTH_DST must be XF_8UW, XF_16UW, XF_64UW or XF_128UW ");
	assert(((image_height <= ROWS ) && (image_width <= COLS)) && "ROWS and COLS should be greater than input image");

	xFAbsDiffKernel<ROWS,COLS,DEPTH,NPC,WORDWIDTH_SRC, WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(_src1,_src2,_dst,image_height,image_width);
}
//#pragma SDS data data_mover("src1.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("src2.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("dst.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("src2.data":SEQUENTIAL)
#pragma SDS data copy("src1.data"[0:"src1.size"])
#pragma SDS data copy("src2.data"[0:"src2.size"])
#pragma SDS data access_pattern("dst.data":SEQUENTIAL)
#pragma SDS data copy("dst.data"[0:"dst.size"])
template<int SRC_T, int ROWS, int COLS, int NPC =1>
void absdiff(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1,xf::Mat<SRC_T, ROWS, COLS, NPC> & src2,xf::Mat<SRC_T, ROWS, COLS, NPC> & dst) {

	hls::stream<XF_TNAME(SRC_T, NPC)> _src1;
	hls::stream<XF_TNAME(SRC_T, NPC)> _src2;
	hls::stream<XF_TNAME(SRC_T, NPC)> _dst;
#pragma HLS inline off
#pragma HLS dataflow

	Read_Loop:
	for(int i=0; i<src1.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src1.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			_src1.write( *(src1.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
			_src2.write( *(src2.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFAbsDiff<ROWS, COLS, XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC)>(_src1,_src2,_dst,src1.rows,src1.cols);

	for(int i=0; i<dst.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(dst.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(dst.data + i*(dst.cols>>(XF_BITSHIFT(NPC))) +j) = _dst.read();

		}
	}
}


template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST>
void xFAdd(hls::stream< XF_SNAME(WORDWIDTH_SRC)> & _src1,hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src2,hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _dst,int _policytype, uint16_t image_height, uint16_t image_width)
{

	image_width = image_width >> XF_BITSHIFT(NPC);
#pragma HLS INLINE
	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8) ) &&
	"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((DEPTH == XF_8UP) || (DEPTH == XF_16SP)) &&
	"Depth must be XF_8UP or XF_16SP");
	assert(((WORDWIDTH_SRC == XF_8UW) || (WORDWIDTH_SRC == XF_16UW) ||
			(WORDWIDTH_SRC == XF_64UW) || (WORDWIDTH_SRC == XF_128UW)) &&
	"WORDWIDTH_SRC must be XF_8UW, XF_16UW, XF_64UW or XF_128UW ");
	assert(((WORDWIDTH_DST == XF_8UW) || (WORDWIDTH_DST == XF_16UW) ||
			(WORDWIDTH_DST == XF_64UW) || (WORDWIDTH_DST == XF_128UW)) &&
	"WORDWIDTH_DST must be XF_8UW, XF_16UW, XF_64UW or  XF_128UW ");
	assert((_policytype == XF_CONVERT_POLICY_SATURATE ||
			_policytype == XF_CONVERT_POLICY_TRUNCATE)
	&& "_policytype must be 'AU_CONVERT_POLICY_SATURATE' or 'AU_CONVERT_POLICY_TRUNCATE'");

	assert(((image_height <= ROWS ) && (image_width <= COLS)) && "ROWS and COLS should be greater than input image");


	xFAddKernel<ROWS,COLS,DEPTH,NPC,WORDWIDTH_SRC, WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(_src1,_src2,_dst,_policytype,image_height,image_width);

}
//#pragma SDS data data_mover("src1.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("src2.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("dst.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("src2.data":SEQUENTIAL)
#pragma SDS data copy("src1.data"[0:"src1.size"])
#pragma SDS data copy("src2.data"[0:"src2.size"])
#pragma SDS data access_pattern("dst.data":SEQUENTIAL)
#pragma SDS data copy("dst.data"[0:"dst.size"])
template<int POLICY_TYPE, int SRC_T, int ROWS, int COLS, int NPC =1>
void add(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1,xf::Mat<SRC_T, ROWS, COLS, NPC> & src2,xf::Mat<SRC_T, ROWS, COLS, NPC> & dst)
{

	hls::stream<XF_TNAME(SRC_T, NPC)> _src1;
	hls::stream<XF_TNAME(SRC_T, NPC)> _src2;
	hls::stream<XF_TNAME(SRC_T, NPC)> _dst;
#pragma HLS inline off
#pragma HLS dataflow

	Read_Loop:
	for(int i=0; i<src1.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src1.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			_src1.write( *(src1.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
			_src2.write( *(src2.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}


	xFAdd <ROWS, COLS, XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC)>(_src1, _src2, _dst,POLICY_TYPE, src1.rows, src1.cols);

	for(int i=0; i<dst.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(dst.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(dst.data + i*(dst.cols>>(XF_BITSHIFT(NPC))) +j) = _dst.read();

		}
	}
}



template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST>
void xFSub(hls::stream< XF_SNAME(WORDWIDTH_SRC)>& _src1, hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src2, hls::stream< XF_SNAME(WORDWIDTH_DST) >& _dst, int _policytype,uint16_t image_height,uint16_t image_width)
{


	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8) ) &&
	"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert(((DEPTH == XF_8UP) || (DEPTH == XF_16SP)) &&
	"Depth must be XF_8UP or XF_16SP");
	assert(((WORDWIDTH_SRC == XF_8UW) || (WORDWIDTH_SRC == XF_16UW) ||
			(WORDWIDTH_SRC == XF_64UW) || (WORDWIDTH_SRC == XF_128UW)) &&
	"WORDWIDTH_SRC must be XF_8UW, XF_16UW, XF_64UW or XF_128UW ");
	assert(((WORDWIDTH_DST == XF_8UW) || (WORDWIDTH_DST == XF_16UW) ||
			(WORDWIDTH_DST == XF_64UW) || (WORDWIDTH_DST == XF_128UW)) &&
	"WORDWIDTH_DST must be XF_8UW, XF_16UW, XF_64UW or  XF_128UW ");
	assert((_policytype == XF_CONVERT_POLICY_SATURATE ||
			_policytype == XF_CONVERT_POLICY_TRUNCATE)
	&& "_policytype must be 'AU_CONVERT_POLICY_SATURATE' or 'AU_CONVERT_POLICY_TRUNCATE'");

	assert(((image_height <= ROWS ) && (image_width <= COLS)) && "ROWS and COLS should be greater than input image");

	image_width = image_width >> XF_BITSHIFT(NPC);

		xFSubKernel<ROWS,COLS,DEPTH,NPC,WORDWIDTH_SRC, WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(_src1,_src2,_dst,_policytype,image_height,image_width);

}
//#pragma SDS data data_mover("src1.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("src2.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("dst.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("src2.data":SEQUENTIAL)
#pragma SDS data copy("src1.data"[0:"src1.size"])
#pragma SDS data copy("src2.data"[0:"src2.size"])
#pragma SDS data access_pattern("dst.data":SEQUENTIAL)
#pragma SDS data copy("dst.data"[0:"dst.size"])

template<int POLICY_TYPE, int SRC_T, int ROWS, int COLS, int NPC =1>
void subtract(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & src2, xf::Mat<SRC_T, ROWS, COLS, NPC> & dst)
{

	hls::stream<XF_TNAME(SRC_T, NPC)> _src1;
	hls::stream<XF_TNAME(SRC_T, NPC)> _src2;
	hls::stream<XF_TNAME(SRC_T, NPC)> _dst;
#pragma HLS inline off
#pragma HLS dataflow

	Read_Loop:
	for(int i=0; i<src1.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src1.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			_src1.write( *(src1.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
			_src2.write( *(src2.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFSub<ROWS, COLS, XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC)>(_src1, _src2, _dst,POLICY_TYPE, src1.rows, src1.cols);



	for(int i=0; i<dst.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(dst.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(dst.data + i*(dst.cols>>(XF_BITSHIFT(NPC))) +j) = _dst.read();

		}
	}

}



template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST>
void xFBitwiseAND(hls::stream< XF_SNAME(WORDWIDTH_SRC)>& _src1,hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src2,hls::stream< XF_SNAME(WORDWIDTH_DST) >& _dst,uint16_t image_height,uint16_t image_width)
{

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
	"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert((DEPTH == XF_8UP) && "Depth must be XF_8UP");
	assert(((WORDWIDTH_SRC == XF_8UW) || (WORDWIDTH_SRC == XF_64UW)) &&
	"WORDWIDTH_SRC must be XF_8UW or XF_64UW ");
	assert(((WORDWIDTH_DST == XF_8UW) || (WORDWIDTH_DST == XF_64UW)) &&
	"WORDWIDTH_DST must be XF_8UW or XF_64UW ");
	assert(((image_height <= ROWS ) && (image_width <= COLS)) && "ROWS and COLS should be greater than input image");
	image_width = image_width >> XF_BITSHIFT(NPC);

	xFBitwiseANDKernel<ROWS,COLS,DEPTH,NPC,WORDWIDTH_SRC, WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(_src1,_src2,_dst,image_height,image_width);
}
//#pragma SDS data data_mover("src1.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("src2.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("dst.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("src2.data":SEQUENTIAL)
#pragma SDS data copy("src1.data"[0:"src1.size"])
#pragma SDS data copy("src2.data"[0:"src2.size"])
#pragma SDS data access_pattern("dst.data":SEQUENTIAL)
#pragma SDS data copy("dst.data"[0:"dst.size"])
template<int SRC_T, int ROWS, int COLS, int NPC = 1>
void bitwise_and(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & src2, xf::Mat<SRC_T, ROWS, COLS, NPC> & dst)
{
	hls::stream<XF_TNAME(SRC_T, NPC)> _src1;
	hls::stream<XF_TNAME(SRC_T, NPC)> _src2;
	hls::stream<XF_TNAME(SRC_T, NPC)> _dst;
#pragma HLS inline off
#pragma HLS dataflow

	Read_Loop:
	for(int i=0; i<src1.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src1.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			_src1.write( *(src1.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
			_src2.write( *(src2.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}


	xFBitwiseAND<ROWS, COLS, XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC)>(_src1, _src2, _dst,src1.rows, src1.cols);

	for(int i=0; i<dst.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(dst.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(dst.data + i*(dst.cols>>(XF_BITSHIFT(NPC))) +j) = _dst.read();

		}
	}
}

template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST>
void xFBitwiseOR(hls::stream< XF_SNAME(WORDWIDTH_SRC)>& _src1,
hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src2,
hls::stream< XF_SNAME(WORDWIDTH_DST) >& _dst,uint16_t image_height,uint16_t image_width)
{

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
	"NPC must be XF_NPPC1 or XF_NPPC8");
	assert((DEPTH == XF_8UP) && "Depth must be XF_8UP");
	assert(((WORDWIDTH_SRC == XF_8UW) || (WORDWIDTH_SRC == XF_64UW)) &&
	"WORDWIDTH_SRC must be XF_8UW, XF_64UW");
	assert(((WORDWIDTH_DST == XF_8UW) || (WORDWIDTH_DST == XF_64UW)) &&
	"WORDWIDTH_DST must be XF_8UW or XF_64UW ");
	assert(((image_height <= ROWS ) && (image_width <= COLS)) && "ROWS and COLS should be greater than input image");
	image_width = image_width >> XF_BITSHIFT(NPC);
	xFBitwiseORKernel<ROWS,COLS,DEPTH,NPC,WORDWIDTH_SRC, WORDWIDTH_DST,
	(COLS>>XF_BITSHIFT(NPC))>(_src1,_src2,_dst,image_height,image_width);
}
//#pragma SDS data data_mover("src1.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("src2.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("dst.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("src2.data":SEQUENTIAL)
#pragma SDS data copy("src1.data"[0:"src1.size"])
#pragma SDS data copy("src2.data"[0:"src2.size"])
#pragma SDS data access_pattern("dst.data":SEQUENTIAL)
#pragma SDS data copy("dst.data"[0:"dst.size"])
template<int SRC_T, int ROWS, int COLS, int NPC = 1>
void bitwise_or(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & src2, xf::Mat<SRC_T, ROWS, COLS, NPC> & dst)
{
	hls::stream<XF_TNAME(SRC_T, NPC)> _src1;
	hls::stream<XF_TNAME(SRC_T, NPC)> _src2;
	hls::stream<XF_TNAME(SRC_T, NPC)> _dst;
#pragma HLS inline off
#pragma HLS dataflow

	Read_Loop:
	for(int i=0; i<src1.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src1.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			_src1.write( *(src1.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
			_src2.write( *(src2.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFBitwiseOR<ROWS, COLS, XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC)>(_src1, _src2,_dst,src1.rows, src1.cols);


	for(int i=0; i<dst.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(dst.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(dst.data + i*(dst.cols>>(XF_BITSHIFT(NPC))) +j) = _dst.read();

		}
	}
}

template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST>
void xFBitwiseNOT(hls::stream< XF_SNAME(WORDWIDTH_SRC)>& _src,hls::stream< XF_SNAME(WORDWIDTH_DST) >& _dst,uint16_t image_height,uint16_t image_width)
{

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
	"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert((DEPTH == XF_8UP) && "Depth must be XF_8UP");
	assert(((WORDWIDTH_SRC == XF_8UW) || (WORDWIDTH_SRC == XF_64UW)) &&
	"WORDWIDTH_SRC must be XF_8UW or XF_64UW ");
	assert(((WORDWIDTH_DST == XF_8UW) || (WORDWIDTH_DST == XF_64UW)) &&
	"WORDWIDTH_DST must be XF_8UW or XF_64UW ");
	assert(((image_height <= ROWS ) && (image_width <= COLS)) && "ROWS and COLS should be greater than input image");
	image_width = image_width >> XF_BITSHIFT(NPC);
	xFBitwiseNOTKernel<ROWS,COLS,DEPTH,NPC,WORDWIDTH_SRC, WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(_src,_dst,image_height,image_width);
}
//#pragma SDS data data_mover("src.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("dst.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("src.data":SEQUENTIAL)
#pragma SDS data copy("src.data"[0:"src.size"])
#pragma SDS data access_pattern("dst.data":SEQUENTIAL)
#pragma SDS data copy("dst.data"[0:"dst.size"])
template<int SRC_T, int ROWS, int COLS, int NPC = 1>
void bitwise_not(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{

		hls::stream<XF_TNAME(SRC_T, NPC)> src1;
		hls::stream<XF_TNAME(SRC_T, NPC)> dst;
#pragma HLS inline off
#pragma HLS dataflow
		Read_Loop:
		for(int i=0; i<_src.rows;i++)
		{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
			for(int j=0; j<(_src.cols)>>(XF_BITSHIFT(NPC));j++)
			{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
				#pragma HLS PIPELINE
				#pragma HLS loop_flatten off
				src1.write( *(_src.data + i*(_src.cols>>(XF_BITSHIFT(NPC))) +j) );
			}
		}



		xFBitwiseNOT<ROWS, COLS, XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC)>(src1,dst,_src.rows, _src.cols);

		for(int i=0; i<_dst.rows;i++)
		{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
			for(int j=0; j<(_dst.cols)>>(XF_BITSHIFT(NPC));j++)
			{
		#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
				#pragma HLS PIPELINE
				#pragma HLS loop_flatten off
				*(_dst.data + i*(_dst.cols>>(XF_BITSHIFT(NPC))) +j) = dst.read();

			}
		}

}


template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST>
void xFBitwiseXOR(hls::stream< XF_SNAME(WORDWIDTH_SRC)>& _src1,
hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src2,
hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _dst,uint16_t image_height,uint16_t image_width)
{

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
	"NPC must be XF_NPPC1 or XF_NPPC8 ");
	assert((DEPTH == XF_8UP) && "Depth must be XF_8UP");
	assert(((WORDWIDTH_SRC == XF_8UW) || (WORDWIDTH_SRC == XF_64UW)) &&
	"WORDWIDTH_SRC must be XF_8UW or XF_64UW ");
	assert(((WORDWIDTH_DST == XF_8UW) || (WORDWIDTH_DST == XF_64UW)) &&
	"WORDWIDTH_DST must be XF_8UW or XF_64UW ");
	assert(((image_height <= ROWS ) && (image_width <= COLS)) && "ROWS and COLS should be greater than input image");
	image_width = image_width >> XF_BITSHIFT(NPC);
	xFBitwiseXORKernel<ROWS,COLS,DEPTH,NPC,WORDWIDTH_SRC, WORDWIDTH_DST,
	(COLS>>XF_BITSHIFT(NPC))>(_src1,_src2,_dst,image_height,image_width);
}
//#pragma SDS data data_mover("src1.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("src2.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("dst.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("src2.data":SEQUENTIAL)
#pragma SDS data copy("src1.data"[0:"src1.size"])
#pragma SDS data copy("src2.data"[0:"src2.size"])
#pragma SDS data access_pattern("dst.data":SEQUENTIAL)
#pragma SDS data copy("dst.data"[0:"dst.size"])
template<int SRC_T, int ROWS, int COLS, int NPC = 1>
void bitwise_xor(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & src2, xf::Mat<SRC_T, ROWS, COLS, NPC> & dst)
{
	hls::stream<XF_TNAME(SRC_T, NPC)> _src1;
	hls::stream<XF_TNAME(SRC_T, NPC)> _src2;
	hls::stream<XF_TNAME(SRC_T, NPC)> _dst;
#pragma HLS inline off
#pragma HLS dataflow

	Read_Loop:
	for(int i=0; i<src1.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(src1.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			_src1.write( *(src1.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
			_src2.write( *(src2.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFBitwiseXOR<ROWS, COLS, XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC)>(_src1, _src2, _dst,src1.rows, src1.cols);

	for(int i=0; i<dst.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(dst.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(dst.data + i*(dst.cols>>(XF_BITSHIFT(NPC))) +j) = _dst.read();

		}
	}

}

template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH_SRC,int WORDWIDTH_DST>
void xFMul(hls::stream< XF_SNAME(WORDWIDTH_SRC)>& _src1,
hls::stream< XF_SNAME(WORDWIDTH_SRC) >& _src2,
hls::stream< XF_SNAME(WORDWIDTH_DST) >& _dst,
int _policytype, float _scale_val,uint16_t image_height,uint16_t image_width)
{



		assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8)) &&
		"NPC must be XF_NPPC1 or XF_NPPC8 ");
		assert(((DEPTH == XF_8UP) || (DEPTH == XF_16SP)) &&
		"Depth must be XF_8UP or XF_16SP");
		assert(((WORDWIDTH_SRC == XF_8UW) || (WORDWIDTH_SRC == XF_16UW) ||
			(WORDWIDTH_SRC == XF_64UW) || (WORDWIDTH_SRC == XF_128UW)) &&
			"WORDWIDTH_SRC must be XF_8UW, XF_16UW, XF_64UW or XF_128UW ");
	assert(((WORDWIDTH_DST == XF_8UW)  || (WORDWIDTH_DST == XF_16UW)  ||
			(WORDWIDTH_DST == XF_64UW) || (WORDWIDTH_DST == XF_128UW)) &&
			"WORDWIDTH_DST must be XF_8UW, XF_16UW, XF_64UW or XF_128UW ");
	assert((_policytype == XF_CONVERT_POLICY_SATURATE ||
			_policytype == XF_CONVERT_POLICY_TRUNCATE)
			&& "_policytype must be 'XF_CONVERT_POLICY_SATURATE' or 'XF_CONVERT_POLICY_TRUNCATE'");
	assert(((_scale_val >= 0) && (_scale_val <= 1)) &&
			"_scale_val must be within the range of 0 to 1");
	assert(((image_height <= ROWS ) && (image_width <= COLS)) && "ROWS and COLS should be greater than input image");

	image_width = image_width >> XF_BITSHIFT(NPC);

	xFMulKernel<ROWS,COLS,DEPTH,NPC,WORDWIDTH_SRC, WORDWIDTH_DST,(COLS>>XF_BITSHIFT(NPC))>(_src1,_src2,_dst,_policytype,_scale_val,image_height,image_width);


}
//#pragma SDS data data_mover("src1.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("src2.data":AXIDMA_SIMPLE)
//#pragma SDS data data_mover("dst.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("src1.data":SEQUENTIAL)
#pragma SDS data access_pattern("src2.data":SEQUENTIAL)
#pragma SDS data copy("src1.data"[0:"src1.size"])
#pragma SDS data copy("src2.data"[0:"src2.size"])
#pragma SDS data access_pattern("dst.data":SEQUENTIAL)
#pragma SDS data copy("dst.data"[0:"dst.size"])
template<int POLICY_TYPE, int SRC_T, int ROWS, int COLS, int NPC = 1>
void multiply(xf::Mat<SRC_T, ROWS, COLS, NPC> & src1, xf::Mat<SRC_T, ROWS, COLS, NPC> & src2, xf::Mat<SRC_T, ROWS, COLS, NPC> & dst,float scale)
{
			hls::stream<XF_TNAME(SRC_T, NPC)> _src1;
			hls::stream<XF_TNAME(SRC_T, NPC)> _src2;
			hls::stream<XF_TNAME(SRC_T, NPC)> _dst;

#pragma HLS inline off
#pragma HLS dataflow

			Read_Loop:
			for(int i=0; i<src1.rows;i++)
			{
			#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
				for(int j=0; j<(src1.cols)>>(XF_BITSHIFT(NPC));j++)
				{
			#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
					#pragma HLS PIPELINE
					#pragma HLS loop_flatten off
					_src1.write( *(src1.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
					_src2.write( *(src2.data + i*(src1.cols>>(XF_BITSHIFT(NPC))) +j) );
				}
			}

			xFMul<ROWS, COLS, XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC), XF_WORDWIDTH(SRC_T,NPC)>(_src1, _src2, _dst,POLICY_TYPE,scale,src1.rows, src1.cols);

			for(int i=0; i<dst.rows;i++)
			{
			#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
				for(int j=0; j<(dst.cols)>>(XF_BITSHIFT(NPC));j++)
				{
			#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
					#pragma HLS PIPELINE
					#pragma HLS loop_flatten off
					*(dst.data + i*(dst.cols>>(XF_BITSHIFT(NPC))) +j) = _dst.read();

				}
			}
}
}//namespace
#endif
