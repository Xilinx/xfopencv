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
#ifndef __XF_PYR_DENSE_OPTICAL_FLOW_OFLOW_PROCESS__
#define __XF_PYR_DENSE_OPTICAL_FLOW_OFLOW_PROCESS__
template<unsigned short MAXHEIGHT, unsigned short MAXWIDTH, int WINSIZE, int IT_WIDTH, int IT_INT, int SIXIY_WIDTH, int SIXIY_INT, int SIXYIT_WIDTH, int SIXYIT_INT>
void find_G_and_b_matrix(hls::stream< ap_int<9> > &strmIx, hls::stream< ap_int<9> > &strmIy, hls::stream< ap_fixed<IT_WIDTH,IT_INT> > &strmIt,
		hls::stream< ap_fixed<SIXIY_WIDTH,SIXIY_INT> > &sigmaIx2, hls::stream< ap_fixed<SIXIY_WIDTH,SIXIY_INT> > &sigmaIy2, hls::stream< ap_fixed<SIXIY_WIDTH,SIXIY_INT> > &sigmaIxIy,
		hls::stream< ap_fixed<SIXYIT_WIDTH,SIXYIT_INT> > &sigmaIxIt, hls::stream< ap_fixed<SIXYIT_WIDTH,SIXYIT_INT> > &sigmaIyIt, unsigned int rows, unsigned int cols, int level) {
#pragma HLS inline off
//This function does use WINSIZE*WINSIZE adders and multipliers to compute the sum Sigma(Ix*Ix) over WINSIZE*WINSIZE neighborhood
//Instead, it accumulates Ix*Ix in the current iteration, and deletes the top most row's Ix*Ix, adds bottom most row's Ix*Ix, deletes the left most column's Ix*Ix, and adds right most column's Ix*Ix
//This is done to ensure that a 1 pixel per cycle output is achieved with minimal adders and multipliers
	// bufLines is used to buffer Ix, Iy, It
	ap_int<9> bufLines_ix[WINSIZE][MAXWIDTH+(WINSIZE>>1)];
#pragma HLS array_partition variable=bufLines_ix complete dim=1
	ap_int<9> bufLines_iy[WINSIZE][MAXWIDTH+(WINSIZE>>1)];
#pragma HLS array_partition variable=bufLines_iy complete dim=1
	ap_fixed<IT_WIDTH,IT_INT> bufLines_it[WINSIZE][MAXWIDTH+(WINSIZE>>1)];
#pragma HLS array_partition variable=bufLines_it complete dim=1

//The column sums maintain the column sums of IxIx, IyIy, IxIy, IxIt, and IyIt
	ap_fixed<SIXIY_WIDTH,SIXIY_INT>  colsum_IxIx[MAXWIDTH+(WINSIZE>>1)];
	ap_fixed<SIXIY_WIDTH,SIXIY_INT>  colsum_IxIy[MAXWIDTH+(WINSIZE>>1)];
	ap_fixed<SIXIY_WIDTH,SIXIY_INT>  colsum_IyIy[MAXWIDTH+(WINSIZE>>1)];
	ap_fixed<SIXYIT_WIDTH,SIXYIT_INT> colsum_IxIt[MAXWIDTH+(WINSIZE>>1)];
	ap_fixed<SIXYIT_WIDTH,SIXYIT_INT> colsum_IyIt[MAXWIDTH+(WINSIZE>>1)];
#pragma HLS RESOURCE variable=colsum_IxIx core=RAM_T2P_BRAM
#pragma HLS RESOURCE variable=colsum_IxIy core=RAM_T2P_BRAM
#pragma HLS RESOURCE variable=colsum_IyIy core=RAM_T2P_BRAM
#pragma HLS RESOURCE variable=colsum_IxIt core=RAM_T2P_BRAM
#pragma HLS RESOURCE variable=colsum_IyIt core=RAM_T2P_BRAM

//These buffers store the previous column sums to ensure that the number of reads from the colsum buffers do not exceed 2 per cycle
	ap_fixed<SIXIY_WIDTH,SIXIY_INT>  colsum_prevWIN_IxIx[WINSIZE];
	ap_fixed<SIXIY_WIDTH,SIXIY_INT>  colsum_prevWIN_IxIy[WINSIZE];
	ap_fixed<SIXIY_WIDTH,SIXIY_INT>  colsum_prevWIN_IyIy[WINSIZE];
	ap_fixed<SIXYIT_WIDTH,SIXYIT_INT> colsum_prevWIN_IxIt[WINSIZE];
	ap_fixed<SIXYIT_WIDTH,SIXYIT_INT> colsum_prevWIN_IyIt[WINSIZE];
#pragma HLS array_partition variable=colsum_prevWIN_IxIx complete dim=1
#pragma HLS array_partition variable=colsum_prevWIN_IxIy complete dim=1
#pragma HLS array_partition variable=colsum_prevWIN_IyIy complete dim=1
#pragma HLS array_partition variable=colsum_prevWIN_IxIt complete dim=1
#pragma HLS array_partition variable=colsum_prevWIN_IyIt complete dim=1

//as there is accumulation, initialize all the buffers to 0
for(int i=0;i<WINSIZE;i++){
#pragma HLS LOOP_TRIPCOUNT min=1 max=11
	for(int j=0;j<cols+(WINSIZE>>1);j++){
#pragma HLS pipeline ii=1
#pragma HLS LOOP_FLATTEN OFF
#pragma HLS LOOP_TRIPCOUNT min=1 max=1920
		bufLines_ix[i][j] = 0;
		bufLines_iy[i][j] = 0;
		bufLines_it[i][j] = 0;
		if(i == 0)
	    {
			colsum_IxIx[j] = 0;
			colsum_IxIy[j] = 0;
			colsum_IyIy[j] = 0;
			colsum_IxIt[j] = 0;
			colsum_IyIt[j] = 0;
		}
	}
}
	ap_uint<7> lineStore = 0;

#if DEBUG
  char name[200];
  sprintf(name,"sumIxt_hw%d.txt",level);
  FILE *fpixt = fopen(name, "w");
  sprintf(name,"sumIyt_hw%d.txt",level);
  FILE *fpiyt = fopen(name, "w");
  sprintf(name,"sumIx2_hw%d.txt",level);
  FILE *fpix2 = fopen(name, "w");
  sprintf(name,"sumIy2_hw%d.txt",level);
  FILE *fpiy2 = fopen(name, "w");
  sprintf(name,"sumIxy_hw%d.txt",level);
  FILE *fpixy = fopen(name, "w");
#endif

//variables to store the current output sums
	ap_fixed<SIXIY_WIDTH,SIXIY_INT> sumIx2, sumIy2, sumIxIy;
	ap_fixed<SIXYIT_WIDTH,SIXYIT_INT> sumIxIt, sumIyIt;
	for (ap_uint<16> i=0; i<rows+(WINSIZE>>1); i++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAXHEIGHT
		for (ap_uint<16> j=0; j<cols+(WINSIZE>>1); j++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAXWIDTH
#pragma HLS pipeline ii=1
#pragma HLS LOOP_FLATTEN OFF
			//the column loop count and row loop count run for WINSIZE>>1 iterations more
			//this is to ensure that the sum accumulation happens pixel by pixel
			
			//initialize the output to 0 during the first cycle.
			if (j==0) {
				sumIx2 = 0;
				sumIy2 = 0;
				sumIxIy = 0;
				sumIxIt = 0;
				sumIyIt = 0;
			}
			ap_int<9> regIx=0, regIy=0;
			ap_fixed<IT_WIDTH,IT_INT> regIt = 0;
			ap_int<9> top_Ix=0, top_Iy=0;
			ap_fixed<IT_WIDTH,IT_INT> top_It = 0;
			
			ap_fixed<SIXIY_WIDTH,SIXIY_INT> current_ixix=0, current_iyiy=0, current_ixiy=0;
			ap_fixed<SIXYIT_WIDTH,SIXYIT_INT> current_ixit=0, current_iyit=0;
			ap_fixed<SIXIY_WIDTH,SIXIY_INT> leftwin_ixix=0, leftwin_iyiy=0, leftwin_ixiy=0;
			ap_fixed<SIXYIT_WIDTH,SIXYIT_INT> leftwin_ixit=0, leftwin_iyit=0;
			
			//Read Ix Iy and It
			if(j<cols && i<rows){
				regIx = strmIx.read();
				regIy = strmIy.read();
				regIt = strmIt.read();
			}
			else
			{
				regIx = 0;
				regIy = 0;
				regIt = 0;
			}
			
			//read Ix Iy It from the top row, these will be used to update the column sums
			//top row column sum must be subtracted and the bottom row must be added 
			if(j<cols){
				top_Ix = bufLines_ix[0][j];
				top_Iy = bufLines_iy[0][j];
				top_It = bufLines_it[0][j];
			}
			else{
				top_Ix = 0;
				top_Iy = 0;
				top_It = 0;
			}
			//after every pixel read in the current column, shift the entire current column of Ix Iy It up by 1
			for(int shiftuprow=0; shiftuprow < WINSIZE - 1; shiftuprow++)
			{
#pragma HLS UNROLL
				bufLines_ix[shiftuprow][j] = bufLines_ix[shiftuprow+1][j];
				bufLines_iy[shiftuprow][j] = bufLines_iy[shiftuprow+1][j];
				bufLines_it[shiftuprow][j] = bufLines_it[shiftuprow+1][j];
			}
			//store the current read into the last row.
			bufLines_ix[WINSIZE-1][j] = regIx;
			bufLines_iy[WINSIZE-1][j] = regIy;
			bufLines_it[WINSIZE-1][j] = regIt;
			
			//current column sum is the column sum at j + bottom column 
			current_ixix = colsum_IxIx[j] + (regIx*regIx) - (top_Ix*top_Ix);
			current_ixiy = colsum_IxIy[j] + (regIx*regIy) - (top_Ix*top_Iy);
			current_iyiy = colsum_IyIy[j] + (regIy*regIy) - (top_Iy*top_Iy);
			current_ixit = colsum_IxIt[j] + (regIx*regIt) - (top_Ix*top_It);
			current_iyit = colsum_IyIt[j] + (regIy*regIt) - (top_Iy*top_It);
			
			//update the colsums buffer with the current value
			colsum_IxIx[j] = current_ixix;
			colsum_IxIy[j] = current_ixiy;
			colsum_IyIy[j] = current_iyiy;
			colsum_IxIt[j] = current_ixit;
			colsum_IyIt[j] = current_iyit;
			
			//maintain a window of the computed column sums. This is to subtract the left column from the current sum and add the right column
			ap_fixed<SIXIY_WIDTH,SIXIY_INT>  prev_win_ixix=colsum_prevWIN_IxIx[0];
			ap_fixed<SIXIY_WIDTH,SIXIY_INT>  prev_win_iyiy=colsum_prevWIN_IxIy[0];
			ap_fixed<SIXIY_WIDTH,SIXIY_INT>  prev_win_ixiy=colsum_prevWIN_IyIy[0];
			ap_fixed<SIXYIT_WIDTH,SIXYIT_INT> prev_win_ixit=colsum_prevWIN_IxIt[0];
			ap_fixed<SIXYIT_WIDTH,SIXYIT_INT> prev_win_iyit=colsum_prevWIN_IyIt[0];

			//shift the window left
			for(int shiftregwin=0; shiftregwin < WINSIZE - 1; shiftregwin++)
			{
#pragma HLS UNROLL
				colsum_prevWIN_IxIx[shiftregwin] = colsum_prevWIN_IxIx[shiftregwin + 1]; 
				colsum_prevWIN_IxIy[shiftregwin] = colsum_prevWIN_IxIy[shiftregwin + 1]; 
				colsum_prevWIN_IyIy[shiftregwin] = colsum_prevWIN_IyIy[shiftregwin + 1]; 
				colsum_prevWIN_IxIt[shiftregwin] = colsum_prevWIN_IxIt[shiftregwin + 1]; 
				colsum_prevWIN_IyIt[shiftregwin] = colsum_prevWIN_IyIt[shiftregwin + 1];
			}
			//update the last location with the current column sum
			colsum_prevWIN_IxIx[WINSIZE-1] = current_ixix;
			colsum_prevWIN_IxIy[WINSIZE-1] = current_ixiy;
			colsum_prevWIN_IyIy[WINSIZE-1] = current_iyiy;
			colsum_prevWIN_IxIt[WINSIZE-1] = current_ixit;
			colsum_prevWIN_IyIt[WINSIZE-1] = current_iyit;
			
			//The sum computation is complete once the left most value of the window is subtracted and the right most value of the window is added
			if(j >= WINSIZE)
			// if(0)
			{
			    leftwin_ixix = current_ixix - prev_win_ixix;
			    leftwin_ixiy = current_ixiy - prev_win_iyiy;
			    leftwin_iyiy = current_iyiy - prev_win_ixiy;
			    leftwin_ixit = current_ixit - prev_win_ixit;
			    leftwin_iyit = current_iyit - prev_win_iyit;
			}
			else
			{
				leftwin_ixix = current_ixix;
				leftwin_ixiy = current_ixiy;
				leftwin_iyiy = current_iyiy;
				leftwin_ixit = current_ixit;
				leftwin_iyit = current_iyit;
			}
			
			//current windowed sum outputs. sumIx2, Iy2, IxIy, IxIt, and IyIt are accumulated over the column loop
			sumIx2  += leftwin_ixix;
			sumIy2  += leftwin_iyiy;
			sumIxIy += leftwin_ixiy;
			sumIxIt += leftwin_ixit;
			sumIyIt += leftwin_iyit;
			
			// Please note that Ix = dI/dx = (I[row][col+1] - I[row][col-1])/(col+1-(col-1))
			// and, Iy = dI/dy = (I[row+1][col] - I[row-1][col])/(row+1-(row-1))
			//hence, When Sigma Ix*Ix is computed, it must be divided by 4 and IxIt is computed, it is divided by 2.
			ap_fixed<SIXIY_WIDTH,SIXIY_INT> Ix2out   = ap_fixed<SIXIY_WIDTH,SIXIY_INT>(sumIx2>>2);
			ap_fixed<SIXIY_WIDTH,SIXIY_INT> Iy2out   = ap_fixed<SIXIY_WIDTH,SIXIY_INT>(sumIy2>>2); 
			ap_fixed<SIXIY_WIDTH,SIXIY_INT> IxIyout  = ap_fixed<SIXIY_WIDTH,SIXIY_INT>(sumIxIy>>2);
			ap_fixed<SIXYIT_WIDTH,SIXYIT_INT> IxItout = ap_fixed<SIXYIT_WIDTH,SIXIY_INT>(sumIxIt>>1);
			ap_fixed<SIXYIT_WIDTH,SIXYIT_INT> IyItout = ap_fixed<SIXYIT_WIDTH,SIXIY_INT>(sumIyIt>>1);
			
			if (j>=WINSIZE>>1 && i>=WINSIZE>>1) {
				sigmaIx2.write (Ix2out );
				sigmaIy2.write (Iy2out );
				sigmaIxIy.write(IxIyout);
				sigmaIxIt.write(IxItout);
				sigmaIyIt.write(IyItout);
#if DEBUG
	  fprintf(fpixt,"%12.4f ",float(IxItout));
	  fprintf(fpiyt,"%12.4f ",float(IyItout));
	  fprintf(fpix2,"%12.2f ",float(Ix2out));
	  fprintf(fpiy2,"%12.2f ",float(Iy2out));
	  fprintf(fpixy,"%12.2f ",float(IxIyout));
#endif
			}
		} // end j loop
#if DEBUG
	if (i >= WINSIZE>>1){
      fprintf(fpixt,"\n");
      fprintf(fpiyt,"\n");
      fprintf(fpix2,"\n");
      fprintf(fpiy2,"\n");
	  fprintf(fpixy,"\n");
	}
#endif
	}
#if DEBUG
      fclose(fpixt);
      fclose(fpiyt);
      fclose(fpix2);
      fclose(fpiy2);
      fclose(fpixy);
#endif
} // end find_G()
#endif