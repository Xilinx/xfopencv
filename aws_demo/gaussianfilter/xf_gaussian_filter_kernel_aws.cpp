//Includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hls_stream.h"

#include "common/xf_common.h"

#include "xf_gaussian_filter_config.h"

#include "imgproc/xf_gaussian_filter.hpp"
#include "imgproc/xf_resize.hpp"

#define SRC_T       XF_8UC1

#define INTERPOLATION_TYPE XF_INTERPOLATION_NN

extern "C" void xf_gaussian_filter(XF_TNAME(SRC_T,NPC1) *ai, XF_TNAME(SRC_T,NPC1) *bo, int rows, int cols, float sigma, int o_rows, int o_cols);


    void xf_gaussian_filter(XF_TNAME(SRC_T,NPC1) *ai, XF_TNAME(SRC_T,NPC1) *bo, int rows, int cols, float sigma, int o_rows, int o_cols)
    {
      #pragma HLS INTERFACE m_axi     port=ai  offset=slave bundle=gmem
      #pragma HLS INTERFACE m_axi     port=bo  offset=slave bundle=gmem

      #pragma HLS INTERFACE s_axilite port=ai               bundle=control
      #pragma HLS INTERFACE s_axilite port=bo               bundle=control
      
      #pragma HLS INTERFACE s_axilite port=rows             bundle=control
      #pragma HLS INTERFACE s_axilite port=cols             bundle=control
      #pragma HLS INTERFACE s_axilite port=sigma            bundle=control

      #pragma HLS INTERFACE s_axilite port=o_rows           bundle=control
      #pragma HLS INTERFACE s_axilite port=o_cols           bundle=control

      #pragma HLS INTERFACE s_axilite port=return           bundle=control

      #pragma HLS inline off
      #pragma HLS dataflow

      const int pROWS = HEIGHT;
      const int pCOLS = WIDTH;
      const int pNPC1 = NPC1;

	    hls::stream<XF_TNAME(SRC_T,NPC1)> src;
	    hls::stream<XF_TNAME(SRC_T,NPC1)> flt;
	    hls::stream<XF_TNAME(SRC_T,NPC1)> dst;

	    /********************************************************/

	    Read_yuyv_Loop:
	    for(int i=0; i < rows; i++)
	      {
	        #pragma HLS LOOP_TRIPCOUNT min=1 max=pROWS

		      for(int j=0; j < (cols)>>(XF_BITSHIFT(NPC1));j++)
		        {
	            #pragma HLS LOOP_TRIPCOUNT min=1 max=pCOLS/pNPC1
			        #pragma HLS PIPELINE
			        #pragma HLS loop_flatten off

			        src.write( *(ai + i*(cols>>(XF_BITSHIFT(NPC1))) +j) );
		        }
	      }

	    xf::xFGaussianFilter< HEIGHT, WIDTH, XF_DEPTH(SRC_T, NPC1), NPC1, XF_WORDWIDTH(SRC_T,NPC1)>(src, flt, FILTER_WIDTH, XF_BORDER_CONSTANT, rows, cols,sigma);


                                                                  //setup same maximum output image size as for source image

	    xf::xFresize< HEIGHT, WIDTH, SRC_T, NPC1, XF_WORDWIDTH(SRC_T,NPC1), HEIGHT/2, WIDTH/2>(flt, dst, INTERPOLATION_TYPE, rows, cols, o_rows, o_cols);
		
      for(int i=0;i<o_rows;i++)
	      {
          #pragma HLS LOOP_TRIPCOUNT min=1 max=pROWS
		  
          for(int j=0;j < o_cols>>XF_BITSHIFT(NPC1); j++)
		        {
              #pragma HLS LOOP_TRIPCOUNT min=1 max=pCOLS
              #pragma HLS PIPELINE II=1
              #pragma HLS LOOP_FLATTEN OFF
			
              *(bo + i*(o_cols>>XF_BITSHIFT(NPC1)) + j) = dst.read();
		        }
	      }

    }


