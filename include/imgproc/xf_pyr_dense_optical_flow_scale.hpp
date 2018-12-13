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
#ifndef __XF_PYR_DENSE_OPTICAL_FLOW_SCALE__
#define __XF_PYR_DENSE_OPTICAL_FLOW_SCALE__

template<int MAXWIDTH, int FLOW_WIDTH, int FLOW_INT, int SCCMP_WIDTH, int SCCMP_INT, int SCALE_WIDTH, int SCALE_INT>
void load_data (hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &inStrm0,
                hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &inStrm1,
                ap_fixed<FLOW_WIDTH,FLOW_INT> buf[2][MAXWIDTH], int rows, int cols, bool &flagLoaded, int i, ap_ufixed<SCALE_WIDTH,SCALE_INT> scaleI, ap_fixed<SCCMP_WIDTH,SCCMP_INT> &fracI, int &prevIceil) {
#pragma HLS inline off
	ap_fixed<SCCMP_WIDTH,SCCMP_INT> iSmall = i * scaleI;
	int iSmallFloor = (int) iSmall;
	fracI = iSmall - (ap_fixed<SCCMP_WIDTH,SCCMP_INT>)iSmallFloor;
	if ((iSmallFloor + 1 > prevIceil || i<2) && (iSmallFloor < rows-1)) {
		flagLoaded = 1;
		for (int i=0; i<cols; i++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAXWIDTH
#pragma HLS pipeline ii=1
#pragma HLS LOOP_FLATTEN OFF
			buf[0][i] = inStrm0.read();
			buf[1][i] = inStrm1.read();
		}
		prevIceil = iSmallFloor + 1;
	}
	else {
		flagLoaded = 0;
	}
} // end load_data()

template< int FLOW_WIDTH,int FLOW_INT, int SCCMP_WIDTH, int SCCMP_INT, int RMAPPX_WIDTH, int RMAPPX_INT>
ap_fixed<FLOW_WIDTH,FLOW_INT> compute_result(ap_fixed<SCCMP_WIDTH,SCCMP_INT> fracI, ap_fixed<SCCMP_WIDTH,SCCMP_INT> fracJ, ap_fixed<FLOW_WIDTH,FLOW_INT> i0, ap_fixed<FLOW_WIDTH,FLOW_INT> i1, ap_fixed<FLOW_WIDTH,FLOW_INT> i2, ap_fixed<FLOW_WIDTH,FLOW_INT> i3) {
#pragma HLS inline off
	ap_fixed<18,1> fi = (fracI);
	ap_fixed<18,1> fj = (fracJ);
	ap_fixed<36,1> fij = (ap_fixed<36,1>)fi * (ap_fixed<36,1>)fj;

	ap_fixed<18,1> p3 = (ap_fixed<18,1>) fij;
	ap_fixed<18,1> p2 = (ap_fixed<18,1>)((ap_fixed<36,1>) fi - fij);
	ap_fixed<18,1> p1 = (ap_fixed<18,1>)((ap_fixed<36,1>) fj - fij);
	ap_fixed<21,4> p0 = ap_fixed<21,4>(1.0) - ap_fixed<21,4>(p1) - ap_fixed<21,4>(p2) - ap_fixed<21,4>(p3);
	ap_fixed<FLOW_WIDTH+2,FLOW_INT+2> resIf =  (ap_fixed<FLOW_WIDTH+2,FLOW_INT+2>)i0*p0 
										     + (ap_fixed<FLOW_WIDTH+2,FLOW_INT+2>)i1*p1
										     + (ap_fixed<FLOW_WIDTH+2,FLOW_INT+2>)i2*p2
										     + (ap_fixed<FLOW_WIDTH+2,FLOW_INT+2>)i3*p3;
	return (ap_fixed<FLOW_WIDTH,FLOW_INT>)resIf;
} // end compute_result()

