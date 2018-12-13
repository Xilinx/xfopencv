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
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#ifndef _XF_MEAN_STDDEV_HPP_
#define _XF_MEAN_STDDEV_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#define POW32 2147483648

#include "hls_stream.h"
#include "common/xf_common.h"
#include "core/xf_math.h"

namespace xf{

template<int ROWS, int COLS,int PLANES, int DEPTH, int NPC, int WORDWIDTH, int TC>
void xFStddevkernel(hls::stream<XF_SNAME(WORDWIDTH)>&  _src_mat1,unsigned short* _mean,unsigned short*_dst_stddev,uint16_t height,uint16_t width)
{
#pragma HLS inline
	ap_uint<4> j;
	ap_uint<45> tmp_var_vals[(1<<XF_BITSHIFT(NPC))*PLANES];//={0};
	ap_uint<64> var[PLANES];//={0};
	uint32_t tmp_sum_vals[(1<<XF_BITSHIFT(NPC))*PLANES];//={0};
	uint64_t sum[PLANES];//={0};

#pragma HLS ARRAY_PARTITION variable=tmp_var_vals complete dim=1
#pragma HLS ARRAY_PARTITION variable=tmp_sum_vals complete dim=1


	for ( j = 0; j<((1<<XF_BITSHIFT(NPC))*PLANES);j++)
	{
#pragma HLS UNROLL
		tmp_var_vals[j] = 0;
		tmp_sum_vals[j] = 0;
	}
	for(j=0;j<PLANES;j++)
	{
		sum[j]=0;
		var[j]=0;
	}
int p=0;
	ap_uint<13> row,col;
	Row_Loop1:
	for( row = 0; row < height; row++)
	{
#pragma HLS LOOP_TRIPCOUNT min=ROWS max=ROWS

		Col_Loop1:
		for( col = 0; col < (width>>XF_BITSHIFT(NPC)); col++)
		{
#pragma HLS LOOP_TRIPCOUNT min=TC max=TC
#pragma HLS pipeline
#pragma HLS LOOP_FLATTEN OFF

			XF_SNAME(WORDWIDTH) in_buf;
			in_buf = _src_mat1.read();

			Extract1:
			for(uchar_t i = 0,j=0; i < ((8 << (XF_BITSHIFT(NPC)))*PLANES); i+=8,j++)
			{
#pragma HLS DEPENDENCE variable=tmp_var_vals intra false
#pragma HLS unroll

				ap_uint<8> val=0;
				val = in_buf.range(i+7, i);
				tmp_sum_vals[j] = tmp_sum_vals[j] + val;
				unsigned short int  temp=((unsigned short)val * (unsigned short)val);

				tmp_var_vals[j] += temp;
			}
		}
	}

	for ( j = 0; j<(1<<XF_BITSHIFT(NPC));j++) {
		for(int c=0;c<PLANES;c++) {
			#pragma HLS UNROLL
			sum[c] = (sum[c] + tmp_sum_vals[c*(1<<XF_BITSHIFT(NPC))+j]);
		}
		for(int c=0;c<PLANES;c++) {
			#pragma HLS UNROLL
			var[c] =(ap_uint<64>)((ap_uint<64>) var[c] + (ap_uint<64>)tmp_var_vals[c*(1<<XF_BITSHIFT(NPC))+j]);
			}
	}


		for(int c=0;c<PLANES;c++)
		{
#pragma HLS UNROLL
			unsigned int tempmean=0;
			tempmean = (unsigned short)((ap_uint<64>)(256*(ap_uint<64>)sum[c]) / (width * height));
			_mean[c]  = tempmean;

			/* Variance Computation */

			uint32_t temp = (ap_uint<32>)((ap_uint<64>)(65536 * (ap_uint<64>)var[c])/(width * height));

			uint32_t Varstddev = temp - (tempmean*tempmean);

			uint32_t t1 = (uint32_t)((Varstddev >> 16) << 16);

			_dst_stddev[c] = (unsigned short)xf::Sqrt(t1);//StdDev;//(StdDev >> 4);
		}


}

template<int ROWS, int COLS, int PLANES, int DEPTH, int NPC, int WORDWIDTH>
void xFMeanStddev(hls::stream< XF_SNAME(WORDWIDTH) >& _src_mat,unsigned short* _mean,unsigned short* _stddev,uint16_t height,uint16_t width)
{


	assert((DEPTH == XF_8UP)  && "Input image and output image DEPTH should be same");

	assert(((NPC == XF_NPPC1) || (NPC == XF_NPPC8))
			&& " NPC must be XF_NPPC1, XF_NPPC8");

	assert(((height <= ROWS ) && (width <= COLS)) && "ROWS and COLS should be greater than input image");



	xFStddevkernel<ROWS, COLS,PLANES, DEPTH, NPC, WORDWIDTH, (COLS >> XF_BITSHIFT(NPC))>(_src_mat, _mean, _stddev,height,width);


}
#pragma SDS data data_mover("_src.data":AXIDMA_SIMPLE)
#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data access_pattern(_mean:SEQUENTIAL)
#pragma SDS data access_pattern(_stddev:SEQUENTIAL)
#pragma SDS data copy(_mean[0:3])
#pragma SDS data copy(_stddev[0:3])

template<int SRC_T,int ROWS, int COLS,int NPC=1>
void meanStdDev(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src,unsigned short* _mean,unsigned short* _stddev)
{
#pragma HLS inline off
#pragma HLS dataflow

	hls::stream<XF_TNAME(SRC_T, NPC)> _src_stream;

	Read_Loop:
	for(int i=0; i<_src.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			_src_stream.write( *(_src.data + i*(_src.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFMeanStddev<ROWS, COLS, XF_CHANNELS(SRC_T,NPC),XF_DEPTH(SRC_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC)>(_src_stream, _mean, _stddev, _src.rows, _src.cols);
}
}

#endif // _XF_MEAN_STDDEV_HPP_
