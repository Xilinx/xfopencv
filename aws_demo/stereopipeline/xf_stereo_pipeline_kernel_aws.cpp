//Includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "xf_stereo_pipeline_config.h"

extern "C"                                            
  {                                                      
    void xf_stereopipeline                            
      (                                               
        XF_TNAME(XF_8UC1 , XF_NPPC1) *img_l ,       
                                                      
//        XF_TNAME(XF_8UC1 , XF_NPPC1) *img_r,       
                                                      
//        XF_TNAME(XF_8UC1 , XF_NPPC1) *img_s ,       

        XF_TNAME(XF_32FC1 , XF_NPPC1) *img_map_x_l,       
        XF_TNAME(XF_32FC1 , XF_NPPC1) *img_map_y_l,         

        XF_TNAME(XF_8UC1 , XF_NPPC1) *img_remapped_l ,       

        ap_fixed<32,12> *cameraMA_l_fix, // ap_fixed<32,12> *cameraMA_r_fix,                                                     
        ap_fixed<32,12> *distC_l_fix   , // ap_fixed<32,12> *distC_r_fix   ,                                                     
        ap_fixed<32,12> *irA_l_fix     , // ap_fixed<32,12> *irA_r_fix     ,                                                     

        int cm_size, int dc_size,                   
                                                      
        int rows,                                     
        int cols                                      
      );                                                                                                                                                        
  }

