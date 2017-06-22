# xfOpenCV/examples
Each of the folders inside examples folder aims to evaluate at least one of the xfOpenCV kernels.

Each example folder consists of an include folder, which contains the xfOpenCV header files, and the following files -

| File Name | Description |
| :------------- | :------------- |
| xf_headers.h | Contains the headers required for host code (the code that runs on ARM) |
| xf_<example_name>_config.h, xf_config_params.h | Contains the hardware kernel configuration information and includes the kernel headers |
| xf_<example_name>_tb.cpp | Contains the main() function and evaluation code for each of the xfOpenCV kernels |
| description.json | Contains the project configuration information for the SDx GUI |
| Makefile | Makefile to build the example in commandline |


The following table lists which xfOpenCV kernel(s) each example aims to evaluate -

| Example | Function Name |
| :------------- | :------------- |
| accumulate | xFaccumulate |
| accumulatesquared | xFaccumulateSquare |
| accumulateweighted | xFaccumulateWeighted |
| arithm | xFabsdiff, xFadd, xFsubtract, xFbitwise_and, xFbitwise_or, xFbitwise_not, xFbitwise_xor |
| bilateralfilter | xFBilateralFilter |
| boxfilter | xFboxfilter |
| canny | xFcanny |
| channelcombine | xFmerge |
| channelextract | xFextractChannel |
| convertbitdepth | xFconvertTo |
| customconv | xFfilter2D |
| cvtcolor | xFiyuv2nv12, xFiyuv2rgba, xFiyuv2yuv4, xFnv122iyuv, xFnv122rgba, xFnv122yuv4, xFnv212iyuv, xFnv212rgba, xFnv212yuv4, xFrgba2yuv4, xFrgba2iyuv, xFrgba2nv12, xFrgba2nv21, xFuyvy2iyuv, xFuyvy2nv12, xFuyvy2rgba, xFyuyv2iyuv, xFyuyv2nv12, xFyuyv2rgba |
| dilation | xFdilate |
| erosion | xFerode |
| fast | xFFAST |
| gaussianfilter | xFGaussianBlur |
| harris | xFCornerHarris |
| histogram | xFcalcHist |
| histequialize | xFequalizeHist |
| hog | xFHOGDescriptor |
| integralimg | xFIntegralImage |
| lkdensepyrof | xFDensePyrOpticalFlow |
| lknpyroflow | xFDenseNonPyrLKOpticalFlow |
| lut | xFLUT |
| magnitude | xFmagnitude |
| meanshifttracking | xFMeanShift |
| meanstddev | xFmeanstd |
| medianblur | xFMedianBlur |
| minmaxloc | xFminMaxLoc |
| otsuthreshold | xFOtsuThreshold |
| phase | xFphase |
| pyrdown | xFPyrDown |
| pyrup | xFPyrUp |
| remap | xFRemap |
| resize | xFResize |
| scharrfilter | xFScharr |
| sobelfilter | xFSobel |
| stereopipeline | xFStereoPipeline |
| stereolbm | xFStereoBM |
| svm | xFSVM |
| threshold | xFThreshold |
| warpaffine | xFwarpAffine |
| warpperspective | xFperspective |
| warptransform | xFWarpTransform |
