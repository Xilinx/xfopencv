#include <stdlib.h>
#include <stdio.h>
#include <vector>

#include "xcl2.hpp" 

#include "xf_stereo_pipeline_config.h"

void stereopipeline_accel
  (
    xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &leftMat,   xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &rightMat, 
    
    xf::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &dispMat,
	  
    xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapxLMat, xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapyLMat, 
    xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapxRMat, xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapyRMat, 

    xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &leftRemappedMat, xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &rightRemappedMat,
	  
    xf::xFSBMState<SAD_WINDOW_SIZE,NO_OF_DISPARITIES,PARALLEL_UNITS> &bm_state, 
    
    ap_fixed<32,12> *cameraMA_l_fix,  ap_fixed<32,12> *cameraMA_r_fix, 
    ap_fixed<32,12> *distC_l_fix   ,  ap_fixed<32,12> *distC_r_fix   , 
	  ap_fixed<32,12> *irA_l_fix     ,  ap_fixed<32,12> *irA_r_fix     , 

    int _cm_size, int _dc_size
  )
{
    std::vector<cl::Device> devices = xcl::get_xil_devices();

    cl::Device device = devices[0];

    cl::Context context(device);

    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE);
    std::string device_name = device.getInfo<CL_DEVICE_NAME>();

    std::string binaryFile = "xf_stereopipeline.awsxclbin";
    
    std::cout << "========" <<  binaryFile << "  ==================" << std::endl;
    
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    cl::Program program(context, devices, bins);
    cl::Kernel kernel(program,"xf_stereopipeline");

    //----------- Allocate Buffer in Global Memory -----------//

//    cl::Buffer buffer_inp(context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY , imgInput.rows  * imgInput.cols, imgInput.data);
//    cl::Buffer buffer_out(context,CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, imgOutput.rows * imgOutput.cols, imgOutput.data);
//
//    std::vector<cl::Memory> writeBufVec;
//    writeBufVec.push_back(buffer_inp);
//    
//		//----------- Migrate  input data to device global memory -----------//
//    
//		q.enqueueMigrateMemObjects(writeBufVec,0);        // 0 means from host
//
//    auto krnl = cl::KernelFunctor<cl::Buffer&, cl::Buffer&, int, int, float, int, int >(kernel);
//
//    //----------- Launch the Kernel -----------//
//
//    krnl(cl::EnqueueArgs(q, cl::NDRange(1,1,1), cl::NDRange(1,1,1)), buffer_inp, buffer_out, imgInput.rows, imgInput.cols, sigma, imgOutput.rows, imgOutput.cols);
//    
//		//----------- Copy Result from Device Global Memory to Host Local Memory -----------//
//    
//    std::vector<cl::Memory> readBufVec;
//    readBufVec.push_back(buffer_out);
//
//		q.enqueueMigrateMemObjects(readBufVec,CL_MIGRATE_MEM_OBJECT_HOST);
//
//    q.finish();

}
