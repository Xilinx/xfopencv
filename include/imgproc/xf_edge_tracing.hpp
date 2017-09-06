

#ifndef _XF_EDGE_TRACING_HPP_
#define _XF_EDGE_TRACING_HPP_

#ifndef __cplusplus
#error C++ is needed to use this file!
#endif

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include <ap_int.h>
#include <string.h>

#define BRAMS_SETS			36
#define BRAM_DEPTH			512
#define INTRA_ITERATIONS	8
#define INTER_ITERATIONS	2
#define SLICES				4
#define PIXELS				34
#define PACK_PIXEL_BITS		128

#define	MIN(x, y)			(((x) < (y)) ? (x) : (y))
#define PIXEL_PROCESS_BITS	68
#define BRAMS_TOTAL			BRAMS_SETS+2

namespace xf{

static void applyEqn(ap_uint<2>& x0, ap_uint<2>& x1, ap_uint<2>& x2, ap_uint<2>& x3,
		ap_uint<2>& a, ap_uint<2>& x4, ap_uint<2>& x5, ap_uint<2>& x6,
		ap_uint<2>& x7) {
#pragma HLS inline

	//# Apply equations
	bool a0 = a.range(1, 1);
	bool a1 = a.range(0, 0);

	//# if center pixel is weak
	CASE_1: if (a0 == false && a1 == true) {
		//# if any of the surrounding pixel is strong, center pixel changes to strong. Otherwise remains weak
		a1 |= (x0.range(1, 1) | x1.range(1, 1) | x2.range(1, 1) | x3.range(1, 1)
				| x4.range(1, 1) | x5.range(1, 1) | x6.range(1, 1)
				| x7.range(1, 1));
	}
	//# if center pixel is strong
	CASE_2: if (a0 == true && a1 == true) {
		x0.range(1, 1) = x0.range(1, 1) | x0.range(0, 0);
		x1.range(1, 1) = x1.range(1, 1) | x1.range(0, 0);
		x2.range(1, 1) = x2.range(1, 1) | x2.range(0, 0);
		x3.range(1, 1) = x3.range(1, 1) | x3.range(0, 0);
		x4.range(1, 1) = x4.range(1, 1) | x4.range(0, 0);
		x5.range(1, 1) = x5.range(1, 1) | x5.range(0, 0);
		x6.range(1, 1) = x6.range(1, 1) | x6.range(0, 0);
		x7.range(1, 1) = x7.range(1, 1) | x7.range(0, 0);
	}
	//# Center pixel update
	a.range(1, 1) = a0;
	a.range(0, 0) = a1;
}

template<int n>
void PixelProcessNew(ap_uint<PIXEL_PROCESS_BITS> k1,
		ap_uint<PIXEL_PROCESS_BITS> k2, ap_uint<PIXEL_PROCESS_BITS> k3,
		ap_uint<PIXEL_PROCESS_BITS>& l1, ap_uint<PIXEL_PROCESS_BITS>& l2,
		ap_uint<PIXEL_PROCESS_BITS>& l3) {
#pragma HLS inline off
	ap_uint<2> x1[PIXELS], x2[PIXELS], x3[PIXELS];
	ap_uint<2> y1[PIXELS], y2[PIXELS], y3[PIXELS];
	ap_uint<2> z1[PIXELS], z2[PIXELS], z3[PIXELS];

	for (int i = 0, j = 0; i < PIXEL_PROCESS_BITS; i += 2, j++) {
#pragma HLS unroll
		x1[j] = k1.range(i + 1, i);
		x2[j] = k2.range(i + 1, i);
		x3[j] = k3.range(i + 1, i);
	}

	PL_1: for (int i = 1; i < PIXELS - 1; i += 3) {
#pragma HLS unroll
		applyEqn(x1[i - 1], x1[i], x1[i + 1], x2[i - 1], x2[i], x2[i + 1],
				x3[i - 1], x3[i], x3[i + 1]);
	}

	for (int i = 0; i < PIXELS; i++) {
#pragma HLS unroll
		y1[i] = x1[i];
		y2[i] = x2[i];
		y3[i] = x3[i];
	}

	PL_2: for (int i = 2; i < PIXELS; i += 3) {
#pragma HLS unroll
		applyEqn(y1[i - 1], y1[i], y1[i + 1], y2[i - 1], y2[i], y2[i + 1],
				y3[i - 1], y3[i], y3[i + 1]);
	}

	for (int i = 0; i < PIXELS; i++) {
#pragma HLS unroll
		z1[i] = y1[i];
		z2[i] = y2[i];
		z3[i] = y3[i];
	}

	PL_3: for (int i = 3; i < PIXELS - 1; i += 3) {
#pragma HLS unroll
		applyEqn(z1[i - 1], z1[i], z1[i + 1], z2[i - 1], z2[i], z2[i + 1],
				z3[i - 1], z3[i], z3[i + 1]);
	}

	for (int i = 0, j = 0; i < PIXEL_PROCESS_BITS; i += 2, j++) {
		l1.range(i + 1, i) = z1[j];
		l2.range(i + 1, i) = z2[j];
		l3.range(i + 1, i) = z3[j];
	}
}

template<int BRAMS, int DEPTH>
void TopDown(ap_uint<64> iBuff[BRAMS][DEPTH], uint16_t width) {

	ap_uint<64> arr1[BRAMS_TOTAL], arr2[BRAMS_TOTAL], arr4[BRAMS_TOTAL];
#pragma HLS array_partition variable=arr1 complete
#pragma HLS array_partition variable=arr2 complete
#pragma HLS array_partition variable=arr4 complete

	ap_uint<4> arr3[BRAMS_TOTAL], arr5[BRAMS_TOTAL];
#pragma HLS array_partition variable=arr3 complete

	for (int j = 0; j < 3; j++) {
		RD_INIT: for (int i = 0; i < BRAMS_TOTAL; i++) {
#pragma HLS unroll
			arr1[i] = iBuff[i][0];
			arr3[i] = 0;
		}

		ap_uint<PIXEL_PROCESS_BITS> k1[15], k2[15], k3[15];
		ap_uint<PIXEL_PROCESS_BITS> l1[15], l2[15], l3[15];

		//# Elements per row
		ELEMENTS_P_ROW: for (int el = 1; el < (width >> 2); el++) {

#pragma HLS pipeline
			RD: for (int i = 0; i < BRAMS_TOTAL; i++) {
#pragma HLS unroll
				arr2[i] = arr1[i];
				arr1[i] = iBuff[i][el];
				arr3[i] = arr1[i].range(3, 0);
			}

			RL11: for (int i = 1, k = 0; i < BRAMS_SETS - 1; i += 3, k++) {
#pragma HLS unroll
				k1[k].range(63, 0) = arr2[i + j - 1];
				k1[k].range(PIXEL_PROCESS_BITS - 1, 64) = arr1[i + j - 1].range(
						3, 0);
				k2[k].range(63, 0) = arr2[i + j];
				k2[k].range(PIXEL_PROCESS_BITS - 1, 64) = arr1[i + j].range(3,
						0);
				k3[k].range(63, 0) = arr2[i + j + 1];
				k3[k].range(PIXEL_PROCESS_BITS - 1, 64) = arr1[i + j + 1].range(
						3, 0);
			}

			for (int i = 0; i < BRAMS_TOTAL; i++) {
#pragma HLS unroll
				arr4[i] = arr2[i];
			}
#if 1
			RL12: for (int k = 0; k < 12; k++) {
#pragma HLS unroll
				PixelProcessNew<1>(k1[k], k2[k], k3[k], l1[k], l2[k], l3[k]);
			}
#else
			PixelProcessNew<1>(k1[0], k2[0], k3[0], l1[0], l2[0], l3[0]);
			PixelProcessNew<2>(k1[1], k2[1], k3[1], l1[1], l2[1], l3[1]);
			PixelProcessNew<3>(k1[2], k2[2], k3[2], l1[2], l2[2], l3[2]);

			PixelProcessNew<4>(k1[3], k2[3], k3[3], l1[3], l2[3], l3[3]);
			PixelProcessNew<5>(k1[4], k2[4], k3[4], l1[4], l2[4], l3[4]);
			PixelProcessNew<6>(k1[5], k2[5], k3[5], l1[5], l2[5], l3[5]);

			PixelProcessNew<7>(k1[6], k2[6], k3[6], l1[6], l2[6], l3[6]);
			PixelProcessNew<8>(k1[7], k2[7], k3[7], l1[7], l2[7], l3[7]);
			PixelProcessNew<9>(k1[8], k2[8], k3[8], l1[8], l2[8], l3[8]);

			PixelProcessNew<10>(k1[9], k2[9], k3[9], l1[9], l2[9], l3[9]);
			PixelProcessNew<11>(k1[10], k2[10], k3[10], l1[10], l2[10], l3[10]);
			PixelProcessNew<12>(k1[11], k2[11], k3[11], l1[11], l2[11], l3[11]);

			PixelProcessNew<13>(k1[12], k2[12], k3[12], l1[12], l2[12], l3[12]);
			PixelProcessNew<14>(k1[13], k2[13], k3[13], l1[13], l2[13], l3[13]);
			PixelProcessNew<15>(k1[14], k2[14], k3[14], l1[14], l2[14], l3[14]);

#endif

			RL13: for (int i = 1, k = 0; i < BRAMS_SETS - 1; i += 3, k++) {
#pragma HLS unroll
				arr4[i + j - 1] = l1[k].range(63, 0);
				arr3[i + j - 1] = l1[k].range(PIXEL_PROCESS_BITS - 1, 64);
				arr4[i + j] = l2[k].range(63, 0);
				arr3[i + j] = l2[k].range(PIXEL_PROCESS_BITS - 1, 64);
				arr4[i + j + 1] = l3[k].range(63, 0);
				arr3[i + j + 1] = l3[k].range(PIXEL_PROCESS_BITS - 1, 64);
			}

			for (int i = 0; i < BRAMS_TOTAL; i++) {
#pragma HLS unroll
				//# Accurate
				//					arr1[i].range(3,0) = arr3[i];
				if (el == 1) {
					arr5[i] = arr3[i];
				} else {
					arr4[i].range(3, 0) = arr5[i];
					arr5[i] = arr3[i];
				}
			}

			WR: for (int i = 0; i < BRAMS_TOTAL; i++) {
#pragma HLS unroll
				iBuff[i][el - 1] = arr4[i];
			}
		}
	}
}

/**
 * xfEdgeTracing : Connects edge
 */
static void xfEdgeTracing(unsigned long long* _dst_mat, unsigned long long *nms_in,
		uint16_t height, uint16_t width) {

	//# BRAMs to store 280 rows of NMS output
	ap_uint<64> iBuff[1 + BRAMS_SETS + 1][BRAM_DEPTH];
#pragma HLS array_partition variable=iBuff dim=1
#pragma HLS resource variable=iBuff core=RAM_S2P_BRAM

	//# I/P & O/P Registers
	ap_uint<64> iReg[1];
	ap_uint<64> oReg[1];

	int offarray[4];
	int lBarray[4];

	//# Inter Iterations
	INTER_ITERATION_LOOP: for (int inter_i = 0; inter_i < INTER_ITERATIONS;
			inter_i++) {

		//# Loop for Reading chunks of NMS output
		unsigned short offset = 0;
		unsigned short lBound = 0;
		SLICE_LOOP: for (int slice = 0; slice < SLICES; slice++) {
			int row = slice * (height >> 2);			// 4-slices
			if (inter_i == 0) {
				offset = row * (width << 1) / 64;
				offarray[slice] = offset;
				lBound = (width << 1) / 64* MIN((height>>2)+18, height-row);
				lBarray[slice] = lBound;
			} else {
				offset =
						((offarray[3 - slice] - height) < 0) ?
								0 : (offarray[3 - slice] - height);
				lBound = lBarray[slice];
			}

			ap_uint<10> idx1 = 0, dep = 0;
			ap_uint<6> idx2 = 1;
			bool rbound = false, lbound = false;

			Read_N_Arrange: for (unsigned short i = 0; i < lBound; i++) {
#pragma HLS loop_tripcount min=16200 max=16800
#pragma HLS pipeline
				iReg[0] = *(nms_in + offset + i);

				if (idx1 == 60) {
					idx1 = 0;
					idx2++;
				}
				if (idx2 == BRAMS_SETS) {
					lbound = true;
				} else {
					lbound = false;
				}
				if (idx2 == BRAMS_SETS + 1) {
					idx2 = 1;
					dep += 60;
					rbound = true;
				} else {
					rbound = false;
				}

				ap_uint<11> index = idx1 + dep;
				iBuff[idx2][index] = iReg[0];
#if 0
				if(rbound)
					iBuff[46][index-60] = iReg[0];
				if(lbound)
					iBuff[0][index+60] = iReg[0];
#endif
				idx1++;
			}

#if 1
			//# Intra Iterations
			INTRA_ITERATION_LOOP: for (int intra_i = 0;
					intra_i < INTRA_ITERATIONS; intra_i++) {
				TopDown<BRAMS_SETS + 2, BRAM_DEPTH>(iBuff, width);
			}

#endif

			idx1 = 0;
			idx2 = 1;
			dep = 0;
			Write: for (unsigned short i = 0; i < lBound; i++) {
#pragma HLS loop_tripcount min=16200 max=16800
#pragma HLS pipeline
				if (idx1 == 60) {
					idx1 = 0;
					idx2++;
				}
				if (idx2 == BRAMS_SETS + 1) {
					idx2 = 1;
					dep += 60;
				}

				oReg[0] = iBuff[idx2][idx1 + dep];
				*(nms_in + offset + i) = oReg[0];

				idx1++;
			}
		}

	}

	ap_uint<64> oBuff[BRAM_DEPTH], oRegF[1];
	//# Write the final output as 8-bit / pixel
	FIN_WR_LOOP: for (int ii = 0; ii < height >> 3; ii++) {
		memcpy(oBuff, nms_in + ii * (width >> 2), width << 1);
		ap_uint<3> id = 0;
		ap_uint<9> pixel = 0;
		WR_FIN_PIPE: for (int j = 0, bit = 0; j < width; j++, bit += 16) {
#pragma HLS pipeline
			if (id == 4) {
				id = 0;
				pixel++;
				bit = 0;
			}
			for (int k = 0, l = 0; k < 16; k += 2, l += 8) {
				ap_uint<2> pix = oBuff[pixel].range(bit + k + 1, bit + k);
				if (pix == 3)
					oRegF[0].range(l + 7, l) = 255;
				else
					oRegF[0].range(l + 7, l) = 0;
			}
			id++;
			*(_dst_mat + ii * width + j) = oRegF[0];
		}
	}

}

#pragma SDS data access_pattern("_src.data":RANDOM, "_dst.data":RANDOM)
#pragma SDS data zero_copy("_src.data"[0:"_src.size"], "_dst.data"[0:"_dst.size"])

template<int SRC_T, int DST_T, int ROWS, int COLS,int NPC_SRC,int NPC_DST>
void EdgeTracing(xf::Mat<SRC_T, ROWS, COLS, NPC_SRC> & _src,xf::Mat<DST_T, ROWS, COLS, NPC_DST> & _dst)
{
	xfEdgeTracing((unsigned long long *)_dst.data,(unsigned long long *)_src.data,_dst.rows,_dst.cols);
}

}
#endif