void xf_stereopipeline(                                                  
                            XF_TNAME(XF_8UC1 , XF_NPPC1) *img_l,      
                                                                         
//                            XF_TNAME(XF_8UC1 , XF_NPPC1) *img_r,      
                                                                         
//                            XF_TNAME(XF_8UC1 , XF_NPPC1) *img_s,      

                            XF_TNAME(XF_32FC1 , XF_NPPC1) *img_map_x_l,       
                            XF_TNAME(XF_32FC1 , XF_NPPC1) *img_map_y_l,         

                            XF_TNAME(XF_8UC1 , XF_NPPC1) *img_remapped_l ,       
  
                            ap_fixed<32,12> *cameraMA_l_fix, // ap_fixed<32,12> *cameraMA_r_fix,                                                     
                            ap_fixed<32,12> *distC_l_fix   , // ap_fixed<32,12> *distC_r_fix   ,                                                     
                            ap_fixed<32,12> *irA_l_fix     , // ap_fixed<32,12> *irA_r_fix     ,                                                     
                                                                         
                            int cm_size, int dc_size,                  
                                                                         
                            int rows,                                    
                            int cols                                     
                         )
{
  #pragma HLS INTERFACE m_axi     port=img_l      offset=slave bundle=gmem_0
//  #pragma HLS INTERFACE m_axi     port=img_r      offset=slave bundle=gmem_0
                                                 
//  #pragma HLS INTERFACE m_axi     port=img_s      offset=slave bundle=gmem

  #pragma HLS INTERFACE m_axi     port=img_map_x_l  offset=slave bundle=gmem_1
  #pragma HLS INTERFACE m_axi     port=img_map_y_l  offset=slave bundle=gmem_1

  #pragma HLS INTERFACE m_axi     port=img_remapped_l offset=slave bundle=gmem_0

//  #pragma HLS INTERFACE m_axi     port=mapxLMat   offset=slave bundle=gmem
//  #pragma HLS INTERFACE m_axi     port=mapyLMat   offset=slave bundle=gmem
//
//  #pragma HLS INTERFACE m_axi     port=mapxRMat   offset=slave bundle=gmem
//  #pragma HLS INTERFACE m_axi     port=mapyRMat   offset=slave bundle=gmem
//
//  #pragma HLS INTERFACE m_axi     port=leftRemappedMat   offset=slave bundle=gmem
//  #pragma HLS INTERFACE m_axi     port=rightRemappedMat  offset=slave bundle=gmem


  #pragma HLS INTERFACE m_axi     port=cameraMA_l_fix    offset=slave bundle=gmem
//  #pragma HLS INTERFACE m_axi     port=cameraMA_r_fix    offset=slave bundle=gmem
                                                         
  #pragma HLS INTERFACE m_axi     port=distC_l_fix       offset=slave bundle=gmem
//  #pragma HLS INTERFACE m_axi     port=distC_r_fix       offset=slave bundle=gmem
                                                         
  #pragma HLS INTERFACE m_axi     port=irA_l_fix         offset=slave bundle=gmem
//  #pragma HLS INTERFACE m_axi     port=irA_r_fix         offset=slave bundle=gmem


  #pragma HLS INTERFACE s_axilite     port=img_l              bundle=control
//  #pragma HLS INTERFACE s_axilite     port=img_r              bundle=control
                                                             
//  #pragma HLS INTERFACE s_axilite     port=img_s              bundle=control
                                       
  #pragma HLS INTERFACE s_axilite     port=img_map_x_l        bundle=control
  #pragma HLS INTERFACE s_axilite     port=img_map_y_l        bundle=control                         

  #pragma HLS INTERFACE s_axilite     port=img_remapped_l     bundle=control                         
                                                             
//  #pragma HLS INTERFACE s_axilite     port=mapxLMat           bundle=control
//  #pragma HLS INTERFACE s_axilite     port=mapyLMat           bundle=control
                                                             
//  #pragma HLS INTERFACE s_axilite     port=mapxRMat           bundle=control
//  #pragma HLS INTERFACE s_axilite     port=mapyRMat           bundle=control

//  #pragma HLS INTERFACE s_axilite     port=leftRemappedMat    bundle=control
//  #pragma HLS INTERFACE s_axilite     port=rightRemappedMat   bundle=control
                                                              
                                                              
  #pragma HLS INTERFACE s_axilite     port=cameraMA_l_fix     bundle=control
//  #pragma HLS INTERFACE s_axilite     port=cameraMA_r_fix     bundle=control
                                                              
  #pragma HLS INTERFACE s_axilite     port=distC_l_fix        bundle=control
//  #pragma HLS INTERFACE s_axilite     port=distC_r_fix        bundle=control
                                                              
  #pragma HLS INTERFACE s_axilite     port=irA_l_fix          bundle=control
//  #pragma HLS INTERFACE s_axilite     port=irA_r_fix          bundle=control


  #pragma HLS INTERFACE s_axilite port=cm_size    bundle=control
  #pragma HLS INTERFACE s_axilite port=dc_size    bundle=control

  #pragma HLS INTERFACE s_axilite port=rows        bundle=control
  #pragma HLS INTERFACE s_axilite port=cols        bundle=control

  #pragma HLS INTERFACE s_axilite port=return      bundle=control


  #pragma HLS dataflow


  const int pROWS = XF_HEIGHT;
  const int pCOLS = XF_WIDTH ;

  const int pNPC  = XF_NPPC1;


  xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> leftMat;
//  xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> rightMat;

  #pragma HLS stream variable=leftMat.data  depth=pCOLS/pNPC 
//  #pragma HLS stream variable=rightMat.data depth=pCOLS/pNPC 



//  xf::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> dispMat;
//
//  #pragma HLS stream variable=dispMat.data  depth=pCOLS/pNPC 

  xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> map_x_l;
  xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> map_y_l;

  #pragma HLS stream variable=map_x_l.data depth=pCOLS/pNPC 
  #pragma HLS stream variable=map_y_l.data depth=pCOLS/pNPC 

//  xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> map_x_r;
//  xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> map_y_r;

//  #pragma HLS stream variable=map_x_r.data depth=pCOLS/pNPC 
//  #pragma HLS stream variable=map_y_r.data depth=pCOLS/pNPC 

  xf::Mat<XF_8UC1,  XF_HEIGHT, XF_WIDTH, XF_NPPC1> remapped_l;
//  xf::Mat<XF_8UC1,  XF_HEIGHT, XF_WIDTH, XF_NPPC1> remapped_r;

  #pragma HLS stream variable=remapped_l.data depth=pCOLS/pNPC 
//  #pragma HLS stream variable=remapped_r.data depth=pCOLS/pNPC 

  //=================================================//

//  xf::xFSBMState<SAD_WINDOW_SIZE,NO_OF_DISPARITIES,PARALLEL_UNITS> bm_state;

//  ap_fixed<32,12> cameraMA_l_fix[XF_CAMERA_MATRIX_SIZE];
//  ap_fixed<32,12> cameraMA_r_fix[XF_CAMERA_MATRIX_SIZE]; 

//  #pragma HLS stream variable=cameraMA_l_fix depth=2 
//  #pragma HLS stream variable=cameraMA_r_fix depth=2
  
//  ap_fixed<32,12> distC_l_fix[XF_DIST_COEFF_SIZE];
//  ap_fixed<32,12> distC_r_fix[XF_DIST_COEFF_SIZE];

//  #pragma HLS stream variable=distC_l_fix depth=2 
//  #pragma HLS stream variable=distC_r_fix depth=2

//  ap_fixed<32,12> irA_l_fix[XF_CAMERA_MATRIX_SIZE];
//  ap_fixed<32,12> irA_r_fix[XF_CAMERA_MATRIX_SIZE];

//  #pragma HLS stream variable=irA_l_fix depth=2 
//  #pragma HLS stream variable=irA_r_fix depth=2

  //================================================//


  for(int i=0; i < rows; i++)
    {
      #pragma HLS LOOP_TRIPCOUNT min=1 max=pROWS

      for(int j=0; j < (cols >> (XF_BITSHIFT(XF_NPPC1))); j++)
        {
          #pragma HLS LOOP_TRIPCOUNT min=1 max=pCOLS/pNPC
          #pragma HLS PIPELINE
          #pragma HLS loop_flatten off

          *(leftMat.data + i*(cols >> (XF_BITSHIFT(XF_NPPC1))) +j) = *(img_l + i*(cols >> (XF_BITSHIFT(XF_NPPC1))) +j);
        }
    }

  
  xf::InitUndistortRectifyMapInverse < XF_CAMERA_MATRIX_SIZE, XF_DIST_COEFF_SIZE, XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1 > (cameraMA_l_fix, distC_l_fix, irA_l_fix, map_x_l, map_y_l, cm_size, dc_size);

  xf::remap <XF_REMAP_BUFSIZE, XF_INTERPOLATION_BILINEAR, XF_8UC1, XF_32FC1, XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1, false> ( leftMat, remapped_l, map_x_l, map_y_l ); 



//  xf::InitUndistortRectifyMapInverse < XF_CAMERA_MATRIX_SIZE, XF_DIST_COEFF_SIZE, XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1 > (cameraMA_r_fix, distC_r_fix, irA_r_fix, map_x_r, map_y_r, cm_size, dc_size);

//  xf::remap <XF_REMAP_BUFSIZE, XF_INTERPOLATION_BILINEAR, XF_8UC1, XF_32FC1, XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1, false> ( rightMat, remapped_r, map_x_r, map_y_r); 




//  xf::StereoBM <SAD_WINDOW_SIZE, NO_OF_DISPARITIES, PARALLEL_UNITS, XF_8UC1, XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> ( remapped_l, remapped_r, dispMat, bm_state );




  for(int i=0; i < rows; i++)
    {
      #pragma HLS LOOP_TRIPCOUNT min=1 max=pROWS

      for(int j=0; j < (cols >> (XF_BITSHIFT(XF_NPPC1))); j++)
        {
          #pragma HLS LOOP_TRIPCOUNT min=1 max=pCOLS/pNPC
          #pragma HLS PIPELINE
          #pragma HLS loop_flatten off

          //*(img_map_x_l + i*(cols >> (XF_BITSHIFT(XF_NPPC1))) +j) = *(map_x_l.data + i*(cols >> (XF_BITSHIFT(XF_NPPC1))) +j);
          //*(img_map_y_l + i*(cols >> (XF_BITSHIFT(XF_NPPC1))) +j) = *(map_y_l.data + i*(cols >> (XF_BITSHIFT(XF_NPPC1))) +j);

          *(img_remapped_l + i*(cols >> (XF_BITSHIFT(XF_NPPC1))) +j) = *(remapped_l.data + i*(cols >> (XF_BITSHIFT(XF_NPPC1))) +j);
          
        }
    }






}







