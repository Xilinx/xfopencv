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
#ifndef __XF_PYR_DENSE_OPTICAL_FLOW_FIND_GRADIENTS__
#define __XF_PYR_DENSE_OPTICAL_FLOW_FIND_GRADIENTS__

template<unsigned short MAXHEIGHT, unsigned short MAXWIDTH, int NUM_LINES, int IT_WIDTH, int IT_INT, int ITCMP_WIDTH, int ITCMP_INT, int FLOW_WIDTH, int FLOW_INT, int RMAPPX_WIDTH, int RMAPPX_INT>
ap_fixed<IT_WIDTH,IT_INT> findIntensity(unsigned char lineBuffer[NUM_LINES+1][MAXWIDTH],
		ap_uint<11> i, ap_uint<11> j, ap_fixed<FLOW_WIDTH,FLOW_INT> u, ap_fixed<FLOW_WIDTH,FLOW_INT> v, int totalLinesInBuffer, unsigned char effBufferedLines, int lineStore, unsigned int rows, unsigned int cols, unsigned char *fi0, unsigned char *fi1, unsigned char *fi2, unsigned char *fi3) {
	
	//remapped indices in fixed point
	ap_fixed<ITCMP_WIDTH,ITCMP_INT> tmp_locj = u + j;
	ap_fixed<ITCMP_WIDTH,ITCMP_INT> tmp_loci = v + i;

	ap_fixed<ITCMP_WIDTH,ITCMP_INT> locj = tmp_locj;
	ap_fixed<ITCMP_WIDTH,ITCMP_INT> loci = tmp_loci;
	//check if the remapped location lies in the image boundaries and in the number of lines stored in the buffer
	//if not, output the current pixel value.
	if (tmp_loci < 0 || (tmp_loci) > rows-1 || (tmp_locj) < 0 || (tmp_locj) > cols-1 || (v) <(-effBufferedLines/2) || (v)>(effBufferedLines/2)) {
		int temp = lineStore - totalLinesInBuffer/2;
		if (temp > effBufferedLines)
			temp -= (totalLinesInBuffer);
		else if (temp<0)
			temp = (effBufferedLines+temp);

		return lineBuffer[temp][j];
	}
	else {	
		// Finding which line of the linebuffer to read from
		ap_fixed<ITCMP_WIDTH,ITCMP_INT> b = ((lineStore) - (totalLinesInBuffer)/2) + v;
		//integer value of the line buffer index
		int b0 = b;
		//fraction value for bilinear interpolation 
		ap_fixed<ITCMP_WIDTH,ITCMP_INT> fracy = ap_fixed<ITCMP_WIDTH,ITCMP_INT>(b - b0);
		if (b<0) {
			fracy = 1 + fracy;
		}

		//wrap the b0 value around the buffer to ensure that b0 lies between 0 and totalLinesInBuffer
		if (b0 > effBufferedLines)
			b0 -= (totalLinesInBuffer);
		else if (b<0)
			b0 = (effBufferedLines+b0);

		
		//b1 is the next row needed to interpolate for the current output pixel
		int b1 = b0 + 1;
		if (b1 > effBufferedLines)
			b1 -= (totalLinesInBuffer);
		else if (b1<0)
			b1 = (effBufferedLines+b1);

		// Find which column location in the line buffers to access
		int lx0 = tmp_locj;
		int lx1 = lx0 + 1;
		
		//compute the fractional interpolation component in the x direction
		ap_fixed<ITCMP_WIDTH,ITCMP_INT> fracx = ap_fixed<ITCMP_WIDTH,ITCMP_INT>(tmp_locj - lx0);
		ap_uint<8> i0 = 0; 
		ap_uint<8> i1 = 0; 
		ap_uint<8> i2 = 0; 
		ap_uint<8> i3 = 0; 
		//read pixels from the line buffers
		i0 = lineBuffer[b0][lx0];
		i1 = lineBuffer[b0][lx1];
		i2 = lineBuffer[b1][lx0];
		i3 = lineBuffer[b1][lx1];
		*fi0 = i0;
		*fi1 = i1;
		*fi2 = i2;
		*fi3 = i3;
		//bilinear interpolation
		ap_fixed<ITCMP_WIDTH,ITCMP_INT> fracx_fx = fracx;
		ap_fixed<ITCMP_WIDTH,ITCMP_INT> fracy_fx = fracy;  
		// ap_fixed<RMAPPX_WIDTH,RMAPPX_INT> frac_mul = fracx * fracy;
		ap_fixed<ITCMP_WIDTH,ITCMP_INT> tempmul = 1;
		ap_fixed<RMAPPX_WIDTH,RMAPPX_INT> resf_fx = (tempmul -fracx_fx -fracy_fx + fracx_fx*fracy_fx)*i0 + (fracx_fx - fracx_fx*fracy_fx)*i1 + (fracy_fx - fracx_fx*fracy_fx)*i2 + fracx_fx*fracy_fx*i3;
		ap_fixed<IT_WIDTH,IT_INT> resit = resf_fx;
		return resit;
	}
	
} // end findIntensity()

