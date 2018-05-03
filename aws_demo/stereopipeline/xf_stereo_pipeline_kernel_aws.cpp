//Includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "xf_stereo_pipeline_config.h"

template<int WIN_ROWS, int INTERPOLATION_TYPE, int SRC_T, int MAP_T, int DST_T, int ROWS, int COLS, int NPC = XF_NPPC1, bool USE_URAM = false>
void remap_aws( XF_TNAME(SRC_T,NPC) *_src_mat, XF_TNAME(SRC_T,NPC) *_remapped_mat, XF_TNAME(SRC_T,NPC) *_mapx_mat, XF_TNAME(SRC_T,NPC) *_mapy_mat, int m_rows, int m_cols );


extern "C"
  {
    void xf_stereopipeline
      (
        XF_TNAME(XF_8UC1 , XF_NPPC1) *leftMat ,            //xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &leftMat,   
                            
        XF_TNAME(XF_8UC1 , XF_NPPC1) *rightMat,            //xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &rightMat, 
    
        XF_TNAME(XF_8UC1 , XF_NPPC1) *dispMat ,            //xf::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &dispMat,
	  
        XF_TNAME(XF_32FC1, XF_NPPC1) *mapxLMat,            //xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapxLMat,  //out + internal stream
                                                                              
        XF_TNAME(XF_32FC1, XF_NPPC1) *mapyLMat,            //xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapyLMat,  //out + internal stream
                                                                              
        XF_TNAME(XF_32FC1, XF_NPPC1) *mapxRMat,            //xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapxRMat,  //out + internal stream
                                                                              
        XF_TNAME(XF_32FC1, XF_NPPC1) *mapyRMat,            //xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapyRMat,  //out + internal stream

        XF_TNAME(XF_8UC1 , XF_NPPC1) *leftRemappedMat,     //xf::Mat<XF_8UC1 , XF_HEIGHT, XF_WIDTH, XF_NPPC1> &leftRemappedMat, 
                            
        XF_TNAME(XF_8UC1 , XF_NPPC1) *rightRemappedMat,     //xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &rightRemappedMat,
	  
        xf::xFSBMState<SAD_WINDOW_SIZE,NO_OF_DISPARITIES,PARALLEL_UNITS> bm_state, 
    
        ap_fixed<32,12> *cameraMA_l_fix,  ap_fixed<32,12> *cameraMA_r_fix, 
        ap_fixed<32,12> *distC_l_fix   ,  ap_fixed<32,12> *distC_r_fix   , 
	      ap_fixed<32,12> *irA_l_fix     ,  ap_fixed<32,12> *irA_r_fix     , 

        int _cm_size, int _dc_size

        int rows,
        int cols 
      );
  }

