Xilinx xfOpenCV Library
======================
The xfOpenCV library is a set of 50+ kernels, optimized for Xilinx FPGAs and SoCs, based on the OpenCV computer vision library. The kernels in the xfOpenCV library are optimized and supported in the Xilinx SDx Tool Suite.

## DESIGN FILE HIERARCHY
The library is organized into the following folders -

| Folder Name | Contents |
| :------------- | :------------- |
| examples | Examples that evaluate the xfOpenCV kernels, and demonstrate the kernels' use model in SDSoC flow |
| examples_sdaccel | Two examples that evaluate the xfOpenCV kernels, and demonstrate the kernels' use model in SDAccel flow. These 2 examples serve as reference on how to use all other supported xfOpenCV kernels in SDAccel |
| include | The relevant headers necessary to use the xfOpenCV kernels |

The organization of contents in each folder is described in the readmes of the respective folders.
For more information on the xfOpenCV libraries and their use models, please refer to the [Xilinx OpenCV User Guide][].

## HOW TO DOWNLOAD THE REPOSITORY
To get a local copy of the repository, clone this repository to the local system with the following command:
```
git clone https://github.com/Xilinx/xfopencv xfopencv
```
Where 'xfopencv' is the name of the directory where the repository will be stored on the local system.This command needs to be executed only once to retrieve the latest version of the xfOpenCV library. The only required software is a local installation of git.

## HARDWARE and SOFTWARE REQUIREMENTS
The xfOpenCV library is designed to work with Zynq and Zynq Ultrascale+ FPGAs. The library has been verified on zcu102 and zcu104 boards.
SDSoC 2018.3 Development Environment is required to work with the library.
zcu102 reVISION platform is required to run the library on zcu102 board and zcu104 reVISION platform to run on zcu104 board. Please download it from here: [reVISION Platform]

## OTHER INFORMATION
Full User Guide for xfOpenCV and using OpenCV on Xilinx devices Check here:
[Xilinx OpenCV User Guide][]

For information on getting started with the reVISION stack check here:
[reVISION Getting Started Guide]

For more information about SDSoC check here:
[SDSoC User Guide][]

For more information about SDAccel check here:
[SDAccel User Guide][]

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
June2018  | 4.0            | 2018.2 Xilinx release <br>
December2018  | 5.0            | 2018.3 Xilinx release <br>

## Changelog:
1. Added new functions:

    • Demosaicing

    • Kalman Filter

2. Ported [HLS Video Library][] along with all supporting functions into xfOpenCV. For more details see "Migrating HLS Video Library to xfOpenCV" section of [UG1233][].

3. Added UltraRAM support for the following  functions:

      • Box Filter

      • Canny Edge Detection

      • Demosaicing

      • Dense Pyramidal Optical Flow

      • Harris Corner

      • Histogram Of Gradients (HOG)

      • Kalman Filter

      • Non-Pyramidal Optical Flow

      • Pyramid down

      • Remap

      • Sobel filter

      • StereoLBM

      • Stereo Pipeline

      • Warp Transform



4. A subset of xfOpenCV functions now support SDAccel flow. Added 2 examples as reference and a methodology chapter in [UG1233][].

5. Minor bug fixes.

#### Known Issues:
1. Windows OS has path length limitations, kernel names must be smaller than 25 characters.
2. Missing one command from the list of commands in Chapter 3 "Evaluating the Functionality" section of UG1233. Details [here](https://github.com/Xilinx/xfopencv/blob/master/examples_sdaccel/UG1233_errata.md).


[reVISION Getting Started Guide]: https://github.com/Xilinx/Revision-Getting-Started-Guide
[HLS Video Library]:
https://xilinx-wiki.atlassian.net/wiki/spaces/A/pages/18841665/HLS+Video+Library
[reVISION Platform]: https://github.com/Xilinx/reVISION-Getting-Started-Guide/blob/master/Docs/software-tools-system-requirements.md#32-software
[SDSoC Forums]: https://forums.xilinx.com/t5/SDSoC-Development-Environment/bd-p/sdsoc
[SDSoC User Guide]: https://www.xilinx.com/support/documentation/sw_manuals/xilinx2018_3/ug1027-sdsoc-user-guide.pdf
[3-Clause BSD License]: LICENSE.txt
[Repository Contribution README]: CONTRIBUTING.md
[Xilinx OpenCV User Guide]: https://www.xilinx.com/support/documentation/sw_manuals/xilinx2018_3/ug1233-xilinx-opencv-user-guide.pdf
[UG1233]:
https://www.xilinx.com/support/documentation/sw_manuals/xilinx2018_3/ug1233-xilinx-opencv-user-guide.pdf
[SDAccel User Guide]:
https://www.xilinx.com/support/documentation/sw_manuals/xilinx2018_3/ug1023-sdaccel-user-guide.pdf
