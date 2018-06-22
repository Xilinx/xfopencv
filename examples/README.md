# xfOpenCV/examples
Each of the folders inside examples folder aims to evaluate at least one of the xfOpenCV kernels.

Each example folder consists of a data folder, which contains the corresponding test images. Additionally, each example contains the following files -

| File Name | Description |
| :------------- | :------------- |
| xf_headers.h | Contains the headers required for host code (the code that runs on ARM) |
| xf_<example_name>_config.h, xf_config_params.h | Contains the hardware kernel configuration information and includes the kernel headers |
| xf_<example_name>_tb.cpp | Contains the main() function and evaluation code for each of the xfOpenCV kernels |
| xf_<example_name>_accel.cpp | Contains the function call for the specific xfOpenCV kernel |
| description_zcu102.json | Contains the project configuration information for the SDx GUI for zcu102 platform |
| description_zcu104.json | Contains the project configuration information for the SDx GUI for zcu104 platform |
| Makefile | Makefile to build the example in commandline |


The following table lists which xfOpenCV kernel(s) each example aims to evaluate -

| Example | Function Name |
| :------------- | :------------- |
| accumulate | accumulate |
| accumulatesquared | accumulateSquare |
| accumulateweighted | accumulateWeighted |
| arithm | absdiff, add, subtract, bitwise_and, bitwise_or, bitwise_not, bitwise_xor |
| bilateralfilter | bilateralFilter |
| boxfilter | boxFilter |
| canny | Canny, EdgeTracing |
| channelcombine | merge |
| channelextract | extractChannel |
| colordetect | RGB2HSV, colorthresholding, erode, dilate |
| convertbitdepth | convertTo |
| cornertracker | cornerHarris, cornersImgToList, pyrDown, cornerUpdate |
| customconv | filter2D |
| cvtcolor | iyuv2nv12, iyuv2rgba, iyuv2yuv4, nv122iyuv, nv122rgba, nv122yuv4, nv212iyuv, nv212rgba, nv212yuv4, rgba2yuv4, rgba2iyuv, rgba2nv12, rgba2nv21, uyvy2iyuv, uyvy2nv12, uyvy2rgba, yuyv2iyuv, yuyv2nv12, yuyv2rgba |
| dilation | dilate |
| erosion | erode |
| fast | fast |
| gaussiandifference | GaussianBlur, Duplicatemats, delayMat, subtract |
| gaussianfilter | GaussianBlur |
| harris | cornerHarris |
| histogram | calcHist |
| histequialize | equalizeHist |
| hog | HOGDescriptor |
| houghlines | HoughLines |
| integralimg | integral |
| lkdensepyrof | DensePyrOpticalFlow |
| lknpyroflow | DenseNonPyrLKOpticalFlow |
| lut | LUT |
| magnitude | magnitude |
| meanshifttracking | MeanShift |
| meanstddev | meanStdDev |
| medianblur | medianBlur |
| minmaxloc | minMaxLoc |
| otsuthreshold | OtsuThreshold |
| phase | phase |
| pyrdown | pyrDown |
| pyrup | pyrUp |
| remap | remap |
| resize | resize |
| scharrfilter | Scharr |
| sgbm | SemiGlobalBM |
| sobelfilter | Sobel |
| stereopipeline | InitUndistortRectifyMapInverse, remap, StereoBM |
| stereolbm | StereoBM |
| svm | SVM |
| threshold | Threshold |
| warpaffine | warpAffine |
| warpperspective | warpPerspective |
| warptransform | warpTransform |