void xf_stereopipeline(
                            XF_TNAME(XF_8UC1 , XF_NPPC1) *leftMat ,            //xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &leftMat,   
                            
                            XF_TNAME(XF_8UC1 , XF_NPPC1) *rightMat,            //xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &rightMat, 
    
                            XF_TNAME(XF_8UC1 , XF_NPPC1) *dispMat ,            //xf::Mat<XF_16UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &dispMat,
	  
                            XF_TNAME(XF_32FC1, XF_NPPC1) *mapxLMat,            //xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapxLMat,  //out + internal stream
                                                                              
                            XF_TNAME(XF_32FC1, XF_NPPC1) *mapyLMat,            //xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapyLMat,  //out + internal stream
                                                                              
                            XF_TNAME(XF_32FC1, XF_NPPC1) *mapxRMat,            //xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapxRMat,  //out + internal stream
                                                                              
                            XF_TNAME(XF_32FC1, XF_NPPC1) *mapyRMat,            //xf::Mat<XF_32FC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &mapyRMat,  //out + internal stream

                            XF_TNAME(XF_8UC1 , XF_NPPC1) *leftRemappedMat,     //xf::Mat<XF_8UC1 , XF_HEIGHT, XF_WIDTH, XF_NPPC1> &leftRemappedMat, 
                            
                            XF_TNAME(XF_8UC1 , XF_NPPC1) *rightRemappedMat,     //xf::Mat<XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1> &rightRemappedMat,
	  
                            xf::xFSBMState<SAD_WINDOW_SIZE,NO_OF_DISPARITIES,PARALLEL_UNITS> bm_state, 
    
                            ap_fixed<32,12> *cameraMA_l_fix,  ap_fixed<32,12> *cameraMA_r_fix, 
                            
                            ap_fixed<32,12> *distC_l_fix   ,  ap_fixed<32,12> *distC_r_fix   , 
	                          
                            ap_fixed<32,12> *irA_l_fix     ,  ap_fixed<32,12> *irA_r_fix     , 

                            int _cm_size, int _dc_size,

                            int rows,
                            int cols 
                         )
{
  #pragma HLS INTERFACE m_axi     port=leftMat    offset=slave bundle=gmem
  #pragma HLS INTERFACE m_axi     port=rightMat   offset=slave bundle=gmem
                                                 
  #pragma HLS INTERFACE m_axi     port=dispMat    offset=slave bundle=gmem

  #pragma HLS INTERFACE m_axi     port=mapxLMat   offset=slave bundle=gmem
  #pragma HLS INTERFACE m_axi     port=mapyLMat   offset=slave bundle=gmem

  #pragma HLS INTERFACE m_axi     port=mapxRMat   offset=slave bundle=gmem
  #pragma HLS INTERFACE m_axi     port=mapyRMat   offset=slave bundle=gmem

  #pragma HLS INTERFACE m_axi     port=leftRemappedMat   offset=slave bundle=gmem
  #pragma HLS INTERFACE m_axi     port=rightRemappedMat  offset=slave bundle=gmem


  #pragma HLS INTERFACE m_axi     port=cameraMA_l_fix    offset=slave bundle=gmem
  #pragma HLS INTERFACE m_axi     port=cameraMA_r_fix    offset=slave bundle=gmem
                                                         
  #pragma HLS INTERFACE m_axi     port=distC_l_fix       offset=slave bundle=gmem
  #pragma HLS INTERFACE m_axi     port=distC_r_fix       offset=slave bundle=gmem
                                                         
  #pragma HLS INTERFACE m_axi     port=irA_l_fix         offset=slave bundle=gmem
  #pragma HLS INTERFACE m_axi     port=irA_r_fix         offset=slave bundle=gmem


  #pragma HLS INTERFACE s_axilite     port=leftMat            bundle=control
  #pragma HLS INTERFACE s_axilite     port=rightMat           bundle=control
                                                             
  #pragma HLS INTERFACE s_axilite     port=dispMat            bundle=control
                                                             
  #pragma HLS INTERFACE s_axilite     port=mapxLMat           bundle=control
  #pragma HLS INTERFACE s_axilite     port=mapyLMat           bundle=control
                                                             
  #pragma HLS INTERFACE s_axilite     port=mapxRMat           bundle=control
  #pragma HLS INTERFACE s_axilite     port=mapyRMat           bundle=control

  #pragma HLS INTERFACE s_axilite     port=leftRemappedMat    bundle=control
  #pragma HLS INTERFACE s_axilite     port=rightRemappedMat   bundle=control
                                                              
                                                              
  #pragma HLS INTERFACE s_axilite     port=cameraMA_l_fix     bundle=control
  #pragma HLS INTERFACE s_axilite     port=cameraMA_r_fix     bundle=control
                                                              
  #pragma HLS INTERFACE s_axilite     port=distC_l_fix        bundle=control
  #pragma HLS INTERFACE s_axilite     port=distC_r_fix        bundle=control
                                                              
  #pragma HLS INTERFACE s_axilite     port=irA_l_fix          bundle=control
  #pragma HLS INTERFACE s_axilite     port=irA_r_fix          bundle=control


  #pragma HLS INTERFACE s_axilite port=_cm_size    bundle=control
  #pragma HLS INTERFACE s_axilite port=_dc_size    bundle=control

  #pragma HLS INTERFACE s_axilite port=rows        bundle=control
  #pragma HLS INTERFACE s_axilite port=cols        bundle=control

  #pragma HLS INTERFACE s_axilite port=return      bundle=control

  #pragma HLS inline off
  #pragma HLS dataflow




  XF_TNAME(XF_32FC1, XF_NPPC1) map_x_l[ XF_HEIGHT * (XF_WIDTH>>(XF_BITSHIFT(XF_NPPC1))) ];
  XF_TNAME(XF_32FC1, XF_NPPC1) map_y_l[ XF_HEIGHT * (XF_WIDTH>>(XF_BITSHIFT(XF_NPPC1))) ];

  XF_TNAME(XF_32FC1, XF_NPPC1) map_x_r[ XF_HEIGHT * (XF_WIDTH>>(XF_BITSHIFT(XF_NPPC1))) ];
  XF_TNAME(XF_32FC1, XF_NPPC1) map_y_r[ XF_HEIGHT * (XF_WIDTH>>(XF_BITSHIFT(XF_NPPC1))) ];

  XF_TNAME(XF_8UC1 , XF_NPPC1) remapped_l[ XF_HEIGHT * (XF_WIDTH>>(XF_BITSHIFT(XF_NPPC1))) ];
  XF_TNAME(XF_8UC1 , XF_NPPC1) remapped_r[ XF_HEIGHT * (XF_WIDTH>>(XF_BITSHIFT(XF_NPPC1))) ];



	xf::xFInitUndistortRectifyMapInverseKernel<XF_HEIGHT, XF_WIDTH, XF_CAMERA_MATRIX_SIZE, ap_fixed<32,12>, XF_DIST_COEFF_SIZE, XF_TNAME(XF_32FC1, XF_NPPC1)>(cameraMA_l_fix, distC_l_fix, irA_l_fix, map_x_l, map_y_l, rows, cols);

  remap_aws<XF_REMAP_BUFSIZE, XF_INTERPOLATION_BILINEAR, XF_8UC1, XF_32FC1, XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>( leftMat, remapped_l, map_x_l, map_y_l, rows, cols );	




	xf::xFInitUndistortRectifyMapInverseKernel<XF_HEIGHT, XF_WIDTH, XF_CAMERA_MATRIX_SIZE, ap_fixed<32,12>, XF_DIST_COEFF_SIZE, XF_TNAME(XF_32FC1, XF_NPPC1)>(cameraMA_r_fix, distC_r_fix, irA_r_fix, map_x_r, map_y_r, rows, cols);

  remap_aws<XF_REMAP_BUFSIZE, XF_INTERPOLATION_BILINEAR, XF_8UC1, XF_32FC1, XF_8UC1, XF_HEIGHT, XF_WIDTH, XF_NPPC1>( rightMat, remapped_r, map_x_r, map_y_r, rows, cols );	




	xf::xFFindStereoCorrespondenceLBM<XF_HEIGHT, XF_WIDTH, XF_8UC1, XF_16UC1, XF_NPPC1, SAD_WINDOW_SIZE, NO_OF_DISPARITIES, PARALLEL_UNITS>(remapped_l, remapped_r, dispMat, bm_state, rows, cols);
}