//    void xf_stereopipeline                                                                                                                       
//      (                                                                                                                                          
//        XF_TNAME(XF_8UC1 , XF_NPPC1) *leftMat ,          // xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &leftMat,                             
//                                                         
//        XF_TNAME(XF_8UC1 , XF_NPPC1) *rightMat,          // xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &rightMat,                            
//                                                         
//        XF_TNAME(XF_8UC1 , XF_NPPC1) *dispMat ,          // xf::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &dispMat,                            
//                                                         
//        XF_TNAME(XF_32FC1, XF_NPPC1) *mapxLMat,          // xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapxLMat,    out + internal stream  
//                                                         
//        XF_TNAME(XF_32FC1, XF_NPPC1) *mapyLMat,          // xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapyLMat,    out + internal stream  
//                                                         
//        XF_TNAME(XF_32FC1, XF_NPPC1) *mapxRMat,          // xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapxRMat,    out + internal stream  
//                                                         
//        XF_TNAME(XF_32FC1, XF_NPPC1) *mapyRMat,          // xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapyRMat,    out + internal stream  
//                                                         
//        XF_TNAME(XF_8UC1 , XF_NPPC1) *leftRemappedMat,   // xf::Mat<XF_8UC1 , XF_HEIGHT, XF_WIDTH, XF_NPPC1> &leftRemappedMat,                    
//                                                         
//        XF_TNAME(XF_8UC1 , XF_NPPC1) *rightRemappedMat,  // xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &rightRemappedMat,  
//                                                                                                                               
//        xf::xFSBMState<SAD_WINDOW_SIZE,NO_OF_DISPARITIES,PARALLEL_UNITS> bm_state,                                             
//                                                                                                                               
//        ap_fixed<32,12> *cameraMA_l_fix,  ap_fixed<32,12> *cameraMA_r_fix,                                                     
//        ap_fixed<32,12> *distC_l_fix   ,  ap_fixed<32,12> *distC_r_fix   ,                                                     
//        ap_fixed<32,12> *irA_l_fix     ,  ap_fixed<32,12> *irA_r_fix     ,                                                     
//                                                                                                                               
//        int cm_size, int dc_size                                                                                             
//                                                                                                                               
//        int rows,                                                                                                              
//        int cols                                                                                                               
//      );                                                                                                                       







