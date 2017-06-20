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



#include "xf_min_max_loc_config.h"


int main(int argc, char** argv)
{
	if (argc != 2)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path> \n");
		return -1;
	}

	cv::Mat in_img, in_gray, in_conv;

	/*  reading in the color image  */
	in_img = cv::imread(argv[1],1);

	if (in_img.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}

	/*  convert to gray  */
	cvtColor(in_img,in_gray,CV_BGR2GRAY);

	/*  convert to 16S type  */
#if T_8U
	in_gray.convertTo(in_conv,CV_8UC1);
#elif T_16U
	in_gray.convertTo(in_conv,CV_16UC1);
#elif T_16S
	in_gray.convertTo(in_conv,CV_16SC1);
#elif T_32S
	in_gray.convertTo(in_conv,CV_32SC1);
#endif



	double cv_minval=0,cv_maxval=0;
	cv::Point cv_minloc,cv_maxloc;

	/////////  OpenCV reference  ///////
	cv::minMaxLoc(in_conv, &cv_minval, &cv_maxval, &cv_minloc, &cv_maxloc, cv::noArray());

	int32_t min_value, max_value;
	uint16_t _min_locx,_min_locy,_max_locx,_max_locy;



#if NO
	#if T_8U
	xF::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> imgInput(in_conv.rows,in_conv.cols);
	imgInput.copyTo((unsigned char *)in_conv.data);
	
	#if __SDSCC__
	TIME_STAMP_INIT
	#endif
	xFminMaxLoc<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1>(imgInput, &min_value, &max_value, &_min_locx, &_min_locy, &_max_locx, &_max_locy);
	#if __SDSCC__
	TIME_STAMP
	#endif

	#elif T_16U
	xF::Mat<XF_16UC1, HEIGHT, WIDTH, XF_NPPC1> imgInput(in_conv.rows,in_conv.cols);
	imgInput.copyTo((unsigned short *)in_conv.data);

	#if __SDSCC__
	TIME_STAMP_INIT
	#endif
	xFminMaxLoc<XF_16UC1, HEIGHT, WIDTH, XF_NPPC1>(imgInput, &min_value, &max_value, &_min_locx, &_min_locy, &_max_locx, &_max_locy);
	#if __SDSCC__
	TIME_STAMP
	#endif

	#elif T_16S
	xF::Mat<XF_16SC1, HEIGHT, WIDTH, XF_NPPC1> imgInput(in_conv.rows,in_conv.cols);
	imgInput.copyTo((short *)in_conv.data);

	#if __SDSCC__
	TIME_STAMP_INIT
	#endif
	xFminMaxLoc<XF_16SC1, HEIGHT, WIDTH, XF_NPPC1>(imgInput, &min_value, &max_value, &_min_locx, &_min_locy, &_max_locx, &_max_locy);
	#if __SDSCC__
	TIME_STAMP
	#endif

#elif T_32S
	xF::Mat<XF_32SC1, HEIGHT, WIDTH, XF_NPPC1> imgInput(in_conv.rows,in_conv.cols);
	imgInput.copyTo((unsigned int *)in_conv.data);

	#if __SDSCC__
	TIME_STAMP_INIT
	#endif
	xFminMaxLoc<XF_32SC1, HEIGHT, WIDTH, XF_NPPC1>(imgInput, &min_value, &max_value, &_min_locx, &_min_locy, &_max_locx, &_max_locy);
	#if __SDSCC__
	TIME_STAMP
	#endif
	#endif

#endif

#if RO
		#if T_8U
		xF::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC8> imgInput(in_conv.rows,in_conv.cols);
		imgInput.copyTo((unsigned char *)in_conv.data);

		#if __SDSCC__
		TIME_STAMP_INIT
		#endif
		xFminMaxLoc<XF_8UC1, HEIGHT, WIDTH, XF_NPPC8>(imgInput, &min_value, &max_value, &_min_locx, &_min_locy, &_max_locx, &_max_locy);
		#if __SDSCC__
		TIME_STAMP
		#endif

		#elif T_16U
		xF::Mat<XF_16UC1, HEIGHT, WIDTH, XF_NPPC8> imgInput(in_conv.rows,in_conv.cols);
		imgInput.copyTo((unsigned short *)in_conv.data);

		#if __SDSCC__
		TIME_STAMP_INIT
		#endif
		xFminMaxLoc<XF_16UC1, HEIGHT, WIDTH, XF_NPPC8>(imgInput, &min_value, &max_value, &_min_locx, &_min_locy, &_max_locx, &_max_locy);
		#if __SDSCC__
		TIME_STAMP
		#endif

		#elif T_16S
		xF::Mat<XF_16SC1, HEIGHT, WIDTH, XF_NPPC8> imgInput(in_conv.rows,in_conv.cols);
		imgInput.copyTo((short int *)in_conv.data);

		#if __SDSCC__
		TIME_STAMP_INIT
		#endif
		xFminMaxLoc<XF_16SC1, HEIGHT, WIDTH, XF_NPPC8>(imgInput, &min_value, &max_value, &_min_locx, &_min_locy, &_max_locx, &_max_locy);
		#if __SDSCC__
		TIME_STAMP
		#endif

		#endif

#endif

	/////// OpenCV output ////////
	std::cout<<"OCV-Minvalue = "<<cv_minval<<std::endl;
	std::cout<<"OCV-Maxvalue = "<<cv_maxval<<std::endl;
	std::cout<<"OCV-Min Location.x = "<<cv_minloc.y<<"  OCV-Min Location.y = "<<cv_minloc.x<<std::endl;
	std::cout<<"OCV-Max Location.x = "<<cv_maxloc.y<<"  OCV-Max Location.y = "<<cv_maxloc.x<<std::endl;

	/////// Kernel output ////////
	std::cout<<"HLS-Minvalue = "<<min_value<<std::endl;
	std::cout<<"HLS-Maxvalue = "<<max_value<<std::endl;
	std::cout<<"HLS-Min Location.x = "<< _min_locx<< "  HLS-Min Location.y = "<<_min_locy<<std::endl;
	std::cout<<"HLS-Max Location.x = "<< _max_locx<< "  HLS-Max Location.y = "<<_max_locy<<std::endl;

	/////// printing the difference in min and max, values and locations of both OpenCV and Kernel function /////
	printf("Difference in Minimum value: %d \n",(cv_minval-min_value));
	printf("Difference in Maximum value: %d \n",(cv_maxval-max_value));
	printf("Difference in Minimum value location: (%d,%d) \n",(cv_minloc.y-_min_locx),(cv_minloc.x-_min_locy));
	printf("Difference in Maximum value location: (%d,%d) \n",(cv_maxloc.y-_max_locx),(cv_maxloc.x-_max_locy));


	if(((cv_minloc.y-_min_locx) > 1) | ((cv_minloc.x-_min_locy) > 1)){
		printf("\nTestcase failed\n");
		return -1;
	}

	return 0;
}