template<unsigned short MAXHEIGHT, unsigned short MAXWIDTH, int FLOW_WIDTH, int FLOW_INT, int SCCMP_WIDTH, int SCCMP_INT, int RMAPPX_WIDTH, int RMAPPX_INT, int SCALE_WIDTH, int SCALE_INT>
void process(ap_fixed<FLOW_WIDTH,FLOW_INT> buf[2][MAXWIDTH], ap_fixed<FLOW_WIDTH,FLOW_INT> buffer[2][2][MAXWIDTH], unsigned short int outRows, unsigned short int outCols,
             hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> >& outStrm0,
             hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> >& outStrm1,
             bool flagLoaded, int row, ap_ufixed<SCALE_WIDTH,SCALE_INT> scaleI, ap_ufixed<SCALE_WIDTH,SCALE_INT> scaleJ, ap_fixed<SCCMP_WIDTH,SCCMP_INT> fracI, int mul) {
#pragma HLS inline off
	int bufCount = 0;
	ap_fixed<FLOW_WIDTH,FLOW_INT> regLoad;
	int prevJceil = -1;
	ap_fixed<FLOW_WIDTH,FLOW_INT> i0[2]={0,0};
	ap_fixed<FLOW_WIDTH,FLOW_INT> i1[2]={0,0};
	ap_fixed<FLOW_WIDTH,FLOW_INT> i2[2]={0,0};
	ap_fixed<FLOW_WIDTH,FLOW_INT> i3[2]={0,0};
	L3:for (ap_uint<16> j=0; j<outCols; j++) {
#pragma HLS LOOP_TRIPCOUNT min=1 max=MAXWIDTH
#pragma HLS pipeline
#pragma HLS LOOP_FLATTEN OFF			
#pragma HLS DEPENDENCE variable=buffer array inter false
		ap_fixed<SCCMP_WIDTH,SCCMP_INT> jSmall = j * scaleJ;
		int jSmallFloor = (int) jSmall;
		ap_fixed<SCCMP_WIDTH,SCCMP_INT> iSmall = row * scaleI;
		int iSmallFloor = (int) iSmall;
		ap_fixed<SCCMP_WIDTH,SCCMP_INT> fracI = iSmall - (ap_fixed<SCCMP_WIDTH,SCCMP_INT>)iSmallFloor;
		ap_fixed<SCCMP_WIDTH,SCCMP_INT> fracJ = jSmall - (ap_fixed<SCCMP_WIDTH,SCCMP_INT>)jSmallFloor;

		if (row == 0) {
			fracI = 1;
			if (j==0) {
              for (int k=0; k<2; k++) {
                ap_fixed<FLOW_WIDTH,FLOW_INT> reg = buf[k][bufCount];
                buffer[k][1][bufCount] = reg;
                i3[k] = reg;
              }
				fracI = 1; fracJ = 1;
				bufCount++;
				prevJceil = 0;
			}
			else if (j<outCols) {
				if (prevJceil == jSmallFloor) {
                  for (int k=0; k<2; k++) {
					i2[k] = i3[k];
					ap_fixed<FLOW_WIDTH,FLOW_INT> reg = buf[k][bufCount];
					buffer[k][1][bufCount] = reg;
					i3[k] = reg;
                  }
					bufCount++;
					prevJceil = jSmallFloor + 1;
				}
			}
			else {
				i3[0] = buffer[0][1][bufCount-1];
				i3[1] = buffer[1][1][bufCount-1];
				fracI = 1; fracJ = 1;
			}
		}
		else if (row < outRows-1) {
			if (j==0) {
				i0[0] = 0; i2[0] = 0;
				i0[1] = 0; i2[1] = 0;
				fracJ = 1;
				if (flagLoaded) {
                  for (int k=0; k<2; k++) {
					ap_fixed<FLOW_WIDTH,FLOW_INT> reg = buf[k][bufCount];
					ap_fixed<FLOW_WIDTH,FLOW_INT> tmp = buffer[k][1][bufCount];
					buffer[k][0][bufCount] =  tmp;
					i1[k] = tmp;
					buffer[k][1][bufCount] = reg;
					i3[k] = reg;
                  }
					bufCount++;
				}
				else {
                  for (int k=0; k<2; k++) {
					i1[k] = buffer[k][0][bufCount];
					i3[k] = buffer[k][1][bufCount];
                  }
					bufCount++;
				}
				prevJceil = 0;
			}
			else if (j < outCols) {
				if (prevJceil == jSmallFloor) {
					i0[0] = i1[0]; i2[0] = i3[0];
					i0[1] = i1[1]; i2[1] = i3[1];
					if (flagLoaded) {		
                      for (int k=0; k<2; k++) {
						ap_fixed<FLOW_WIDTH,FLOW_INT> reg = buf[k][bufCount];
						ap_fixed<FLOW_WIDTH,FLOW_INT> tmp = buffer[k][1][bufCount];
						buffer[k][0][bufCount] =  tmp;
						i1[k] = tmp;
						buffer[k][1][bufCount] = reg;
						i3[k] = reg;
                      }
						bufCount++;
					}
					else {
                      for (int k=0; k<2; k++) {
						i1[k] = buffer[k][0][bufCount];
						i3[k] = buffer[k][1][bufCount];
                      }
						bufCount++;
					}
					prevJceil = jSmallFloor + 1;
				}
			}
			else {
				fracJ = 1;
			}
		}
		else {
			if (j==0) {
				i3[0] = buffer[0][1][bufCount];
				i3[1] = buffer[1][1][bufCount];
				fracI = 1; fracJ = 1;
				bufCount++;
				prevJceil = 0;
			}
			else if (j < outCols) {
				if (prevJceil == jSmallFloor) {
                  for (int k=0; k<2; k++) {
					i2[k] = i3[k];
					ap_fixed<FLOW_WIDTH,FLOW_INT> reg = buffer[k][1][bufCount];
					i3[k] = reg;
                  }
					bufCount++;
					prevJceil = jSmallFloor + 1;
				}
				fracI = 1;
			}
			else { 
				i3[0] = buffer[0][1][bufCount-1];
				i3[1] = buffer[1][1][bufCount-1];
				fracI = 1; fracJ = 1;
			}

		} // end else
		ap_fixed<FLOW_WIDTH,FLOW_INT> resIf0 = compute_result <FLOW_WIDTH, FLOW_INT, SCCMP_WIDTH, SCCMP_INT, RMAPPX_WIDTH, RMAPPX_INT>(fracI, fracJ, i0[0], i1[0], i2[0], i3[0]);
		outStrm0.write(resIf0<<1);
		ap_fixed<FLOW_WIDTH,FLOW_INT> resIf1 = compute_result <FLOW_WIDTH, FLOW_INT, SCCMP_WIDTH, SCCMP_INT, RMAPPX_WIDTH, RMAPPX_INT>(fracI, fracJ, i0[1], i1[1], i2[1], i3[1]);
		outStrm1.write(resIf1<<1);

	} // end L3
} // end process()
template<unsigned short MAXHEIGHT, unsigned short MAXWIDTH, int FLOW_WIDTH, int FLOW_INT, int SCCMP_WIDTH, int SCCMP_INT, int RMAPPX_WIDTH, int RMAPPX_INT, int SCALE_WIDTH, int SCALE_INT, bool USE_URAM>
void scale_up( hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &inStrm0, hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &outStrm0,
               hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &inStrm1, hls::stream< ap_fixed<FLOW_WIDTH,FLOW_INT> > &outStrm1,
			unsigned short int inRows, unsigned short int inCols, unsigned short int outRows, unsigned short int outCols, int mul, const bool scale_up_flag, float scale_comp) {
#pragma HLS inline off

	ap_fixed<FLOW_WIDTH,FLOW_INT> buffer[2][2][MAXWIDTH];
#pragma HLS array_reshape variable=buffer dim=1 complete
#pragma HLS array_reshape variable=buffer dim=2 complete
	ap_fixed<FLOW_WIDTH,FLOW_INT> buf0[2][MAXWIDTH], buf1[2][MAXWIDTH];
#pragma HLS array_reshape variable=buf0 dim=1 complete
#pragma HLS array_reshape variable=buf1 dim=1 complete
if (USE_URAM) {	
//#pragma HLS RESOURCE variable=buffer core=XPM_MEMORY uram
//#pragma HLS RESOURCE variable=buf0   core=XPM_MEMORY uram
//#pragma HLS RESOURCE variable=buf1   core=XPM_MEMORY uram
#pragma HLS RESOURCE variable=buffer core=RAM_S2P_URAM
#pragma HLS RESOURCE variable=buf0   core=RAM_S2P_URAM
#pragma HLS RESOURCE variable=buf1   core=RAM_S2P_URAM
}
	
	ap_ufixed<SCALE_WIDTH,SCALE_INT> scaleI = (ap_ufixed<SCALE_WIDTH,SCALE_INT>)scale_comp;
	ap_ufixed<SCALE_WIDTH,SCALE_INT> scaleJ = (ap_ufixed<SCALE_WIDTH,SCALE_INT>)scale_comp;
	#if DEBUG
		cout << "Scale Flag: " << scale_up_flag << "\n";
		cout << "Scale Comp: " << scale_comp << "\n";
		cout << "Scale: " << float(scaleJ) << " " << float(scaleI) << "\n";
	#endif
	ap_fixed<SCCMP_WIDTH,SCCMP_INT> fracI0, fracI1;

	bool flagLoaded0, flagLoaded1;
	bool flag = 0;
	if(scale_up_flag==0){
		for (ap_uint<16> i=0; i<outRows; i++) {
	#pragma HLS LOOP_TRIPCOUNT min=1 max=MAXHEIGHT
			for (ap_uint<16> j=0; j<outCols; j++) {
	#pragma HLS LOOP_TRIPCOUNT min=1 max=MAXWIDTH
	#pragma HLS pipeline II=1
	#pragma HLS LOOP_FLATTEN OFF
				outStrm0.write((ap_fixed<FLOW_WIDTH,FLOW_INT>)inStrm0.read());
				outStrm1.write((ap_fixed<FLOW_WIDTH,FLOW_INT>)inStrm1.read());
			}
		}
	}
	else{
		int prevIceil = -1;
		load_data<MAXWIDTH, FLOW_WIDTH, FLOW_INT, SCCMP_WIDTH, SCCMP_INT, SCALE_WIDTH, SCALE_INT>(inStrm0, inStrm1, buf0, inRows, inCols, flagLoaded0, 0, scaleI, fracI0, prevIceil);
		L2:for (ap_uint<16> i=0; i<outRows-1; i++) {
	#pragma HLS LOOP_TRIPCOUNT min=1 max=MAXHEIGHT
			if (flag==0) {
				load_data<MAXWIDTH, FLOW_WIDTH, FLOW_INT, SCCMP_WIDTH, SCCMP_INT, SCALE_WIDTH, SCALE_INT>(inStrm0, inStrm1, buf1, inRows, inCols, flagLoaded1, i+1, scaleI, fracI1, prevIceil);
				process<MAXHEIGHT, MAXWIDTH, FLOW_WIDTH, FLOW_INT, SCCMP_WIDTH, SCCMP_INT, RMAPPX_WIDTH, RMAPPX_INT, SCALE_WIDTH, SCALE_INT>(buf0, buffer, outRows, outCols, outStrm0, outStrm1, flagLoaded0, i, scaleI, scaleJ, fracI0, mul);
				flag = 1;
			}
			else {
				load_data<MAXWIDTH, FLOW_WIDTH, FLOW_INT, SCCMP_WIDTH, SCCMP_INT, SCALE_WIDTH, SCALE_INT>(inStrm0, inStrm1, buf0, inRows, inCols, flagLoaded0, i+1, scaleI, fracI0, prevIceil);
				process<MAXHEIGHT, MAXWIDTH, FLOW_WIDTH, FLOW_INT, SCCMP_WIDTH, SCCMP_INT, RMAPPX_WIDTH, RMAPPX_INT, SCALE_WIDTH, SCALE_INT>(buf1, buffer, outRows, outCols, outStrm0, outStrm1, flagLoaded1, i, scaleI, scaleJ, fracI1, mul);
				flag = 0;
			}
		} // end L2

		if (flag ==0) {
			process<MAXHEIGHT, MAXWIDTH, FLOW_WIDTH, FLOW_INT, SCCMP_WIDTH, SCCMP_INT, RMAPPX_WIDTH, RMAPPX_INT>(buf0, buffer, outRows, outCols, outStrm0, outStrm1, flagLoaded0, outRows-1, scaleI, scaleJ, fracI0, mul);
		}
		else {
			process<MAXHEIGHT, MAXWIDTH, FLOW_WIDTH, FLOW_INT, SCCMP_WIDTH, SCCMP_INT, RMAPPX_WIDTH, RMAPPX_INT>(buf1, buffer, outRows, outCols, outStrm0, outStrm1, flagLoaded1, outRows-1, scaleI, scaleJ, fracI1, mul);
		}
	}

} // end scale_up

#endif