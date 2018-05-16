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
HOWEVER CXFSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***************************************************************************/

#ifndef _XF_PARAMS_H_
#define _XF_PARAMS_H_

#ifndef __cplusplus
#error C++ is needed to use this file!
#endif

#include "ap_int.h"

#define __ABS(X) ((X) < 0 ? (-(X)) : (X))  

//Channels of an image
enum _channel_extract
{
	XF_EXTRACT_CH_0,  //Used by formats with unknown channel types
	XF_EXTRACT_CH_1,  //Used by formats with unknown channel types
	XF_EXTRACT_CH_2,  //Used by formats with unknown channel types
	XF_EXTRACT_CH_3,  //Used by formats with unknown channel types
	XF_EXTRACT_CH_R,  //Used to extract the RED channel
	XF_EXTRACT_CH_G,  //Used to extract the GREEN channel
	XF_EXTRACT_CH_B,  //Used to extract the BLUE channel
	XF_EXTRACT_CH_A,  //Used to extract the ALPHA channel
	XF_EXTRACT_CH_Y,  //Used to extract the LUMA channel
	XF_EXTRACT_CH_U,  //Used to extract the Cb/U channel
	XF_EXTRACT_CH_V	  //Used to extract the Cr/V/Value channel
};
typedef _channel_extract XF_channel_extract_e;


//Conversion Policy for fixed point arithmetic
enum _convert_policy
{
	XF_CONVERT_POLICY_SATURATE,
	XF_CONVERT_POLICY_TRUNCATE
};
typedef _convert_policy XF_convert_policy_e;


//Bit-depth conversion types
enum _convert_bit_depth
{
	//Down-convert
	XF_CONVERT_16U_TO_8U ,
	XF_CONVERT_16S_TO_8U ,
	XF_CONVERT_32S_TO_8U ,
	XF_CONVERT_32S_TO_16U,
	XF_CONVERT_32S_TO_16S,
	//Up-convert
	XF_CONVERT_8U_TO_16U ,
	XF_CONVERT_8U_TO_16S ,
	XF_CONVERT_8U_TO_32S ,
	XF_CONVERT_16U_TO_32S,
	XF_CONVERT_16S_TO_32S
};
typedef _convert_bit_depth XF_convert_bit_depth_e;


//Thresholding types
enum _threshold_type
{
	XF_THRESHOLD_TYPE_BINARY,
	XF_THRESHOLD_TYPE_RANGE
};
typedef _threshold_type XF_threshold_type_e;


//Pixel Per Cycle
enum _pixel_per_cycle
{
	XF_NPPC1  = 1,
	XF_NPPC2  = 2,
	XF_NPPC4  = 4,
	XF_NPPC8  = 8,
	XF_NPPC16 = 16,
	XF_NPPC32 = 32
};
typedef _pixel_per_cycle XF_nppc_e;


//Pixel types
enum _pixel_type
{
	XF_8UP  = 0,
	XF_8SP  = 1,
	XF_16UP = 2,
	XF_16SP = 3,
	XF_32UP = 4,
	XF_32SP = 5,
	XF_19SP = 6,
	XF_32FP = 7,
	XF_35SP = 8,
	XF_24SP = 9,
	XF_20SP = 10,
	XF_48SP = 11,
	XF_2UP = 12,
	XF_9SP = 13,
	XF_9UP = 14,
	XF_24UP = 15,
};
typedef _pixel_type XF_pixel_type_e;

//Word width
enum _word_width
{
	XF_8UW   = 0,
	XF_16UW  = 1,
	XF_64UW  = 2,
	XF_128UW = 3,
	XF_256UW = 4,
	XF_512UW = 5,
	XF_22UW  = 6,
	XF_176UW = 7,
	XF_352UW = 8,
	XF_32UW  = 9,
	XF_19SW  = 10,
	XF_152SW = 11,
	XF_304SW = 12,
	XF_35SW  = 13,
	XF_280SW = 14,
	XF_560SW = 15,
	XF_192SW = 16,
	XF_160SW = 17,
	XF_24SW  = 18,
	XF_9UW   = 19,
	XF_72UW  = 20,
	XF_144UW = 21,
	XF_576UW = 22,
	XF_32FW = 23,
	XF_2UW = 24,
	XF_48UW = 25,
	XF_24UW = 26
};
typedef _word_width XF_word_width_e;


