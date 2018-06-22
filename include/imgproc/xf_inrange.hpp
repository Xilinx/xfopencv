
#ifndef _XF_INRANGE_HPP_
#define _XF_INRANGE_HPP_

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"


typedef unsigned short  uint16_t;

typedef unsigned int  uint32_t;

namespace xf{

template<int MAXCOLOR>
void apply_threshold(unsigned char low_thresh[MAXCOLOR][3], unsigned char high_thresh[MAXCOLOR][3],ap_uint<8> &outpix,ap_uint<8> &h,ap_uint<8> &s,ap_uint<8> &v)
{
#pragma HLS inline off

	ap_uint<8> tmp_val = 0;


	ap_uint<8> tmp_val1 = 0;


	for(int k=0;k<MAXCOLOR;k++)
	{
		ap_uint<8> t1, t2, t3;
		t1 = 0;
		t2 = 0;
		t3 = 0;

		if((low_thresh[k][0] <= h) && (h <= high_thresh[k][0]))
			t1 = 255;
		if((low_thresh[k][1] <= s) && (s <= high_thresh[k][1]))
			t2 = 255;
		if((low_thresh[k][2] <= v) && (v <= high_thresh[k][2]))
			t3 = 255;

		tmp_val = tmp_val | (t1 & t2 & t3);
	}

	outpix = tmp_val;
}


template<int SRC_T, int DST_T, int ROWS, int COLS, int DEPTH_SRC, int DEPTH_DST, int NPC, int WORDWIDTH_SRC, int WORDWIDTH_DST,int MAXCOLOR>
void xFInRange(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst_mat,
		unsigned char low_thresh[MAXCOLOR][3], unsigned char high_thresh[MAXCOLOR][3], uint16_t img_height, uint16_t img_width)
{

	XF_PTNAME(DEPTH_SRC) in_pix;
	XF_PTNAME(DEPTH_DST) out_pix;
	ap_uint<8> h, s, v;

	for(uint16_t row = 0; row < img_height; row++)
	{
#pragma HLS LOOP_TRIPCOUNT max=ROWS
		for(uint16_t col = 0; col < img_width; col++)
		{
#pragma HLS PIPELINE
#pragma HLS LOOP_TRIPCOUNT max=COLS

			in_pix = _src_mat.data[row*img_width+col];

			h = in_pix.range(7, 0);
			s = in_pix.range(15, 8);
			v = in_pix.range(23, 16);

			apply_threshold<MAXCOLOR>(low_thresh,high_thresh,out_pix,h,s,v);
			_dst_mat.data[row*img_width+col] = (out_pix);
		}
	}
}



#pragma SDS data access_pattern("_src_mat.data":SEQUENTIAL, "_dst_mat.data":SEQUENTIAL)
#pragma SDS data copy("_src_mat.data"[0:"_src_mat.size"], "_dst_mat.data"[0:"_dst_mat.size"])

#pragma SDS data copy(low_thresh[0:9], high_thresh[0:9])

#pragma SDS data mem_attribute("_src_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
#pragma SDS data mem_attribute("_dst_mat.data":NON_CACHEABLE|PHYSICAL_CONTIGUOUS)
template<int SRC_T,int DST_T,int MAXCOLORS, int ROWS, int COLS,int NPC>
void colorthresholding(xf::Mat<SRC_T, ROWS, COLS, NPC> & _src_mat,xf::Mat<DST_T, ROWS, COLS, NPC> & _dst_mat,unsigned char low_thresh[MAXCOLORS*3], unsigned char high_thresh[MAXCOLORS*3])
{

#pragma HLS INLINE OFF
#pragma HLS DATAFLOW

	unsigned char low_th[MAXCOLORS][3], high_th[MAXCOLORS][3];
#pragma HLS ARRAY_PARTITION variable=low_th dim=1 complete
#pragma HLS ARRAY_PARTITION variable=high_th dim=1 complete
	uint16_t j = 0;
		for(uint16_t i = 0; i < (MAXCOLORS); i++)
		{
	#pragma HLS PIPELINE
			low_th[i][0]  = low_thresh[i*j];
			low_th[i][1]  = low_thresh[i*j+1];
			low_th[i][2]  = low_thresh[i*j+2];
			high_th[i][0] = high_thresh[i*j];
			high_th[i][1] = high_thresh[i*j+1];
			high_th[i][2] = high_thresh[i*j+2];
			j = j+3;
		}

	// Define the low and high thresholds
	// Want to grab 3 colors (Yellow, Green, Red) for teh input image
//	unsigned char low_th[3][3] = { { 22, 150, 60 }, // Lower boundary for Yellow
//			{ 38, 150, 60 }, // Lower boundary for Green
//			{ 160, 150, 60 } }; // Lower boundary for Red
//	unsigned char high_th[3][3] = { { 38, 255, 255 }, // Upper boundary for Yellow
//			{ 75, 255, 255 }, // Upper boundary for Green
//			{ 179, 255, 255 } }; // Upper boundary for Red


	uint16_t img_height = _src_mat.rows;
	uint16_t img_width = _src_mat.cols;

	xFInRange<SRC_T, DST_T,  ROWS, COLS, XF_DEPTH(SRC_T,NPC), XF_DEPTH(DST_T,NPC), NPC, XF_WORDWIDTH(SRC_T,NPC),XF_WORDWIDTH(DST_T,NPC),MAXCOLORS>
	(_src_mat, _dst_mat, low_th, high_th, img_height, img_width);
}
}

#endif
