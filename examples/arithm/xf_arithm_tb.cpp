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
#include "xf_arithm_config.h"


int main(int argc, char** argv)
{
	if (argc != 3)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage:\n");
		fprintf(stderr,"<Executable Name> <input image path1> <input image path2> \n");
		return -1;
	}

	cv::Mat in_img1, in_img2, in_gray1, in_gray2, in_gray1_16, in_gray2_16,out_img, ocv_ref, diff;
	/*  reading in the color image  */
	in_gray1 = cv::imread(argv[1],0);
	in_gray2 = cv::imread(argv[2],0);
	if (in_gray1.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[1]);
		return 0;
	}

	if (in_gray2.data == NULL)
	{
		fprintf(stderr,"Cannot open image at %s\n",argv[2]);
		return 0;
	}

	/*  convert to 16S type  */
	in_gray1.convertTo(in_gray1_16,CV_16SC1);
	in_gray2.convertTo(in_gray2_16,CV_16SC1);

#if T_8U
	static xf::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput1(in_gray1.rows,in_gray1.cols);
	static xf::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgInput2(in_gray1.rows,in_gray1.cols);

	out_img.create(in_gray1.rows,in_gray1.cols,in_gray1.depth());
	ocv_ref.create(in_gray2.rows,in_gray1.cols,in_gray1.depth());
	diff.create(in_gray1.rows,in_gray1.cols,in_gray1.depth());

	static xf::Mat<TYPE, HEIGHT, WIDTH, NPC1> imgOutput(in_gray1.rows,in_gray1.cols);

	imgInput1.copyTo(in_gray1.data);
	imgInput2.copyTo(in_gray2.data);


	#if __SDSCC__
	perf_counter hw_ctr;
	 hw_ctr.start();
	#endif
	
	arithm_accel(imgInput1,imgInput2,imgOutput);

	#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif



	///////////// OpenCV reference  /////////////
    //cv::absdiff(in_gray1,in_gray2,ocv_ref);
	//cv::add(in_gray1,in_gray2,ocv_ref,cv::noArray(),-1);
	//cv::subtract(in_gray1,in_gray2,ocv_ref,cv::noArray(),-1);
//	cv::bitwise_and(in_gray1,in_gray2,ocv_ref);
	//bitwise_or(in_gray1,in_gray2,ocv_ref);
	//cv::bitwise_not(in_gray1,ocv_ref);
	//cv::bitwise_xor(in_gray1,in_gray2,ocv_ref);
	cv::multiply(in_gray1,in_gray2,ocv_ref,0.05,-1);

#endif

#if T_16S
	out_img.create(in_gray1_16.rows,in_gray1_16.cols,in_gray1_16.depth());
	ocv_ref.create(in_gray1_16.rows,in_gray1_16.cols,in_gray1_16.depth());
	diff.create(in_gray1_16.rows,in_gray1_16.cols,in_gray1_16.depth());

	static xf::Mat<XF_16SC1, HEIGHT, WIDTH, NPC1> imgInput1_16(in_gray1.rows,in_gray1.cols);
	static xf::Mat<XF_16SC1, HEIGHT, WIDTH, NPC1> imgInput2_16(in_gray1.rows,in_gray1.cols);

	static xf::Mat<XF_16SC1, HEIGHT, WIDTH, NPC1> imgOutput(in_gray1.rows,in_gray1.cols);

	imgInput1_16.copyTo(in_gray1_16.data);
	imgInput2_16.copyTo(in_gray2_16.data);

	#if __SDSCC__
	perf_counter hw_ctr;
	 hw_ctr.start();
	#endif

	arithm_accel(imgInput1_16,imgInput2_16,imgOutput);

	#if __SDSCC__
	hw_ctr.stop();
	uint64_t hw_cycles = hw_ctr.avg_cpu_cycles();
	#endif
	
	///////////// OpenCV reference  /////////////
	//cv::add(in_gray1_16,in_gray2_16,ocv_ref,cv::noArray(),-1);
//	cv::subtract(in_gray1_16,in_gray2_16,ocv_ref,cv::noArray(),-1);
	cv::multiply(in_gray1_16,in_gray2_16,ocv_ref,0.05,-1);
#endif


	xf::imwrite("hls_out.jpg",imgOutput);
	imwrite("ref_img.jpg",ocv_ref);   // save the reference image

	xf::absDiff(ocv_ref,imgOutput,diff);	  // Compute absolute difference image
	//cv::absdiff(ocv_ref,out_img,diff);
	imwrite("diff_img.jpg",diff);            // Save the difference image for debugging purpose

	FILE *fp2=fopen("diff.txt","w");
	// Find minimum and maximum differences.
	double minval=256,maxval=0;
	int cnt = 0;
	for (int i=0;i<in_gray1.rows;i++)
	{
		for(int j=0;j<in_gray1.cols;j++)
		{
			uchar v = diff.at<uchar>(i,j);
			fprintf(fp2,"%d\n",v);
			if (v>2)
				cnt++;
			if (minval > v )
				minval = v;
			if (maxval < v)
				maxval = v;
		}
	}
	fclose(fp2);
	float err_per = 100.0*(float)cnt/(in_gray1.rows*in_gray1.cols);
	fprintf(stderr,"Minimum error in intensity = %f\nMaximum error in intensity = %f\nPercentage of pixels above error threshold = %f\n",minval,maxval,err_per);

	in_img1.~Mat();
	in_img2.~Mat();
	in_gray1.~Mat();
	in_gray2.~Mat();
	in_gray1_16.~Mat();
	in_gray2_16.~Mat();
	out_img.~Mat();
	ocv_ref.~Mat();
	diff.~Mat();

	if(err_per > 0.0f)
	{
		return 1;
	}

	return 0;
}
