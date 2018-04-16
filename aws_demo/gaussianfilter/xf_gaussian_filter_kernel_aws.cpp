//Includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "common/xf_common.h"

#include "xf_gaussian_filter_config.h"

#include "imgproc/xf_gaussian_filter.hpp"


extern "C" void xf_gaussian_filter(unsigned int *a, unsigned int *c, int rows, int cols, float sigma);


    void xf_gaussian_filter(unsigned int *a, unsigned int *c, int rows, int cols, float sigma)
    {
      #pragma HLS INTERFACE m_axi     port=a  offset=slave bundle=gmem
      #pragma HLS INTERFACE m_axi     port=c  offset=slave bundle=gmem
      #pragma HLS INTERFACE s_axilite port=a               bundle=control
      #pragma HLS INTERFACE s_axilite port=c               bundle=control
      #pragma HLS INTERFACE s_axilite port=rows            bundle=control
      #pragma HLS INTERFACE s_axilite port=cols            bundle=control
      #pragma HLS INTERFACE s_axilite port=sigma           bundle=control
      #pragma HLS INTERFACE s_axilite port=return          bundle=control

      #pragma HLS inline region

      int img_size = rows * cols;

      xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> img_a(rows, cols);
      xf::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> img_c(rows, cols);

      #pragma HLS stream variable=img_a.data depth=1
      #pragma HLS stream variable=img_c.data depth=1

      unsigned int *dst_a = (unsigned int*)img_a.data;

      read_data_a: for (int i = 0 ; i < img_size/4; i++)
        {
          #pragma HLS LOOP_TRIPCOUNT min=1 max=65536

          unsigned int ta = a[i];

          dst_a[i] = ta;
        }

      xf::GaussianBlur<FILTER_WIDTH, XF_BORDER_CONSTANT, XF_8UC1, HEIGHT, WIDTH, NPC1>(img_a, img_c, sigma);

      unsigned int *src = (unsigned int *)img_c.data;

      write_data_c: for (int i = 0 ; i < img_size/4; i++)
        {
          #pragma HLS LOOP_TRIPCOUNT min=1 max=65536

          c[i] = src[i];
        }
    }


