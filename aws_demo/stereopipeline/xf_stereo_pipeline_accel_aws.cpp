#include <stdlib.h>
#include <stdio.h>
#include <vector>

#include "xcl2.hpp" 

#include "xf_stereo_pipeline_config.h"

#define CL_MIGRATE_MEM_OBJECT_KERNEL 0       //OpenCL define constant to indicate memory object migration to host only, to make program more readable define "counterpart" constant

void stereo_pipeline_accel
  (
    xf::Mat<XF_8UC1 , XF_HEIGHT, XF_WIDTH, XF_NPPC1> &xf_img_l       , xf::Mat<XF_8UC1 , XF_HEIGHT, XF_WIDTH, XF_NPPC1> &xf_img_r, 
                                                                    
    xf::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &xf_img_s       ,
                                                                  
    xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &xf_map_x_l     , 
    xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &xf_map_y_l     , 
                                                                       xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &xf_map_x_r, 
                                                                       xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &xf_map_y_r, 
    
    xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &xf_remaped_l    , xf::Mat<XF_8UC1 , XF_HEIGHT, XF_WIDTH, XF_NPPC1> &xf_remaped_r,

    xf::xFSBMState<SAD_WINDOW_SIZE,NO_OF_DISPARITIES,PARALLEL_UNITS> &bm_state, 
    
    ap_fixed<32,12> *cameraMA_l_fix, ap_fixed<32,12> *cameraMA_r_fix, 
    ap_fixed<32,12> *distC_l_fix   , ap_fixed<32,12> *distC_r_fix   , 
    ap_fixed<32,12> *irA_l_fix     , ap_fixed<32,12> *irA_r_fix     , 
    
    int cm_size, 
    int dc_size
  )
{
    std::vector<cl::Device> devices = xcl::get_xil_devices();

    cl::Device device = devices[0];

    cl::Context context(device);

    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE);
    std::string device_name = device.getInfo<CL_DEVICE_NAME>();

    std::string binaryFile = "xf_stereo_pipeline.awsxclbin";
    
    std::cout << "========" <<  binaryFile << "  ==================" << std::endl;
    
    cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
    devices.resize(1);
    cl::Program program(context, devices, bins);
    cl::Kernel kernel(program,"xf_stereo_pipeline");

    //----------- Allocate Buffer in Global Memory -----------//

    int rows = xf_img_l.rows;
    int cols = xf_img_l.cols;

    int pixel_qnt = rows * cols;

    cl::Buffer buffer_l   (context, cl_mem_flags(CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY ), pixel_qnt * 1, (void*)xf_img_l.data );          cl::Buffer buffer_r   (context, cl_mem_flags(CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY), pixel_qnt * 1, (void*)xf_img_r.data);
                                                                                        
    cl::Buffer buffer_cm_l(context, cl_mem_flags(CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY ), cm_size   * 4, (void*)cameraMA_l_fix);          cl::Buffer buffer_cm_r(context, cl_mem_flags(CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY), cm_size   * 4, (void*)cameraMA_r_fix);
    cl::Buffer buffer_dc_l(context, cl_mem_flags(CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY ), dc_size   * 4, (void*)distC_l_fix   );          cl::Buffer buffer_dc_r(context, cl_mem_flags(CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY), dc_size   * 4, (void*)distC_r_fix   );
    cl::Buffer buffer_ir_l(context, cl_mem_flags(CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY ), cm_size   * 4, (void*)irA_l_fix     );          cl::Buffer buffer_ir_r(context, cl_mem_flags(CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY), cm_size   * 4, (void*)irA_r_fix     );

    cl::Buffer buffer_s   (context, cl_mem_flags(CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY), pixel_qnt * 2, xf_img_s.data);                                                                                     

    std::vector<cl::Memory> kernel_wr_buf;

    kernel_wr_buf.push_back(buffer_l   );               kernel_wr_buf.push_back(buffer_r   );
    kernel_wr_buf.push_back(buffer_cm_l);               kernel_wr_buf.push_back(buffer_cm_r);
    kernel_wr_buf.push_back(buffer_dc_l);               kernel_wr_buf.push_back(buffer_dc_r);
    kernel_wr_buf.push_back(buffer_ir_l);               kernel_wr_buf.push_back(buffer_ir_r);
    
    //----------- Migrate  input data to device global memory -----------//
    
    q.enqueueMigrateMemObjects(kernel_wr_buf, CL_MIGRATE_MEM_OBJECT_KERNEL);

    // The kernel parameters should be rearranged: input buffers, output buffers, variables
    // 
    //                                img_l        img_r        cm_l         cm_r         dc_l         dc_r         ir_l         ir_r         img_s    cm_size  dc_size  rows  cols
    auto krnl = cl::KernelFunctor<cl::Buffer&, cl::Buffer&, cl::Buffer&, cl::Buffer&, cl::Buffer&, cl::Buffer&, cl::Buffer&, cl::Buffer&, cl::Buffer&, int,     int,     int,  int >(kernel);

    //----------- Launch the Kernel -----------//

    krnl(cl::EnqueueArgs(q, cl::NDRange(1,1,1), cl::NDRange(1,1,1)), buffer_l, buffer_r, buffer_cm_l, buffer_cm_r, buffer_dc_l, buffer_dc_r, buffer_ir_l, buffer_ir_r, buffer_s, cm_size, dc_size, rows, cols);
    
    //----------- Copy Result from Device Global Memory to Host Local Memory -----------//
    
    std::vector<cl::Memory> kernel_rd_buf;
    kernel_rd_buf.push_back(buffer_s);

    q.enqueueMigrateMemObjects(kernel_rd_buf, CL_MIGRATE_MEM_OBJECT_HOST);

    q.finish();
}