template<unsigned short MAXHEIGHT, unsigned short MAXWIDTH, int NUM_PYR_LEVELS, int NUM_LINES, int WINSIZE, int IT_WIDTH, int IT_INT, int ITCMP_WIDTH, int ITCMP_INT, int FLOW_WIDTH, int FLOW_INT, int RMAPPX_WIDTH, int RMAPPX_INT>
void findGradients(unsigned char *currImg3, unsigned char *nextImg, hls::stream< ap_fixed<IT_WIDTH,IT_INT> > &strmIt, hls::stream< ap_int<9> > &strmIx, hls::stream< ap_int<9> > &strmIy,
		unsigned int rows, unsigned int cols, hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &strmFlowUin, hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &strmFlowVin,
		hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &strmFlowU_in1, hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &strmFlowV_in1, int level) {
#pragma HLS inline off
#if DEBUG
char name[200];
sprintf(name,"It_hw%d.txt",level);
    FILE *fpit = fopen(name, "w");
sprintf(name,"idxx_hw%d.txt",level);
    FILE *fpidxx = fopen(name, "w");
sprintf(name,"idxy_hw%d.txt",level);
    FILE *fpidxy = fopen(name, "w");
sprintf(name,"It_i0_hw%d.txt",level);
    FILE *fpiti0 = fopen(name, "w");
sprintf(name,"It_i1_hw%d.txt",level);
    FILE *fpiti1 = fopen(name, "w");
sprintf(name,"It_i2_hw%d.txt",level);
    FILE *fpiti2 = fopen(name, "w");
sprintf(name,"It_i3_hw%d.txt",level);
    FILE *fpiti3 = fopen(name, "w");
sprintf(name,"gx_hw%d.txt",level);
	FILE *fpgx = fopen(name, "w");
sprintf(name,"gy_hw%d.txt",level);
	FILE *fpgy = fopen(name, "w");
#endif

	unsigned char prev = 0;
	unsigned char curr = 0;
	ap_int<9> gradx = 0;
	ap_int<9> grad = 0;
	unsigned int read_curimg = 0;
	unsigned int read_nxtimg = 0;
	
	//line buffer to store NUM_LINES number of rows in the input image to remap the next image as per the previously computed flow vector
	unsigned char lineBuffer[NUM_LINES+1][MAXWIDTH];
#pragma HLS array_partition variable=lineBuffer complete dim=1
	
	//line buffer to 2 rows to compute Iy
	unsigned char curr_img_buf[2][MAXWIDTH];
#pragma HLS array_partition variable=curr_img_buf complete dim=1

//Compute effective number of lines stored in the line buffer for the remap module. It is at max NUM_LINES or it is (2^(number of pyramid levels -1 - current level))*(WINDOW_SIZE - 1) + 1
//lowest image pyramid level, say the window size is NxN
//next highest image pyramid level, 2N*2N
//However, as the number of lines are limited to NUM_LINES that is the maximum number of rows stored
	unsigned char effBufferedLines = std::min(NUM_LINES,(1<<(NUM_PYR_LEVELS - 1 - level))*(WINSIZE-1) + 1); /**** Change this appropriately in original function***/
	ap_uint<8> totalLinesInBuffer = effBufferedLines + 1;

	//remapping next image, as per the previously computed flow vectors, starts after the number of rows in the buffer are totalLinesInBuffer/2
	int lineStore = totalLinesInBuffer/2;
	L3:for (ap_uint<16> I=0; I<(rows+totalLinesInBuffer/2); I++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAXHEIGHT // 1080 + 11-81
		L4:for (ap_uint<16> j=0; j<cols+1; j++) {
			//column loop runs on cols+1 as Ix needs one more columns to process
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAXWIDTH
#pragma HLS pipeline ii=1
#pragma HLS LOOP_FLATTEN OFF
#pragma HLS dependence array intra false
// #pragma HLS loop_flatten off
			//During the time when the first half of the buffer is read with the next image pixels, no processing takes place
			//The first row of the current image is read for finding Iy. This is read into the second row of the current image buffer
			if(I < totalLinesInBuffer/2)
			{
				if(j<cols)
				{
					if(I == 0)
					{
						curr_img_buf[1][j] =  *(currImg3 + read_curimg);
						read_curimg++;
						// curr_img_buf[1][j] =  *(currImg3 + I*cols + j);
					}
					lineBuffer[I][j] = *(nextImg + read_nxtimg);
					read_nxtimg++;
					// lineBuffer[I][j] = *(nextImg + I*cols + j);
				}
			}
			else
			{
				//i is the current output index
				ap_uint<11> i= I - totalLinesInBuffer/2;
				if(j<cols)
				{
					//read the next image and buffer it circularly. Already totalLinesInBuffer/2 are read before the processing starts
					if (i < rows - totalLinesInBuffer/2) {
						lineBuffer[lineStore][j] = *(nextImg + read_nxtimg);
						read_nxtimg++;
						// lineBuffer[lineStore][j] = *(nextImg + (i+(totalLinesInBuffer/2))*cols + j);
					}
					
					//read the flow vectors. Remap the next image as per these flow vectors 
					ap_fixed<FLOW_WIDTH,FLOW_INT> u = strmFlowUin.read();
					ap_fixed<FLOW_WIDTH,FLOW_INT> v = strmFlowVin.read();
					
					//write into a new stream as the flow vectors are needed in the following flow computation module
					strmFlowU_in1.write(u);
					strmFlowV_in1.write(v);
					// Figure out the intensity to be subtracted using (u,v) on lineBuffer
					unsigned char i0=0;
					unsigned char i1=0;
					unsigned char i2=0;
					unsigned char i3=0;
					//find the remapped pixel value of the next image
					ap_fixed<IT_WIDTH,IT_INT> nextI = findIntensity<MAXHEIGHT, MAXWIDTH, NUM_LINES, IT_WIDTH, IT_INT, ITCMP_WIDTH, ITCMP_INT, FLOW_WIDTH, FLOW_INT, RMAPPX_WIDTH, RMAPPX_INT>(lineBuffer, i, j, u, v, totalLinesInBuffer, effBufferedLines, lineStore, rows, cols,&i0,&i1,&i2,&i3);
					
					unsigned char reg = 0;
					unsigned char curr_px = 0;
					ap_uint<8> regSub = 0;
					ap_uint<8> prevreg = 0;
						//compute Ix and Iy. Ix computation is same regardless of the row value
						if (i==0) {
							//When the first row is being processed, read te next row of the current image
							reg = *(currImg3 + read_curimg);
							read_curimg++;
							// reg = *(currImg3 + (i+1)*cols + j);
							regSub = curr_img_buf[1][j];
							
							curr_img_buf[1][j]  = reg;
							curr_img_buf[0][j] = regSub;
							//store the buffer [1][j] in buffer [0][j] and load the current value to buffer [1][j]
							curr_px = regSub;
							
							//Iy is current value - previous value for the first row
							grad = reg - curr_px;
							strmIy.write(grad);
							#if DEBUG
									   fprintf(fpgy,"%d ",short(grad));
							#endif
							//For Ix
							//when the column number is 0, load previous pixel variable with the current pixel
							if (j==0) {
								prev = curr_px;
							} // end if
							// for the first column output, Ix is [0][1] - [0][0]
							else if (j==1) {
								gradx = curr_px - prev;
								curr = curr_px;
								strmIx.write(gradx);
								#if DEBUG
										   fprintf(fpgx,"%d ",short(gradx));
								#endif
							} // end else-if
							//for all the other column numbers, Ix is computed as current read pixel - previous pixel variable
							// previous pixel stores the previous current pixel and the current pixel variable is updated with the currently read pixel
							else {
								gradx = curr_px - prev;
								prev = curr;
								curr = curr_px;
								strmIx.write(gradx);
							#if DEBUG
									   fprintf(fpgx,"%d ",short(gradx));
							#endif
							} // end else
						
					
						} // end else-if
						//For the last row, Iy is computed as the previous row - current row
						else if (i==rows-1) {
							regSub = curr_img_buf[1][j];
							grad = regSub - curr_img_buf[0][j];
							strmIy.write(grad);
							#if DEBUG
									   fprintf(fpgy,"%d ",short(grad));
							#endif
							curr_px = regSub;					
										
							
							if (j==0) {
								prev = curr_px;
							} // end if
							else if (j==1) {
								gradx = curr_px - prev;
								curr = curr_px;
								strmIx.write(gradx);
								#if DEBUG
										   fprintf(fpgx,"%d ",short(gradx));
								#endif
							} // end else-if
							else {
								gradx = curr_px - prev;
								prev = curr;
								curr = curr_px;
								strmIx.write(gradx);
								#if DEBUG
										   fprintf(fpgx,"%d ",short(gradx));
								#endif
							} // end else
						} // end else-if
						//for all the other row numbers,
						// buffer [0][j] is loaded with the pixel value in buffer[1][j]
						// buffer [1][j] is loaded with current pixel read
						//Iy is computed as current pixel read - buffer [0][j] before it is updated with the pixel in buffer [1][j]
						else {
							reg = *(currImg3 + read_curimg);
							read_curimg++;
							// reg = *(currImg3 + (i+1)*cols + j);
							unsigned char regSub0 = curr_img_buf[0][j];
							regSub  = curr_img_buf[1][j];
							curr_img_buf[0][j] = regSub;
							curr_img_buf[1][j] = reg;
							
							grad = reg - regSub0;
							strmIy.write(grad);
							#if DEBUG
									   fprintf(fpgy,"%d ",short(grad));
							#endif
							curr_px = regSub;
							
							if (j==0) {
								prev = curr_px;
							} // end if
							else if (j==1) {
								gradx = curr_px - prev;
								curr = curr_px;
								strmIx.write(gradx);
								#if DEBUG
										   fprintf(fpgx,"%d ",short(gradx));
								#endif
							} // end else-if
							else {
								gradx = curr_px - prev;
								prev = curr;
								curr = curr_px;
								strmIx.write(gradx);
								#if DEBUG
										   fprintf(fpgx,"%d ",short(gradx));
								#endif
							} // end else
							
							
							
						} // end else
					//temporal gradient It is computed as the difference between current pixel and the previous pixel
					ap_fixed<IT_WIDTH,IT_INT> regIT = curr_px - nextI;
					strmIt.write(regIT);
					
					#if DEBUG
						  fprintf(fpit,"%12.2f ",float(regIT));
						  fprintf(fpidxx,"%12.2f ",float(u));
						  fprintf(fpidxy,"%12.2f ",float(v));
						  fprintf(fpiti0,"%12.2f ",float(i0));
						  fprintf(fpiti1,"%12.2f ",float(i1));
						  fprintf(fpiti2,"%12.2f ",float(i2));
						  fprintf(fpiti3,"%12.2f ",float(i3));
					#endif
					
					if (j== cols-1) {
						//line store is incremented up to effBufferedLines after every (wrapped to 0 if it exceeds) after every row is read/processed.
						lineStore++;
						if (lineStore > effBufferedLines)
							lineStore = 0;
					} // end if j==cols-1
				}
				else
				{
					gradx = curr - prev;
					strmIx.write(gradx);
					#if DEBUG
						fprintf(fpgx,"%d ",short(gradx));
					#endif
				}
			}
		} // end j loop
#if DEBUG
	if(I >= totalLinesInBuffer/2)
	{
		fprintf(fpit,"\n");
		fprintf(fpidxx,"\n");
		fprintf(fpidxy,"\n");
		fprintf(fpiti0,"\n");
		fprintf(fpiti1,"\n");
		fprintf(fpiti2,"\n");
		fprintf(fpiti3,"\n");
	}
    fprintf(fpgx,"\n");
    fprintf(fpgy,"\n");
#endif
	} //end i loop
	
#if DEBUG
   fclose(fpit);
   fclose(fpidxx);
   fclose(fpidxy);
   fclose(fpiti0);
   fclose(fpiti1);
   fclose(fpiti2);
   fclose(fpiti3);
   fclose(fpgx);
   fclose(fpgy);
#endif
} // end find_It()

#endif
