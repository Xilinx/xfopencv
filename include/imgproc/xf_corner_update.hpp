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
#ifndef __XF_CORNER_UPDATE_HPP__
#define __XF_CORNER_UPDATE_HPP__
#include "ap_int.h"
#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
namespace xf
{	
#pragma SDS data mem_attribute(list_fix:NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute(list:NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("flow_vectors.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data access_pattern(list_fix:RANDOM)
#pragma SDS data access_pattern(list:RANDOM)
#pragma SDS data access_pattern("flow_vectors.data":RANDOM)
#pragma SDS data zero_copy (list_fix[0:nCorners])
#pragma SDS data zero_copy (list[0:nCorners])
#pragma SDS data zero_copy ("flow_vectors.data"[0:"flow_vectors.size"])
template <unsigned int MAXCORNERSNO, unsigned int TYPE, unsigned int ROWS, unsigned int COLS, unsigned int NPC>
void cornerUpdate(ap_uint<64> *list_fix, unsigned int *list, uint32_t nCorners, xf::Mat<TYPE,ROWS,COLS,NPC> &flow_vectors, ap_uint<1> harris_flag)
{
	for (int li=0; li<nCorners; li++)
	{
#pragma HLS PIPELINE II=28 //4 reads on a single bus. 1 read and 1 write to the same bus. Set II = 28 after trail and error
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAXCORNERSNO

		unsigned int list_flag_tmp;
		ap_uint<21> row_num_fix = 0;
		ap_uint<21> col_num_fix = 0;
		if (harris_flag)
		{
			ap_uint<32> point = (ap_uint<32>)list[li];
			ap_uint<64> list_flag_data = (ap_uint<64>)list_fix[li];
			list_flag_tmp =(unsigned int)list_flag_data.range(63,42);
			// row_num_fix and col_num_fix in Q16.0 format, but the format is converted to Q16.5
			if(list_flag_tmp == 0)
			{
				row_num_fix = ((ap_uint<21>)point.range(31,16))<<5;
				col_num_fix = ((ap_uint<21>)point.range(15,0))<<5;
				list_flag_tmp = (unsigned int)list_flag_data.range(63,42);
			}
		}
		else
		{
			ap_uint<64> point = (ap_uint<64>)list_fix[li];

			// row_num_fix and col_num_fix in Q16.5 format
			list_flag_tmp =(unsigned int)point.range(63,42);
			if(list_flag_tmp == 0)
			{
				row_num_fix = (ap_uint<21>)point.range(41,21);
				col_num_fix = (ap_uint<21>)point.range(20,0);
				list_flag_tmp = (unsigned int)point.range(63,42);
			}
		}

		unsigned short row_num = (unsigned short)(row_num_fix >> 5);
		unsigned short col_num = (unsigned short)(col_num_fix >> 5);

		// reading the packed flow vectors
		unsigned int flowuv_tl = flow_vectors.data[row_num*(flow_vectors.cols) + col_num];
		unsigned int flowuv_tr = flow_vectors.data[row_num*(flow_vectors.cols) + (col_num+1)];
		unsigned int flowuv_bl = flow_vectors.data[(row_num+1)*(flow_vectors.cols) + col_num];
		unsigned int flowuv_br = flow_vectors.data[(row_num+1)*(flow_vectors.cols) + (col_num+1)];

		ap_uint<21> rl_fix = ((row_num_fix>>5)<<5);
		ap_uint<21> ct_fix = ((col_num_fix>>5)<<5);
		ap_uint<21> rr_fix = rl_fix + 32;
		ap_uint<21> cb_fix = ct_fix + 32;

		// Q0.5*Q0.5 -> Q0.10 >> 5 -> Q0.5
		unsigned short tl_w = ((rr_fix - row_num_fix)*(cb_fix - col_num_fix))>>5;
		unsigned short tr_w = ((row_num_fix - rl_fix)*(cb_fix - col_num_fix))>>5;
		unsigned short bl_w = ((rr_fix - row_num_fix)*(col_num_fix - ct_fix))>>5;
		unsigned short br_w = ((row_num_fix - rl_fix)*(col_num_fix - ct_fix))>>5;

		// extracting the flow vectors, format A10.6
		short flow_utl = (flowuv_tl>>16);
		short flow_vtl = (0x0000FFFF & flowuv_tl);
		short flow_utr = (flowuv_tr>>16);
		short flow_vtr = (0x0000FFFF & flowuv_tr);
		short flow_ubl = (flowuv_bl>>16);
		short flow_vbl = (0x0000FFFF & flowuv_bl);
		short flow_ubr = (flowuv_br>>16);
		short flow_vbr = (0x0000FFFF & flowuv_br);

		short flow_u = ((tl_w*flow_utl) + (tr_w*flow_utr) + (bl_w*flow_ubl) + (br_w*flow_ubr)) >> 6;
		short flow_v = ((tl_w*flow_vtl) + (tr_w*flow_vtr) + (bl_w*flow_vbl) + (br_w*flow_vbr)) >> 6;

		// add the flow vector to the corner data, rx and ry in Q16.5 format // TODO: check the overflow/underflow
		ap_uint<21> rx = (ap_uint<21>)flow_u + (ap_uint<21>)col_num_fix;
		ap_uint<21> ry = (ap_uint<21>)flow_v + (ap_uint<21>)row_num_fix;

		if((ry < (flow_vectors.rows<<5)) && (ry >= 0) && (rx>= 0) && (rx < (flow_vectors.cols<<5)) && (list_flag_tmp == 0))
		{
			ap_uint<64> outpoint_fix = 0;
			outpoint_fix.range(41,21) = (ap_uint<21>)ry;
			outpoint_fix.range(20,0) = (ap_uint<21>)rx;
			list_fix[li] = (ap_uint<64>)outpoint_fix;

			ap_uint<32> outpoint = 0;
			outpoint.range(31,16) = (ry+16)>>5; // rounding-off by adding 0.5 and flooring
			outpoint.range(15,0) = (rx+16)>>5;
			list[li] = (ap_uint<32>)outpoint;
		}
		else
		{
			list_fix[li].range(63,42) = 1;
//			list_flag[li] = (unsigned int)1;
		}
	}
}
}
#endif
