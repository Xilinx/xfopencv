#ifndef _XF_Duplicate_HPP_
#define _XF_Duplicate_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

namespace xf {
template<int ROWS, int COLS, int DEPTH, int NPC, int WORDWIDTH>
void xFDuplicate(hls::stream< XF_SNAME(WORDWIDTH) > &_src_mat,
				 hls::stream< XF_SNAME(WORDWIDTH) > &_dst1_mat,
				 hls::stream< XF_SNAME(WORDWIDTH) > &_dst2_mat, uint16_t img_height, uint16_t img_width)
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
#pragma HLS LOOP_TRIPCOUNT min=240 max=240
#pragma HLS pipeline
			XF_SNAME(WORDWIDTH) tmp_src;
			tmp_src = _src_mat.read();
			_dst1_mat.write(tmp_src);
			_dst2_mat.write(tmp_src);
		}
	}
}



#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data access_pattern("_dst1.data":SEQUENTIAL)
#pragma SDS data copy("_dst1.data"[0:"_dst1.size"])
#pragma SDS data access_pattern("_dst2.data":SEQUENTIAL)
#pragma SDS data copy("_dst2.data"[0:"_dst2.size"])

template<int SRC_T, int ROWS, int COLS,int NPC>
void duplicateMat(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst1,xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst2)
{
#pragma HLS inline off

#pragma HLS dataflow

	hls::stream<XF_TNAME(SRC_T,NPC)>src;
	hls::stream< XF_TNAME(SRC_T,NPC)> dst;
	hls::stream< XF_TNAME(SRC_T,NPC)> dst1;

	/********************************************************/

	Read_yuyv_Loop:
	for(int i=0; i<_src.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_src.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			src.write( *(_src.data + i*(_src.cols>>(XF_BITSHIFT(NPC))) +j) );
		}
	}

	xFDuplicate< ROWS, COLS, XF_DEPTH(SRC_T,NPC),NPC,XF_WORDWIDTH(SRC_T,NPC)>(src,dst,dst1, _src.rows,_src.cols);

	for(int i=0; i<_dst1.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst1.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_dst1.data + i*(_dst1.cols>>(XF_BITSHIFT(NPC))) +j) = dst.read();
			*(_dst2.data + i*(_dst2.cols>>(XF_BITSHIFT(NPC))) +j) = dst1.read();

		}
	}
}
}
#endif
