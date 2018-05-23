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

#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "common/xf_sw_utils.h"

#include "xf_gaussian_filter_config.h"

using namespace std;

int main(int argc, char **argv) 
{
  if (argc != 2)
    {
      printf("Usage: <executable> <input image path> \n");
      return -1;
    }

  cv::Mat cv_img_inp, cv_img_out, cv_img_ref;
  cv::Mat diff;

  int rows_out, cols_out;

  cv_img_inp = cv::imread(argv[1], 0); // reading in the color image

  if (!cv_img_inp.data) 
    {
      printf("Failed to load the image ... !!!");
      return -1;
    }

  rows_out = cv_img_inp.rows * SCALE;
  cols_out = cv_img_inp.cols * SCALE;

  cv_img_ref.create(cv_img_inp.rows, cv_img_inp.cols, cv_img_inp.depth()); // create memory for OCV output image

  cv_img_out.create(rows_out, cols_out, cv_img_inp.depth()); // create memory for OCV output image

  float sigma = SIGMA;

  // OpenCV Gaussian filter function
  cv::GaussianBlur(cv_img_inp, cv_img_ref, cvSize(FILTER_WIDTH, FILTER_WIDTH), SIGMA, SIGMA, CV_GAUSSIAN_BORDER);

  cv::resize(cv_img_ref, cv_img_out, cvSize(cv_img_out.cols, cv_img_out.rows), 0, 0, CV_RESIZE_INTERPOLATION );

  imwrite("cv_img_out.jpg", cv_img_out);


  diff.create(cv_img_out.rows, cv_img_out.cols, cv_img_out.depth()); // create memory for diff image


  //=====================================================================//


  xf::Mat<XF_8UC1, ROWS_INP, COLS_INP, NPC1> xf_img_inp(cv_img_inp.rows,cv_img_inp.cols);
  xf::Mat<XF_8UC1, ROWS_OUT, COLS_OUT, NPC1> xf_img_out(cv_img_out.rows,cv_img_out.cols);

  xf_img_inp = xf::imread<XF_8UC1, ROWS_INP, COLS_INP, NPC1>(argv[1], 0);

  gaussian_filter_accel(xf_img_inp, xf_img_out, sigma);


  // Write output image
  xf::imwrite("xf_img_out.jpg",xf_img_out);


  xf::absDiff(cv_img_out, xf_img_out, diff); // Compute absolute difference image

  imwrite("error.png", diff); // Save the difference image for debugging purpose

  // Find minimum and maximum differences.

  #define THRESHOLD 1

  double minval = 256, maxval = 0;
  int cnt = 0;

  for( int i = 0; i < diff.rows; i++ ) 
    {
      for( int j = 0; j < diff.cols; j++ ) 
        {
          uchar v = diff.at<uchar>(i, j);

          if( v > THRESHOLD )  
            cnt++;
          
          if (minval > v) minval = v;
          if (maxval < v) maxval = v;
        }
    }

  float err_per = 100.0 * (float) cnt / (cv_img_inp.rows * cv_img_inp.cols);
  
  printf( "\nMinimum error in intensity = %f\n", minval);
  printf(   "Maximum error in intensity = %f\n", maxval);

  printf( "\nPercentage of pixels above error threshold = %f\n", err_per);
        
  if(err_per > 1) 
    {  
      printf("\nTest Failed\n");  
      return -1; 
    }

  printf("\nTest Pass\n");  
  
  return 0;
}
