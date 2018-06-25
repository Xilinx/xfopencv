//Includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "xf_stereo_pipeline_config.h"

#include "imgproc/xf_stereo_pipeline.hpp"
#include "imgproc/xf_remap.hpp"
#include "imgproc/xf_stereoBM.hpp"


extern "C"                                            
  {                                                      
    void xf_stereo_pipeline                            
      (                                     
        //           Left                 |              Right
        XF_TNAME(XF_8UC1, XF_NPPC1) *img_l,  XF_TNAME(XF_8UC1, XF_NPPC1) *img_r,       
                                                      
        ap_fixed<32,12> *cameraMA_l_fix   ,  ap_fixed<32,12> *cameraMA_r_fix,                                                     
        ap_fixed<32,12> *distC_l_fix      ,  ap_fixed<32,12> *distC_r_fix   ,                                                     
        ap_fixed<32,12> *irA_l_fix        ,  ap_fixed<32,12> *irA_r_fix     ,                                                     

        XF_TNAME(XF_16UC1, XF_NPPC1) *img_d ,

        int preFilterType,
        int preFilterCap,
        int minDisparity,
        int textureThreshold,
        int uniquenessRatio,

        int cm_size, 
        int dc_size,                   
                                                      
        int rows,                                     
        int cols                                      
      );                                                                                                                                                        
  }

