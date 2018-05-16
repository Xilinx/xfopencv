//Includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hls_stream.h"

#include "common/xf_common.h"

#include "xf_gaussian_filter_config.h"

#include "imgproc/xf_gaussian_filter.hpp"
#include "imgproc/xf_resize.hpp"

extern "C" void xf_gaussian_filter(XF_TNAME(XF_8UC1, NPC1) *img_inp, XF_TNAME(XF_8UC1, NPC1) *img_out, int rows_inp, int cols_inp, float sigma, int rows_out, int cols_out);

void xf_gaussian_filter(XF_TNAME(XF_8UC1, NPC1) *img_inp, XF_TNAME(XF_8UC1, NPC1) *img_out, int rows_inp, int cols_inp, float sigma, int rows_out, int cols_out)
{
  #pragma HLS INTERFACE m_axi     port=img_inp  offset=slave bundle=gmem
  #pragma HLS INTERFACE m_axi     port=img_out  offset=slave bundle=gmem

  #pragma HLS INTERFACE s_axilite port=img_inp               bundle=control
  #pragma HLS INTERFACE s_axilite port=img_out               bundle=control
      
  #pragma HLS INTERFACE s_axilite port=rows_inp              bundle=control
  #pragma HLS INTERFACE s_axilite port=cols_inp              bundle=control
  #pragma HLS INTERFACE s_axilite port=sigma                 bundle=control
                                                            
  #pragma HLS INTERFACE s_axilite port=rows_out              bundle=control
  #pragma HLS INTERFACE s_axilite port=cols_out              bundle=control
                                                            
  #pragma HLS INTERFACE s_axilite port=return                bundle=control

  #pragma HLS dataflow

  const int pROWS_INP = ROWS_INP;
  const int pCOLS_INP = COLS_INP;

  const int pROWS_OUT = ROWS_OUT;
  const int pCOLS_OUT = COLS_OUT;
  
  const int pNPC1 = NPC1;

  xf::Mat<XF_8UC1, ROWS_INP, COLS_INP, NPC1> mi;
  xf::Mat<XF_8UC1, ROWS_INP, COLS_INP, NPC1> mf;

  #pragma HLS stream variable=mi.data depth=pCOLS_INP/pNPC1
  #pragma HLS stream variable=mf.data depth=pCOLS_INP/pNPC1

  xf::Mat<XF_8UC1, ROWS_OUT, COLS_OUT, NPC1> mo;

  #pragma HLS stream variable=mo.data depth=pCOLS_OUT/pNPC1

  mi.rows = rows_inp;  mi.cols = cols_inp;
  mf.rows = rows_inp;  mi.cols = cols_inp;

  mo.rows = rows_out;  mi.cols = cols_out;

  /********************************************************/

  for(int i=0; i < rows_inp; i++)
    {
      #pragma HLS LOOP_TRIPCOUNT min=1 max=pROWS_INP

      for(int j=0; j < (cols_inp >> (XF_BITSHIFT(NPC1))); j++)
        {
          #pragma HLS LOOP_TRIPCOUNT min=1 max=pCOLS_INP/pNPC1
          #pragma HLS PIPELINE
          #pragma HLS loop_flatten off

          *(mi.data + i*(cols_inp >> (XF_BITSHIFT(NPC1))) +j) = *(img_inp + i*(cols_inp >> (XF_BITSHIFT(NPC1))) +j);
        }
    }

  xf::GaussianBlur<FILTER_WIDTH, XF_GAUSSIAN_BORDER, XF_8UC1, ROWS_INP, COLS_INP, NPC1>(mi, mf, sigma);


  xf::resize<XF_RESIZE_INTERPOLATION, XF_8UC1, ROWS_INP, COLS_INP, ROWS_OUT, COLS_OUT, NPC1> (mf, mo);

  for(int i=0; i < rows_out; i++)
    {
      #pragma HLS LOOP_TRIPCOUNT min=1 max=pROWS_OUT

      for(int j=0; j < (cols_out >> (XF_BITSHIFT(NPC1))); j++)
        {
          #pragma HLS LOOP_TRIPCOUNT min=1 max=pCOLS_OUT/pNPC1
          #pragma HLS PIPELINE
          #pragma HLS loop_flatten off

          *(img_out + i*(cols_out >> (XF_BITSHIFT(NPC1))) +j)  = *(mo.data + i*(cols_out >> (XF_BITSHIFT(NPC1))) +j) ;
        }
    }

}


