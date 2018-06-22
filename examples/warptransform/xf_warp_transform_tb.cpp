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

#include "xf_headers.h"
#include "xf_warp_transform_config.h"

//changing transformation matrix dimensions with transform Affine 2x3, Perspecitve 3x3
#if TRANSFORM_TYPE==1
	#define TRMAT_DIM2 3
	#define TRMAT_DIM1 3
#else
	#define TRMAT_DIM2 3
	#define TRMAT_DIM1 2
#endif

//Random Number generator limits
#define M_NUMI1 1
#define M_NUMI2 20

//image operations and transformation matrix input format
typedef float image_oper;

int main(int argc, char* argv[])
{
cv::RNG rng;

	image_oper R[9];
	cv::Mat _transformation_matrix(TRMAT_DIM1,TRMAT_DIM2,CV_32FC1);
	cv::Mat _transformation_matrix_2(TRMAT_DIM1,TRMAT_DIM2,CV_32FC1);
#if TRANSFORM_TYPE==1
	cv::Point2f src_p[4];
	cv::Point2f dst_p[4];
	src_p[0] = cv::Point2f(       0.0f,       0.0f);
	src_p[1] = cv::Point2f(    WIDTH-1,       0.0f);
	src_p[2] = cv::Point2f(    WIDTH-1,   HEIGHT-1);
	src_p[3] = cv::Point2f(       0.0f,   HEIGHT-1);
//	  to points
	dst_p[0] = cv::Point2f(         rng.uniform(int(M_NUMI1),int(M_NUMI2)),          rng.uniform(int(M_NUMI1),int(M_NUMI2)));
	dst_p[1] = cv::Point2f(   WIDTH-rng.uniform(int(M_NUMI1),int(M_NUMI2)),          rng.uniform(int(M_NUMI1),int(M_NUMI2)));
	dst_p[2] = cv::Point2f(   WIDTH-rng.uniform(int(M_NUMI1),int(M_NUMI2)),   HEIGHT-rng.uniform(int(M_NUMI1),int(M_NUMI2)));
	dst_p[3] = cv::Point2f(         rng.uniform(int(M_NUMI1),int(M_NUMI2)),   HEIGHT-rng.uniform(int(M_NUMI1),int(M_NUMI2)));

	_transformation_matrix = cv::getPerspectiveTransform(dst_p,src_p);
	cv::Mat transform_mat = _transformation_matrix;
#else
	cv::Point2f src_p[3];
	cv::Point2f dst_p[3];
	src_p[0] = cv::Point2f(       0.0f,       0.0f);
	src_p[1] = cv::Point2f(    WIDTH-1,       0.0f);
	src_p[2] = cv::Point2f(       0.0f,   HEIGHT-1);
//	  to points
	dst_p[0] = cv::Point2f(         rng.uniform(int(M_NUMI1),int(M_NUMI2)),         rng.uniform(int(M_NUMI1),int(M_NUMI2)));
	dst_p[1] = cv::Point2f(   WIDTH-rng.uniform(int(M_NUMI1),int(M_NUMI2)),         rng.uniform(int(M_NUMI1),int(M_NUMI2)));
	dst_p[2] = cv::Point2f(         rng.uniform(int(M_NUMI1),int(M_NUMI2)),  HEIGHT-rng.uniform(int(M_NUMI1),int(M_NUMI2)));

	_transformation_matrix = cv::getAffineTransform(dst_p,src_p);
	cv::Mat transform_mat = _transformation_matrix;
#endif
	int i=0,j=0;

	std::cout << "Transformation Matrix \n";
	for(i=0;i<3;i++)
	{
	  for(j=0;j<3;j++)
	  {
#if TRANSFORM_TYPE==1
		R[i*3+j] = image_oper(transform_mat.at<double>(i,j));
		_transformation_matrix_2.at<image_oper>(i,j) = image_oper(transform_mat.at<double>(i,j));
#else
		if(i==2)
		{
			R[i*3+j] = 0;
		}
		else
		{
			R[i*3+j] = image_oper(transform_mat.at<double>(i,j));
			_transformation_matrix_2.at<image_oper>(i,j) = image_oper(transform_mat.at<double>(i,j));
		}
#endif
		std::cout << R[i*3+j] << " ";
	  }
	  std::cout << "\n";
	}

	cv::Mat image_input,image_output;

	image_input = cv::imread(argv[1],0);
	image_output.create(image_input.rows,image_input.cols,image_input.depth());
	cv::Mat diff_img;
	diff_img.create(image_input.rows,image_input.cols,image_input.depth());

	if(image_input.data==NULL)
	{
		printf("Failed to load the image ... %s\n!",argv[1]);
		return -1;
	}
	cv::imwrite("input.png",image_input);

	static xf::Mat<TYPE,HEIGHT,WIDTH,XF_NPPC1> _src(image_input.rows,image_input.cols);
	static xf::Mat<TYPE,HEIGHT,WIDTH,XF_NPPC1> _dst(image_input.rows,image_input.cols);

	//_src = xf::imread<TYPE, HEIGHT, WIDTH, XF_NPPC1>(argv[1], 0);
	_src.copyTo(image_input.data);

	#if __SDSCC__
	perf_counter hw_ctr;
		hw_ctr.start();
	#endif
	warp_transform_accel(_src, _dst, R);
	#if __SDSCC__
		hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif

	xf::imwrite("hls_out.jpg",_dst);

	cv::imwrite("output.png",image_output);

	image_output.data = _dst.copyFrom();

	cv::Mat opencv_image;
	opencv_image.create(HEIGHT,WIDTH,image_input.depth());
	for(int I1=0;I1<opencv_image.rows; I1++)
	{
		for(int J1=0;J1<opencv_image.cols; J1++)
		{
			opencv_image.at<ap_uint8_t>(I1,J1)=0;
		}
	}

#if TRANSFORM_TYPE==1
	#if INTERPOLATION==1
		  cv::warpPerspective(image_input, opencv_image, _transformation_matrix_2, cv::Size(image_input.cols, image_input.rows), cv::INTER_LINEAR + cv::WARP_INVERSE_MAP, cv::BORDER_TRANSPARENT,80);
	#else
		  cv::warpPerspective(image_input, opencv_image, _transformation_matrix_2, cv::Size(image_input.cols, image_input.rows), cv::INTER_NEAREST + cv::WARP_INVERSE_MAP, cv::BORDER_TRANSPARENT,80);
	#endif
#else
	#if INTERPOLATION==1
		  cv::warpAffine(image_input, opencv_image, _transformation_matrix_2, cv::Size(image_input.cols, image_input.rows), cv::INTER_LINEAR + cv::WARP_INVERSE_MAP, cv::BORDER_TRANSPARENT,80);
	#else
		  cv::warpAffine(image_input, opencv_image, _transformation_matrix_2, cv::Size(image_input.cols, image_input.rows), cv::INTER_NEAREST + cv::WARP_INVERSE_MAP, cv::BORDER_TRANSPARENT,80);
	#endif
#endif

	char output_opencv[]= "opencv_output.png";
	cv::imwrite(output_opencv,opencv_image);

	ap_uint8_t temp_px1=0,temp_px2=0,max_err=0,min_err=255;
	int num_errs=0,num_errs1=0;
	for(i=0;i<image_output.rows; i++)
	{
		for(j=0;j<image_output.cols; j++)
		{
			if(image_output.at<ap_uint8_t>(i,j) == opencv_image.at<ap_uint8_t>(i,j))
			{
				diff_img.at<ap_uint8_t>(i,j) = 0;
			}
			else
			{
				if(image_output.at<ap_uint8_t>(i,j) > opencv_image.at<ap_uint8_t>(i,j))
				{
					diff_img.at<ap_uint8_t>(i,j) = image_output.at<ap_uint8_t>(i,j) - opencv_image.at<ap_uint8_t>(i,j);
				}
				else
				{
					diff_img.at<ap_uint8_t>(i,j) = -image_output.at<ap_uint8_t>(i,j) + opencv_image.at<ap_uint8_t>(i,j);
				}
				num_errs = num_errs+1;
				if(max_err < diff_img.at<ap_uint8_t>(i,j))
				{
					max_err = diff_img.at<ap_uint8_t>(i,j);
				}
				if(min_err > diff_img.at<ap_uint8_t>(i,j))
				{
					min_err = diff_img.at<ap_uint8_t>(i,j);
				}

				if(diff_img.at<ap_uint8_t>(i,j) > 0)
				{
					num_errs1 = num_errs1 + 1;
					diff_img.at<ap_uint8_t>(i,j) = 255;
				}
			}
		}
	}
	min_err= (min_err == ap_uint8_t(255))? ap_uint8_t(0) : min_err;
	std::cout << "\nMax_err: " << max_err << " Min_err: " << min_err << " Num_errs: " << num_errs <<  " Num_errs > 1: " << num_errs1 << std::endl;
	cv::imwrite("diff_image.png",diff_img);
	char output_image_name[]= "output.png";
	//cv::imwrite(output_image_name,image_output);

	if(num_errs > 10000)
	{
		return -1;
	}
	else
	{
		return 0;
	}
}
