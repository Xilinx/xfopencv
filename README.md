Xilinx xfOpenCV Library 
======================
The xfOpenCV library is a set of 50+ kernels, optimized for Xilinx FPGAs and SoCs, based on the OpenCV computer vision library. The kernels in the xfOpenCV library are optimized and supported in the Xilinx SDx Tool Suite. 

## DESIGN FILE HIERARCHY
The library is organized into the following folders - 

| Folder Name | Contents |
| :------------- | :------------- |
| examples | Examples that evaluate the xfOpenCV kernels, and demonstrate the kernels' use model |
| include | The relevant headers necessary to use the xfOpenCV kernels |

The organization of contents in each folder is described in the readmes of the respective folders.
For more information on the xfOpenCV libraries and their use models, please refer to the [Xilinx OpenCV User Guide][].

## HOW TO DOWNLOAD THE REPOSITORY
To get a local copy of the SDAccel example repository, clone this repository to the local system with the following command:
```
git clone https://github.com/Xilinx/xfopencv xfopencv
```
Where 'xfopencv' is the name of the directory where the repository will be stored on the local system.This command needs to be executed only once to retrieve the latest version of the xfOpenCV library. The only required software is a local installation of git.

## HARDWARE and SOFTWARE REQUIREMENTS
The xfOpenCV library is designed to work with Zynq and Zynq Ultrascale+ FPGAs. The library has been verified on zcu102 board.
SDSoC 2017.4 Development Environment is required to work with the library.
zcu102 reVISION platform is required to run the library on zcu102 board. Please download it from here: [reVISION Platform]

## OTHER INFORMATION
Full User Guide for xfOpenCV and usng OpenCV on Xilinx devices Check here: 
[Xilinx OpenCV User Guide][]

For information on getting started with the reVISION stack check here:
[reVISION Getting Started Guide]

For more information about SDSoC check here:
[SDSoC User Guides][]

## SUPPORT
For questions and to get help on this project or your own projects, visit the [SDSoC Forums][].

## LICENSE AND CONTRIBUTING TO THE REPOSITORY
The source for this project is licensed under the [3-Clause BSD License][]

To contribute to this project, follow the guidelines in the [Repository Contribution README][]

## ACKNOWLEDGEMENTS
This library is written by developers at
- [Xilinx](http://www.xilinx.com)

## REVISION HISTORY

Date      | Readme Version | Release Notes
--------  |----------------|-------------------------
June2017  | 1.0            | Initial Xilinx release <br> -Windows OS support is in Beta.
September2017  | 2.0            | 2017.2 Xilinx release <br> 
December2017  | 3.0            | 2017.4 Xilinx release <br>

## Changelog:

1. Added examples that use library functions to demonstrate hardware function pipelining using SDSoC:

    • Corner Tracking Using Sparse Optical Flow
    
    • Color Detection
    
    • Difference of Gaussian Filter 
    
    • Stereo Vision Pipeline

2. Added new functions to be used on PS side:

    • xf::imread
    
    • xf::imwrite
    
    • xf::convertTo

	• xf::absDiff	

3. Added 8-bit output support for Sobel Filter and Scharr Filter.
4. Output changed from list to image for Harris Corner and Fast Corner functions.
5. All encrypted functions made public.
6. Order of template parameters changed for Bilateral, Stereo-Pipeline, Remap, HoG and MST functions.
7. Minor bug fixes

 #### Known Issues:
1. Windows OS has path length limitations, kernel names must be smaller than 25 characters.


[reVISION Getting Started Guide]: http://www.wiki.xilinx.com/reVISION+Getting+Started+Guide
[reVISION Platform]: https://www.xilinx.com/member/forms/download/design-license-xef.html?akdm=1&filename=zcu102-rv-ss-2017-4.zip
[SDSoC Forums]: https://forums.xilinx.com/t5/SDSoC-Development-Environment/bd-p/sdsoc
[SDSoC User Guides]: https://www.xilinx.com/support/documentation/sw_manuals/xilinx2017_4/ug1027-sdsoc-user-guide.pdf
[3-Clause BSD License]: LICENSE.txt
[Repository Contribution README]: CONTRIBUTING.md
[Xilinx OpenCV User Guide]: https://www.xilinx.com/support/documentation/sw_manuals/xilinx2017_4/ug1233-xilinx-opencv-user-guide.pdf