void xf_stereo_pipeline
  (    
    //           Left                 |              Right                                              
    XF_TNAME(XF_8UC1, XF_NPPC1) *img_l,  XF_TNAME(XF_8UC1 , XF_NPPC1) *img_r,      
                                                                         
    ap_fixed<32,12> *cameraMA_l_fix   ,  ap_fixed<32,12> *cameraMA_r_fix,                                                     
    ap_fixed<32,12> *distC_l_fix      ,  ap_fixed<32,12> *distC_r_fix   ,                                                     
    ap_fixed<32,12> *irA_l_fix        ,  ap_fixed<32,12> *irA_r_fix     ,                                                     
                                                                         
    XF_TNAME(XF_16UC1, XF_NPPC1) *img_d,

    int preFilterType,
    int preFilterCap,
    int minDisparity,
    int textureThreshold,
    int uniquenessRatio,

    int cm_size, 
    int dc_size,                  
                                                                         
    int rows,                                    
    int cols                                     
  )
{
  #pragma HLS INTERFACE m_axi     port=img_l      offset=slave bundle=gmem_i_l
  #pragma HLS INTERFACE m_axi     port=img_r      offset=slave bundle=gmem_i_r
                                                 
  #pragma HLS INTERFACE m_axi     port=cameraMA_l_fix    offset=slave bundle=gmem_l
  #pragma HLS INTERFACE m_axi     port=cameraMA_r_fix    offset=slave bundle=gmem_r
                                                         
  #pragma HLS INTERFACE m_axi     port=distC_l_fix       offset=slave bundle=gmem_l
  #pragma HLS INTERFACE m_axi     port=distC_r_fix       offset=slave bundle=gmem_r
                                                         
  #pragma HLS INTERFACE m_axi     port=irA_l_fix         offset=slave bundle=gmem_l
  #pragma HLS INTERFACE m_axi     port=irA_r_fix         offset=slave bundle=gmem_r

  #pragma HLS INTERFACE m_axi     port=img_d             offset=slave bundle=gmem_s


  #pragma HLS INTERFACE s_axilite     port=img_l              bundle=control
  #pragma HLS INTERFACE s_axilite     port=img_r              bundle=control
                                                             
  #pragma HLS INTERFACE s_axilite     port=cameraMA_l_fix     bundle=control
  #pragma HLS INTERFACE s_axilite     port=cameraMA_r_fix     bundle=control
                                                              
  #pragma HLS INTERFACE s_axilite     port=distC_l_fix        bundle=control
  #pragma HLS INTERFACE s_axilite     port=distC_r_fix        bundle=control
                                                              
  #pragma HLS INTERFACE s_axilite     port=irA_l_fix          bundle=control
  #pragma HLS INTERFACE s_axilite     port=irA_r_fix          bundle=control


  #pragma HLS INTERFACE s_axilite     port=img_d              bundle=control


  #pragma HLS INTERFACE s_axilite port=preFilterType     bundle=control
  #pragma HLS INTERFACE s_axilite port=preFilterCap      bundle=control
  #pragma HLS INTERFACE s_axilite port=minDisparity      bundle=control
  #pragma HLS INTERFACE s_axilite port=textureThreshold  bundle=control
  #pragma HLS INTERFACE s_axilite port=uniquenessRatio   bundle=control

  #pragma HLS INTERFACE s_axilite port=cm_size           bundle=control
  #pragma HLS INTERFACE s_axilite port=dc_size           bundle=control
                                                      
  #pragma HLS INTERFACE s_axilite port=rows              bundle=control
  #pragma HLS INTERFACE s_axilite port=cols              bundle=control
                                                      
  #pragma HLS INTERFACE s_axilite port=return            bundle=control


  #pragma HLS INLINE OFF
  #pragma HLS dataflow


  const int pROWS = XF_HEIGHT;
  const int pCOLS = XF_WIDTH ;

  const int pNPC  = XF_NPPC1;

  xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> xf_img_l;   // don't use non default constructor xf::Mat<...> xf_img_l(rows, cols) - kernel will suspend on hw emulation and FPGA
  xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> xf_img_r;

  #pragma HLS stream variable=xf_img_l.data  depth=pCOLS/pNPC 
  #pragma HLS stream variable=xf_img_r.data depth=pCOLS/pNPC 


  xf::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> xf_img_d;

  #pragma HLS stream variable=xf_img_d.data  depth=pCOLS/pNPC 

  xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> xf_map_x_l;
  xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> xf_map_y_l;

  #pragma HLS stream variable=xf_map_x_l.data depth=pCOLS/pNPC 
  #pragma HLS stream variable=xf_map_y_l.data depth=pCOLS/pNPC 

  xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> xf_map_x_r;
  xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> xf_map_y_r;

  #pragma HLS stream variable=xf_map_x_r.data depth=pCOLS/pNPC 
  #pragma HLS stream variable=xf_map_y_r.data depth=pCOLS/pNPC 

  xf::Mat<XF_8UC1,  XF_HEIGHT, XF_WIDTH, XF_NPPC1> xf_remapped_l;
  xf::Mat<XF_8UC1,  XF_HEIGHT, XF_WIDTH, XF_NPPC1> xf_remapped_r;

  #pragma HLS stream variable=xf_remapped_l.data depth=pCOLS/pNPC 
  #pragma HLS stream variable=xf_remapped_r.data depth=pCOLS/pNPC 

  xf::xFSBMState<SAD_WINDOW_SIZE,NO_OF_DISPARITIES,PARALLEL_UNITS> bm_state;

  xf_img_l.rows = rows; xf_img_l.cols = cols;
  xf_img_r.rows = rows; xf_img_r.cols = cols;
  xf_img_d.rows = rows; xf_img_d.cols = cols;
  
  xf_map_x_l.rows = rows; xf_map_x_l.cols = cols;
  xf_map_y_l.rows = rows; xf_map_y_l.cols = cols;
  xf_map_x_r.rows = rows; xf_map_x_r.cols = cols;
  xf_map_y_r.rows = rows; xf_map_y_r.cols = cols;

  xf_remapped_l.rows = rows;  xf_remapped_l.cols = cols;
  xf_remapped_r.rows = rows;  xf_remapped_r.cols = cols;

	bm_state.preFilterType       = preFilterType   ;
	bm_state.preFilterCap        = preFilterCap    ;
	bm_state.minDisparity        = minDisparity    ;
	bm_state.textureThreshold    = textureThreshold;
	bm_state.uniquenessRatio     = uniquenessRatio ;

  for(int i=0; i < rows; i++)
    {
      #pragma HLS LOOP_TRIPCOUNT min=1 max=pROWS

      for(int j=0; j < (cols >> (XF_BITSHIFT(XF_NPPC1))); j++)
        {
          #pragma HLS LOOP_TRIPCOUNT min=1 max=pCOLS/pNPC
          #pragma HLS PIPELINE
          #pragma HLS loop_flatten off

          *(xf_img_l.data + i*(cols >> (XF_BITSHIFT(XF_NPPC1))) +j) = *(img_l + i*(cols >> (XF_BITSHIFT(XF_NPPC1))) +j);
          *(xf_img_r.data + i*(cols >> (XF_BITSHIFT(XF_NPPC1))) +j) = *(img_r + i*(cols >> (XF_BITSHIFT(XF_NPPC1))) +j);         
        }
    }

 
  xf::InitUndistortRectifyMapInverse < XF_CAMERA_MATRIX_SIZE, XF_DIST_COEFF_SIZE, XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1 > (cameraMA_l_fix, distC_l_fix, irA_l_fix, xf_map_x_l, xf_map_y_l, cm_size, dc_size);

  xf::remap <XF_REMAP_BUFSIZE, XF_INTERPOLATION_BILINEAR, XF_8UC1, XF_32FC1, XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1, false> ( xf_img_l, xf_remapped_l, xf_map_x_l, xf_map_y_l ); 



  xf::InitUndistortRectifyMapInverse < XF_CAMERA_MATRIX_SIZE, XF_DIST_COEFF_SIZE, XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1 > (cameraMA_r_fix, distC_r_fix, irA_r_fix, xf_map_x_r, xf_map_y_r, cm_size, dc_size);

  xf::remap <XF_REMAP_BUFSIZE, XF_INTERPOLATION_BILINEAR, XF_8UC1, XF_32FC1, XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1, false> ( xf_img_r, xf_remapped_r, xf_map_x_r, xf_map_y_r); 




  xf::StereoBM <SAD_WINDOW_SIZE, NO_OF_DISPARITIES, PARALLEL_UNITS, XF_8UC1, XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> ( xf_remapped_l, xf_remapped_r, xf_img_d, bm_state);




  for(int i=0; i < rows; i++)
    {
      #pragma HLS LOOP_TRIPCOUNT min=1 max=pROWS

      for(int j=0; j < (cols >> (XF_BITSHIFT(XF_NPPC1))); j++)
        {
          #pragma HLS LOOP_TRIPCOUNT min=1 max=pCOLS/pNPC
          #pragma HLS PIPELINE
          #pragma HLS loop_flatten off

          *(img_d + i*(cols >> (XF_BITSHIFT(XF_NPPC1))) +j) = *(xf_img_d.data + i*(cols >> (XF_BITSHIFT(XF_NPPC1))) +j);
        }
    }

}
