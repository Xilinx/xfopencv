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
#include "xf_stereo_pipeline_config.h"
#include "cameraParameters.h"

#include <CL/cl.h>
#include "xcl2.hpp" 

using namespace std;

int main(int argc, char** argv)
{
	cv::setUseOptimized(false);

	if(argc != 3)
	{
		fprintf(stderr,"Invalid Number of Arguments!\nUsage: <executable> <left image> <right image>\n");
		return -1;
	}


	cv::Mat left_img, right_img;
	left_img = cv::imread(argv[1],0);
	if (left_img.data == NULL)
	{
		fprintf(stderr,"Cannot open left image at %s\n", argv[1]);
		return 0;
	}
	right_img = cv::imread(argv[2],0);
	if (right_img.data == NULL)
	{
		fprintf(stderr,"Cannot open right image at %s\n", argv[1]);
		return 0;
	}

	//////////////////	HLS TOP Function Call  ////////////////////////
	int rows = left_img.rows;
	int cols = left_img.cols;
	cv::Mat disp_img(rows,cols,CV_16UC1);

	// allocate mem for camera parameters for rectification and bm_state class
	ap_fixed<32,12> *cameraMA_l_fix = (ap_fixed<32,12>*)malloc(XF_CAMERA_MATRIX_SIZE*sizeof(ap_fixed<32,12>));
	ap_fixed<32,12> *cameraMA_r_fix = (ap_fixed<32,12>*)malloc(XF_CAMERA_MATRIX_SIZE*sizeof(ap_fixed<32,12>));
	ap_fixed<32,12> *irA_l_fix = (ap_fixed<32,12>*)malloc(XF_CAMERA_MATRIX_SIZE*sizeof(ap_fixed<32,12>));
	ap_fixed<32,12> *irA_r_fix = (ap_fixed<32,12>*)malloc(XF_CAMERA_MATRIX_SIZE*sizeof(ap_fixed<32,12>));
	ap_fixed<32,12> *distC_l_fix = (ap_fixed<32,12>*)malloc(XF_DIST_COEFF_SIZE*sizeof(ap_fixed<32,12>));
	ap_fixed<32,12> *distC_r_fix = (ap_fixed<32,12>*)malloc(XF_DIST_COEFF_SIZE*sizeof(ap_fixed<32,12>));
	int *bm_state_arr = (int*)malloc(11*sizeof(int));

	xf::xFSBMState<SAD_WINDOW_SIZE,NO_OF_DISPARITIES,PARALLEL_UNITS> bm_state;
	bm_state.uniquenessRatio = 15;
	bm_state.textureThreshold = 20;
	bm_state.minDisparity = 0;
	bm_state_arr[0] = bm_state.preFilterType;
	bm_state_arr[1] = bm_state.preFilterSize;
	bm_state_arr[2] = bm_state.preFilterCap;
	bm_state_arr[3] = bm_state.SADWindowSize;
	bm_state_arr[4] = bm_state.minDisparity;
	bm_state_arr[5] = bm_state.numberOfDisparities;
	bm_state_arr[6] = bm_state.textureThreshold;
	bm_state_arr[7] = bm_state.uniquenessRatio;
	bm_state_arr[8] = bm_state.ndisp_unit;
	bm_state_arr[9] = bm_state.sweepFactor;
	bm_state_arr[10] = bm_state.remainder;

	// copy camera params
	for(int i=0; i<XF_CAMERA_MATRIX_SIZE; i++) {
		cameraMA_l_fix[i] = (ap_fixed<32,12>)cameraMA_l[i];
		cameraMA_r_fix[i] = (ap_fixed<32,12>)cameraMA_r[i];
		irA_l_fix[i] = (ap_fixed<32,12>)irA_l[i];
		irA_r_fix[i] = (ap_fixed<32,12>)irA_r[i];
	}

	// copy distortion coefficients
	for(int i=0; i<XF_DIST_COEFF_SIZE; i++) {
		distC_l_fix[i] = (ap_fixed<32,12>)distC_l[i];
		distC_r_fix[i] = (ap_fixed<32,12>)distC_r[i];
	}

/////////////////////////////////////// CL ////////////////////////
	
	std::vector<cl::Device> devices = xcl::get_xil_devices();
	cl::Device device = devices[0];
	cl::Context context(device);
  
	cl::CommandQueue q(context, device,CL_QUEUE_PROFILING_ENABLE);

	std::string device_name = device.getInfo<CL_DEVICE_NAME>(); 
	std::string binaryFile = xcl::find_binary_file(device_name,"krnl_stereopipeline");
	cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
	devices.resize(1);
	cl::Program program(context, devices, bins);
	cl::Kernel krnl(program,"stereopipeline_accel");

	std::vector<cl::Memory> inBuf_left, inBuf_right, outBuf_disp;
	std::vector<cl::Memory> inBuf_cam_matL, inBuf_cam_matR, inBuf_distcoefL, inBuf_distcoefR, inBuf_raL, inBuf_raR;
	std::vector<cl::Memory> inBuf_sbmstate;

	cl::Buffer imageToDeviceL(context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, rows*cols, (ap_uint<INPUT_PTR_WIDTH>*)left_img.data);
	cl::Buffer imageToDeviceR(context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, rows*cols, (ap_uint<INPUT_PTR_WIDTH>*)right_img.data);
	cl::Buffer imageFromDevice(context,CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,2*rows*cols, (ap_uint<OUTPUT_PTR_WIDTH>*)disp_img.data);
	cl::Buffer arrToDeviceCML(context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(ap_fixed<32,12>)*XF_CAMERA_MATRIX_SIZE, cameraMA_l_fix);
	cl::Buffer arrToDeviceCMR(context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(ap_fixed<32,12>)*XF_CAMERA_MATRIX_SIZE, cameraMA_r_fix);
	cl::Buffer arrToDeviceDCL(context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(ap_fixed<32,12>)*XF_DIST_COEFF_SIZE, distC_l_fix);
	cl::Buffer arrToDeviceDCR(context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(ap_fixed<32,12>)*XF_DIST_COEFF_SIZE, distC_r_fix);
	cl::Buffer arrToDeviceRAL(context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(ap_fixed<32,12>)*XF_CAMERA_MATRIX_SIZE, irA_l_fix);
	cl::Buffer arrToDeviceRAR(context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(ap_fixed<32,12>)*XF_CAMERA_MATRIX_SIZE, irA_r_fix);
	cl::Buffer structToDevicesbmstate(context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, 11*sizeof(int), bm_state_arr);

	inBuf_left.push_back(imageToDeviceL);
	inBuf_right.push_back(imageToDeviceR);
	outBuf_disp.push_back(imageFromDevice);
	inBuf_cam_matL.push_back(arrToDeviceCML);
	inBuf_cam_matR.push_back(arrToDeviceCMR);
	inBuf_distcoefL.push_back(arrToDeviceDCL);
	inBuf_distcoefR.push_back(arrToDeviceDCR);
	inBuf_raL.push_back(arrToDeviceRAL);
	inBuf_raR.push_back(arrToDeviceRAR);
	inBuf_sbmstate.push_back(structToDevicesbmstate);

	/* Copy input vectors to memory */
	q.enqueueMigrateMemObjects(inBuf_left,0); /* 0 means from host*/
	q.enqueueMigrateMemObjects(inBuf_right,0);
	q.enqueueMigrateMemObjects(inBuf_cam_matL,0);
	q.enqueueMigrateMemObjects(inBuf_cam_matR,0);
	q.enqueueMigrateMemObjects(inBuf_distcoefL,0);
	q.enqueueMigrateMemObjects(inBuf_distcoefR,0);
	q.enqueueMigrateMemObjects(inBuf_raL,0);
	q.enqueueMigrateMemObjects(inBuf_raR,0);
	q.enqueueMigrateMemObjects(inBuf_sbmstate,0);

	// Set the kernel arguments
	krnl.setArg(0, imageToDeviceL);
	krnl.setArg(1, imageToDeviceR);
	krnl.setArg(2, imageFromDevice);
	krnl.setArg(3, arrToDeviceCML);
	krnl.setArg(4, arrToDeviceCMR);
	krnl.setArg(5, arrToDeviceDCL);
	krnl.setArg(6, arrToDeviceDCR);
	krnl.setArg(7, arrToDeviceRAL);
	krnl.setArg(8, arrToDeviceRAR);
	krnl.setArg(9, structToDevicesbmstate);
	krnl.setArg(10,rows);
	krnl.setArg(11,cols);

	// Profiling Objects
	cl_ulong start= 0;
	cl_ulong end = 0;
	double diff_prof = 0.0f;
	cl::Event event_sp;

	// Launch the kernel 
	q.enqueueTask(krnl,NULL,&event_sp);
	
	// profiling
	clWaitForEvents(1, (const cl_event*) &event_sp);
	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_START,&start);
	event_sp.getProfilingInfo(CL_PROFILING_COMMAND_END,&end);
	diff_prof = end-start;
	std::cout<<(diff_prof/1000000)<<"ms"<<std::endl;

	q.enqueueMigrateMemObjects(outBuf_disp,CL_MIGRATE_MEM_OBJECT_HOST);
	q.finish();
/////////////////////////////////////// end of CL ////////////////////////

	// Write output image
	cv::Mat out_disp_img(rows,cols,CV_8UC1);
	disp_img.convertTo(out_disp_img, CV_8U, (256.0/NO_OF_DISPARITIES)/(16.));
	cv::imwrite("hls_output.png",out_disp_img);
	printf ("run complete !\n\n");

	return 0;
}

