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
#include "xf_fast_config.h"


int main (int argc, char** argv)
{

	cv::Mat in_img,out_img,out_img_ocv, out_hls;
	cv::Mat in_gray;

	in_img = cv::imread(argv[1],1); // reading in the color image

	if(!in_img.data)
	{
		printf("Failed to load the image ... %s\n!", argv[1]);
		return -1;
	}

	unsigned int nCorners;
	std::vector<cv::KeyPoint> keypoints;

	uchar_t threshold = 20;								// threshold
	//uchar_t NMS = 0;									// non maximum supression

	cvtColor(in_img,in_gray,CV_BGR2GRAY);

	unsigned short imgwidth = in_img.cols;
	unsigned short imgheight = in_img.rows;

	unsigned int *	list;

#if __SDSCC__
	list=(unsigned int *)sds_alloc_non_cacheable(sizeof(unsigned int)*MAXCORNERS);
#else
	list=(unsigned int *)malloc(sizeof(unsigned int)*MAXCORNERS);
#endif





	xF::Mat<XF_8UC1, HEIGHT, WIDTH, NPC1> imgInput(in_gray.rows,in_gray.cols);


	imgInput.copyTo(in_gray.data);


	#if __SDSCC__
	TIME_STAMP_INIT
	#endif

	//xFFAST<NMS,MAXCORNERS,XF_8UC1,HEIGHT,WIDTH,NPC1>(imgInput,(ap_uint<32>*)list,threshold,&nCorners);
	fast_accel(imgInput,(ap_uint<32>*)list,threshold,&nCorners);

	#if __SDSCC__
	TIME_STAMP
	#endif




	cv::FAST(in_gray,keypoints,threshold,NMS);		//OPenCV reference function

	std::vector<cv::Point> hls_points;
	std::vector<cv::Point> ocv_points;
	std::vector<cv::Point> common_points;
	std::vector<cv::Point> noncommon_points;

	FILE *fp, *fp1;
	fp = fopen("ocvpoints.txt","w");
	fp1 = fopen("hlspoints.txt","w");

	int nsize = keypoints.size();

	printf("ocvpoints:%d=\n",nsize);

	for(int i=0;i<nsize;i++)
	{
		int x=keypoints[i].pt.x;
		int y=keypoints[i].pt.y;
		ocv_points.push_back(cv::Point(x,y));
		fprintf(fp,"x = %d, y = %d\n",x,y);
	}
	fclose(fp);

	out_img_ocv = in_img.clone();

	int ocv_x=0,ocv_y=0;

	for (int cnt1=0;cnt1<keypoints.size();cnt1++)
	{
		ocv_x = keypoints[cnt1].pt.x;
		ocv_y = keypoints[cnt1].pt.y;
		cv::circle( out_img_ocv, cv::Point( ocv_x, ocv_y), 5,  cv::Scalar(0,0,255), 2, 8, 0 );
	}
	cv::imwrite("output_ocv.png", out_img_ocv);

	ap_uint<32> val;
	ap_uint<16> row, col;
	int row1,col1;
	out_img = in_img.clone();

	ap_uint<32> pt;
	//pt = list[0];

	for (int i=0;i<nCorners;i++)
	{
		pt = list[i];
		val = pt;
		row = val.range(31,16);
		col = val.range(15,0);

		if (val != 0)
		{
			cv::circle( out_img, cv::Point( col, row), 5,  cv::Scalar(0,0,255), 2, 8, 0 );
			row1 = (int) row;
			col1 = (int) col;
			hls_points.push_back(cv::Point(col1, row1));
		}
	}
	////////////////////////////////////

	int nsize1 = hls_points.size();

	int Nocv = ocv_points.size();
	int Nhls = hls_points.size();

	for(int r=0; r<nsize1; r++)
	{
		int a, b;
		a = (int)hls_points[r].x;
		b = (int)hls_points[r].y;
		fprintf(fp1,"x = %d, y = %d\n",a,b);
	}
	fclose(fp1);

	for(int j=0;j<Nocv;j++)
	{
		for(int k=0;k<Nhls;k++)
		{
			if((ocv_points[j].x==((hls_points[k].x))) && (ocv_points[j].y==((hls_points[k].y))))
			{
				common_points.push_back(ocv_points[j]);
			}

		}
	}

	FILE *fpt3;
	fpt3=fopen("common.txt","w");

	for(int p=0;p<common_points.size();p++)
	{
		fprintf(fpt3,"x = %d, y = %d\n",common_points[p].x,common_points[p].y);
	}
	fclose(fpt3);



	float persuccess,perloss,pergain;

	int totalocv = ocv_points.size();
	int totalhls = hls_points.size();
	int ncommon = common_points.size();

	persuccess = (((float)ncommon/totalhls)* 100) ;
	perloss = (((float)(totalocv-ncommon)/totalocv)*100);
	pergain = (((float)(totalhls-ncommon)/totalhls)*100);

	printf("Commmon = %d\t Success = %f\t Loss = %f\t Gain = %f\n",ncommon,persuccess,perloss,pergain);

	if(persuccess < 80)
		return 1;

	imwrite("output_hls.png", out_img);

	out_img.~Mat();
	in_img.~Mat();
	in_gray.~Mat();
	out_img_ocv.~Mat();
	hls_points.clear();
	ocv_points.clear();
	common_points.clear();
	keypoints.clear();



	return 0;
}


