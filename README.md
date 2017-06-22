Xilinx xfOpenCV Library 
======================
The xfOpenCV library is a set of 50+ kernels, optimized for Zynq and Zynq Ultrascale+ FPGAs, from the OpenCV computer vision library. The kernels in the xfOpenCV library are optimized and supported in the Xilinx SDx Tool Suite. 

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

## SOFTWARE AND SYSTEM REQUIREMENTS
Board | Software Version |
------|----------------- |
ZCU102 | SDx 2017.1 |
ZC706 | SDx 2017.1 |
ZC702 | SDx 2017.1 |

## OTHER INFORMATION
Full User Guide for xfOpenCV and usng OpenCV on Xilinx devices Check here: 
[Xilinx OpenCV User Guide][]
For more information about SDSoC check here:
[SDSoC User Guides][]

## SUPPORT
For questions and to get help on this project or your own projects, visit the [SDSoC Forums][].

## LICENSE AND CONTRIBUTING TO THE REPOSITORY
The source for this project is licensed under the [3-Clause BSD License][]

To contribute to this project, follow the guidelines in the [Repository Contribution README][]

## 9. ACKNOWLEDGEMENTS
This library is written by developers at
- [Xilinx](http://www.xilinx.com)

## 6. REVISION HISTORY

Date      | Readme Version | Release Notes
--------  |----------------|-------------------------
June2017  | 1.0            | Initial Xilinx release <br> -Windows OS support is in Beta.
 #### Known Issues:
1. Currently, examples only build in Linux OS. Windows OS support is coming soon. For Windows examples, refer to the xfOpenCV examples in the reVision platform, available at here: 
2. Hardware kernels using 128-bit streaming interfaces will experience twice the expected latency. This will be resolved in a coming update. 


[SDSoC Forums]: https://forums.xilinx.com/t5/SDSoC-Development-Environment/bd-p/sdsoc
[SDSoC User Guides]: https://www.xilinx.com/support/documentation/sw_manuals/xilinx2017_1/ug1027-sdsoc-user-guide.pdf
[3-Clause BSD License]: LICENSE.txt
[Repository Contribution README]: CONTRIBUTING.md
[Xilinx OpenCV User Guide]: https://www.xilinx.com/support/documentation/sw_manuals/xilinx2017_1/ug1027-sdsoc-user-guide.pdf
