#include <stdlib.h>
#include <stdio.h>
#include <vector>

#include "xcl2.hpp" 

#include "xf_gaussian_filter_config.h"

#define CL_MIGRATE_MEM_OBJECT_KERNEL 0       //OpenCL define constant to indicate memory object migration to host only, to make program more readable define "counterpart" constant

void gaussian_filter_accel(xf::Mat<XF_8UC1, ROWS_INP, COLS_INP, NPC1> &img_inp, xf::Mat<XF_8UC1, ROWS_OUT, COLS_OUT, NPC1> &img_out, float sigma)
{
    std::vector<cl::Device> devices = xcl::get_xil_devices();

    cl::Device device = devices[0];

    cl::Context context(device);

    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE);
    std::string device_name = device.getInfo<CL_DEVICE_NAME>();

    std::string binaryFile = (xcl::is_emulation() || xcl::is_hw_emulation ()) ? "xf_gaussian_filter.xclbin" : "xf_gaussian_filter.awsxclbin";
    
    std::cout << "======== " <<  binaryFile << " ========" << std::endl;
    
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    cl::Program program(context, devices, bins);
    cl::Kernel kernel(program,"xf_gaussian_filter");

    //----------- Allocate Buffer in Global Memory -----------//

    cl::Buffer buffer_inp(context,CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY , img_inp.rows * img_inp.cols, img_inp.data);
    cl::Buffer buffer_out(context,CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, img_out.rows * img_out.cols, img_out.data);

    std::vector<cl::Memory> writeBufVec;
    writeBufVec.push_back(buffer_inp);
    
    //----------- Migrate  input data to device global memory -----------//
    
    q.enqueueMigrateMemObjects(writeBufVec, CL_MIGRATE_MEM_OBJECT_KERNEL);

    auto krnl = cl::KernelFunctor<cl::Buffer&, cl::Buffer&, int, int, float, int, int >(kernel);

    //----------- Launch the Kernel -----------//

    krnl(cl::EnqueueArgs(q, cl::NDRange(1,1,1), cl::NDRange(1,1,1)), buffer_inp, buffer_out, img_inp.rows, img_inp.cols, sigma, img_out.rows, img_out.cols);
    
    //----------- Copy Result from Device Global Memory to Host Local Memory -----------//
    
    std::vector<cl::Memory> readBufVec;
    readBufVec.push_back(buffer_out);

    q.enqueueMigrateMemObjects(readBufVec,CL_MIGRATE_MEM_OBJECT_HOST);

    q.finish();
}