//Filter size
enum _filter_size 
{
	XF_FILTER_3X3 = 3,
	XF_FILTER_5X5 = 5,
	XF_FILTER_7X7 = 7
};
typedef _filter_size XF_filter_size_e;

//Radius size for Non Maximum Suppression
enum _nms_radius
{
	XF_NMS_RADIUS_1 = 1,
	XF_NMS_RADIUS_2 = 2,
	XF_NMS_RADIUS_3 = 3
};
typedef _nms_radius XF_nms_radius_e;

//Image Pyramid Parameters
enum _image_pyramid_params
{
	XF_PYRAMID_TYPE_GXFSSIAN = 0,
	XF_PYRAMID_TYPE_LAPLACIAN = 1,
	XF_PYRAMID_SCALE_HALF = 2,
	XF_PYRAMID_SCALE_ORB = 3,
	XF_PYRAMID_SCALE_DOUBLE = 4
};
typedef _image_pyramid_params XF_image_pyramid_params_e;

//Magnitude computation
enum _normalisation_params
{
	XF_L1NORM = 0,
	XF_L2NORM = 1
};
typedef _normalisation_params XF_normalisation_params_e;

enum _border_type
{
	XF_BORDER_CONSTANT = 0,
	XF_BORDER_REPLICATE = 1
};
typedef _border_type XF_border_type_e;

//Phase computation
enum _phase_params
{
	XF_RADIANS = 0,
	XF_DEGREES = 1
};
typedef _phase_params XF_phase_params_e;

//Types of Interpolaton techniques used in resize, affine and perspective
enum _interpolation_types
{
	XF_INTERPOLATION_NN = 0,
	XF_INTERPOLATION_BILINEAR = 1,
	XF_INTERPOLATION_AREA = 2
};
typedef _interpolation_types _interpolation_types_e;

// loop dependent variables used in image pyramid
enum _loop_dependent_vars
{
	XF_GXFSSIANLOOP = 8,
	XF_BUFSIZE = 12
};
typedef _loop_dependent_vars loop_dependent_vars_e;

// loop dependent variables used in image pyramid
enum _image_size
{
	XF_SDIMAGE = 0,
	XF_HDIMAGE = 1
};
typedef _image_size image_size_e;

// enumerations for HOG feature descriptor
enum _input_image_type
{
	XF_GRAY = 1,
	XF_RGB = 3
};
typedef _input_image_type input_image_type_e;

// enumerations for HOG feature descriptor
enum _HOG_output_type
{
	XF_HOG_RB = 0,
	XF_HOG_NRB = 1
};
typedef _HOG_output_type HOG_output_type_e;

enum use_model
{
	XF_STANDALONE = 0,
	XF_PIPELINE = 1
};
typedef use_model use_model_e;

// enumerations for HOG feature descriptor
enum _HOG_type
{
	XF_DHOG = 0,
	XF_SHOG = 1
};
typedef _HOG_type HOG_type_e;

// enumerations for Stereo BM
enum XF_stereo_prefilter_type
{
	XF_STEREO_PREFILTER_SOBEL_TYPE,
	XF_STEREO_PREFILTER_NORM_TYPE
};
/****************************new************************/
//typedef XF_stereo_prefilter_type XF_stereo_pre_filter_type_e;
//enum _pixel_percycle
//{
//	XF_NPPC1  = 0,
//	XF_NPPC8  = 3,
//	XF_NPPC16 = 4
//};
//typedef _pixel_percycle XF_nppc_e;


enum _pixeltype
{
	XF_8UC1  = 0,
	XF_16UC1  = 1,
	XF_16SC1  = 2,
	XF_32UC1  = 3,
	XF_32FC1 = 4,
	XF_32SC1 = 5,
	XF_8UC2  = 6,
	XF_8UC4  = 7,
	XF_2UC1  = 8,
	XF_8UC3  = 9,
	XF_16UC3  = 10,
};
typedef _pixeltype XF_npt_e;

#endif//_XF_PARAMS_H_