template<int WIN_ROWS, int INTERPOLATION_TYPE, int SRC_T, int MAP_T, int DST_T, int ROWS, int COLS, int NPC = XF_NPPC1, bool USE_URAM = false>
void remap_aws( XF_TNAME(SRC_T,NPC) *_src_mat, XF_TNAME(SRC_T,NPC) *_remapped_mat, XF_TNAME(SRC_T,NPC) *_mapx_mat, XF_TNAME(SRC_T,NPC) *_mapy_mat, int m_rows, int m_cols )
{
  #pragma HLS inline off
  #pragma HLS dataflow

	assert ((MAP_T == XF_32FC1) && "The MAP_T must be XF_32FC1");
	assert ((NPC == XF_NPPC1) && "The NPC must be XF_NPPC1");

	hls::stream< XF_TNAME(SRC_T,NPC)> _src;
	hls::stream< XF_TNAME(MAP_T,NPC)> _mapx;
	hls::stream< XF_TNAME(MAP_T,NPC)> _mapy;
	hls::stream< XF_TNAME(DST_T,NPC)> _remapped;

	int depth_est = WIN_ROWS * m_cols;

	uint16_t rows = m_rows;
	uint16_t cols = m_cols;

	int loop_count = (rows*cols);
	int TC=(ROWS*COLS);

	int ishift = WIN_ROWS/2;
	int row_tripcount = ROWS+WIN_ROWS;

	xfremap_rows_loop:
	for(int i = 0; i < rows+ishift; i++)
	  {
      #pragma HLS LOOP_FLATTEN OFF
      #pragma HLS LOOP_TRIPCOUNT min=1 max=row_tripcount

		  xfremap_cols_loop:
		  for (int j = 0; j < cols; j++)
		    {
          #pragma HLS pipeline ii=1
          #pragma HLS LOOP_TRIPCOUNT min=1 max=COLS

			    if (i < rows) 
            {
				      _src.write(*(_src_mat + i*cols + j));
			      }

			    if (i >= ishift) 
            {
				      _mapx.write(*(_mapx_mat + (i-ishift)*cols + j));
				      _mapy.write(*(_mapy_mat + (i-ishift)*cols + j));
			      }
		    }
	  }

	xf::xFRemapKernel <WIN_ROWS,INTERPOLATION_TYPE,ROWS,COLS,USE_URAM> (_src, _remapped, _mapx, _mapy, rows, cols);

	xfremap_output_loop:
	for (int i = 0; i < loop_count; i++)
	  {
      #pragma HLS pipeline ii=1
      #pragma HLS LOOP_TRIPCOUNT min=1 max=TC
		  _remapped_mat[i] = _remapped.read();
	  }
}
