#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"

namespace xf{



#pragma SDS data access_pattern("_src.data":SEQUENTIAL)
#pragma SDS data copy("_src.data"[0:"_src.size"])
#pragma SDS data access_pattern("_dst.data":SEQUENTIAL)
#pragma SDS data copy("_dst.data"[0:"_dst.size"])

template<int MAXDELAY,int SRC_T, int ROWS, int COLS,int NPC>
void delayMat(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src, xf::Mat<SRC_T, ROWS, COLS, NPC> & _dst)
{
#pragma HLS inline off

#pragma HLS dataflow

	hls::stream<XF_TNAME(SRC_T,NPC)>src;
	hls::stream< XF_TNAME(SRC_T,NPC)> dst;

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

#pragma HLS stream depth=MAXDELAY variable=src

	for(int i=0; i<_dst.rows;i++)
	{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=ROWS
		for(int j=0; j<(_dst.cols)>>(XF_BITSHIFT(NPC));j++)
		{
	#pragma HLS LOOP_TRIPCOUNT min=1 max=COLS/NPC
			#pragma HLS PIPELINE
			#pragma HLS loop_flatten off
			*(_dst.data + i*(_dst.cols>>(XF_BITSHIFT(NPC))) +j) = src.read();

		}
	}
}
}
