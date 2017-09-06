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

#include "xf_headers.h"
#include "xf_harris_config.h"
#include "xf_ocv_ref.hpp"

int main(int argc,char **argv)
{
	cv::Mat in_img, img_gray;
	cv::Mat hls_out_img, ocv_out_img;
	cv::Mat ocvpnts, hlspnts;


	if(argc != 2){
		printf("Usage : <executable> <input image> \n");
		return -1;
	}
	in_img = cv::imread(argv[1],1); // reading in the color image

	if(!in_img.data)
	{
		printf("Failed to load the image ... %s\n!", argv[1]);
		return -1;
	}

	uint16_t Thresh;										// Threshold for HLS
	float Th;
	if (FILTER_WIDTH == 3){
		Th = 30532960.00;
		Thresh = 442;
	}
	else if (FILTER_WIDTH == 5){
		Th = 902753878016.0;
		Thresh = 3109;
	}
	else if (FILTER_WIDTH == 7){
		Th = 41151168289701888.000000;
		Thresh = 566;
	}
	cvtColor(in_img, img_gray, CV_BGR2GRAY);
	// Convert rgb into grayscale
	hls_out_img.create(img_gray.rows, img_gray.cols, CV_8U); 					// create memory for hls output image
	ocv_out_img.create(img_gray.rows, img_gray.cols, CV_8U);		 			// create memory for opencv output image

	ocv_ref(img_gray, ocv_out_img, Th);
	/**************		HLS Function	  *****************/
	float K = 0.04;
	uint16_t k = K*(1<<16);														// Convert to Q0.16 format
	uint32_t nCorners = 0;
	uint16_t imgwidth  = in_img.cols;
	uint16_t imgheight = in_img.rows;
	unsigned int *list;

#if __SDSCC__
	list = (unsigned int *)sds_alloc_non_cacheable((MAXCORNERS)*sizeof( unsigned int));
#else
	list = (unsigned int *)malloc((MAXCORNERS)*sizeof( unsigned int));
#endif

	

#if NO
	xf::Mat<XF_8UC1, HEIGHT, WIDTH, XF_NPPC1> imgInput(img_gray.rows,img_gray.cols);

	imgInput.copyTo(img_gray.data);
#ifdef __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
	harris_accel(imgInput,(ap_uint<32>*)list, Thresh, k, &nCorners);
#ifdef __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

#endif

#if RO

	xf::Mat<XF_8UC1,HEIGHT,WIDTH,XF_NPPC8> imgInput(img_gray.rows,img_gray.cols);

	imgInput.copyTo(img_gray.data);
#ifdef __SDSCC__
	perf_counter hw_ctr;
	hw_ctr.start();
#endif
	harris_accel(imgInput,(ap_uint<32>*)list, Thresh, k, &nCorners);
#ifdef __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
#endif

#endif

	

	unsigned int val;
	unsigned short int row, col;

	cv::Mat out_img;
	out_img = img_gray.clone();

	std::vector<cv::Point> hls_points;
	std::vector<cv::Point> ocv_points;
	std::vector<cv::Point> common_pts;
	/*						Mark HLS points on the image 				*/

	for(int i=0; i<nCorners; i++)
	{
		val = list[i];
		col = (unsigned short)(val & (0x0000ffff));
		row = (unsigned short)(val>>16) & (0x0000ffff);
		cv::Point tmp;
		tmp.x = col;
		tmp.y = row;
		if((tmp.x < img_gray.cols) && (tmp.y <img_gray.rows) && (row>0)){
			hls_points.push_back(tmp);
		}
		short int y, x;
		y = row;
		x = col;
		if (row > 0)
			cv::circle(out_img, cv::Point( x, y), 5,  Scalar(0,0,255,255), 2, 8, 0 );
	}
	/*						End of marking HLS points on the image 				*/
	/*						Write HLS and Opencv corners into a file			*/

	ocvpnts = img_gray.clone();


	int nhls = hls_points.size();

	/// Drawing a circle around corners
	for( int j = 1; j < ocv_out_img.rows-1 ; j++ ){
		for( int i = 1; i < ocv_out_img.cols-1; i++ ){
			if( (int)ocv_out_img.at<unsigned char>(j,i) ){
				cv::circle( ocvpnts, cv::Point( i, j ), 5,  Scalar(0,0,255), 2, 8, 0 );
				ocv_points.push_back(cv::Point(i,j));

			}
		}
	}

	printf("ocv corner count = %d, Hls corner count = %d\n", ocv_points.size(), nCorners);
	int nocv = ocv_points.size();

	/*									End 								*/
	/*							Find common points in among opencv and HLS						*/
	int ocv_x, ocv_y, hls_x, hls_y;
	for(int j=0;j<nocv;j++)
	{
		for(int k=0; k<nhls; k++)
		{
			ocv_x = ocv_points[j].x;
			ocv_y = ocv_points[j].y;
			hls_x = hls_points[k].x;
			hls_y = hls_points[k].y;

			if((ocv_x==hls_x) && (ocv_y==hls_y)){
				common_pts.push_back(ocv_points[j]);
				break;
			}
		}
	}
	/*							End								*/
	imwrite("output_hls.png", out_img);									// HLS Image
	imwrite("output_ocv.png", ocvpnts);									// Opencv Image
	/*						Success, Loss and Gain Percentages					*/
	float persuccess,perloss,pergain;

	int totalocv = ocv_points.size();
	int ncommon = common_pts.size();
	int totalhls = hls_points.size();
	persuccess = (((float)ncommon/totalhls)* 100) ;
	perloss = (((float)(totalocv-ncommon)/totalocv)*100);
	pergain = (((float)(totalhls-ncommon)/totalhls)*100);

	printf("Commmon = %d\t Success = %f\t Loss = %f\t Gain = %f\n",ncommon,persuccess,perloss,pergain);

	if(persuccess < 60)
		return 1;


	return 0;
}

