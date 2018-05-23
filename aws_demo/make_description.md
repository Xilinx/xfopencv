# Makefiles description # 

Examples for Amazon F1 instance use example specific and common makefile. Example specific makefile is placed at root folder of example and include common makefile ([`aws_demo/common_makefile`](common_makefile)). 

## Example specific makefile ##
Example specific makefile contains following variables to list source files for host application and kernel. 

### Variables for host part ###
 
| Variable&nbsp;Name |Necessity | Purpose |
| :------------- | :------------- |
| **`TEST_NAME`** |Mandatory| Name of the host executable which will be created as successful build result in **`run`** folder |
| **`HOST_AWS_SRC`** |Mandatory| List of host source files placed in root folder of example |
| **`HOST_SDx_SRC`** <br/> **`SDx_LIB_DIR`** | Optional | The **`HOST_SDx_SRC`** contains list of SDx kernel driver source files which provide interaction between host and FPGA kernel on Amazon F1 instance and **`SDx_LIB_DIR`** contains path to these sources. Originally all examples use xcl driver v.2. Default values are assigned in [`common_makefile`](common_makefile). If you would like to use other driver you need to do following: <br/> 1) Modify example source code to use desired driver; <br/>2) assign list of appropriate library source files to **`HOST_SDx_SRC`**; <br/>3) setup path to the library in **`SDx_LIB_DIR`** variable. <br/> Settings of these variables in example specific makefile override default values of [`common_makefile`](common_makefile) |

### Variables for kernel part ###

| Variable&nbsp;Name |Necessity | Purpose |
| :------------- | :------------- |
| **`KERNEL`** |Mandatory| Name of the kernel should be same as kernel source file name (without extension) |


## Common makefile ##
Common makefile contains following variables and makefile's targets for host application and kernel. 

### Variables for host part ###
 
| Variable&nbsp;Name |Default&nbsp;value | Description |
| :------------- | :------------- |
| **`XILINX_SDX`**        |**`/opt/Xilinx/SDx/2017.1.op`**| Path to Xilinx SDx toolset on Amazon F1 instance |
| **`XILINX_HLS`**        |**`$(XILINX_SDX)/Vivado_HLS`** | Path to Xilinx Vivado HLS                        |
| **`SDX_CXX`**           |**`$(XILINX_SDX)/bin/xcpp`**   | Alias for Xilinx SDx compiler                    |
| **`XOCC`**              |**`$(XILINX_SDX)/bin/xocc`**   | ALias for Xilinx XOCC compiler                   |
| **`XILINX_SDX_RUNTIME`**| -                             | Set automatically to run-time library of selected platform (value of **`$(AWS_PLATFORM)`**).|   
| **`XFOPENCV`**          |**`/home/centos/src/project_data/xfopencv`** | Location of xfOpenCV library. <br/>***Note: If you place xfOpenCV library in other location than recommended (default) please update this variable!***                    |
| **`TARGET`**            |**`hw_emu`**                   | The target flow. This variable should be override by desired target flow (**`hw/sw_emu/hw_emu`**) in make command line |
| **`HOST_SDx_SRC`**      |**`xcl2`**                     | List of SDx kernel driver source files which provide interaction between host and FPGA kernel on Amazon F1 instance. Originally all examples use xcl driver v.2.|
| **`SDx_LIB_DIR`**       |**`$(SDACCEL_DIR)/examples/xilinx/libs/xcl2`**   | Path to SDx kernel driver source files |
| **`CXXFLAGS`**          |-                              | Contains SDx compiler options. Please see default value in [`common_makefile`](common_makefile) |
| **`LDFLAGS`**           |-                              | Contains SDx linker options. Please see default value in [`common_makefile`](common_makefile) <br/>***Note: Host application needs specific version of run-time shared libraries. Important to explicitly specify for linker needed libraries with help of `-rpath` option. Take it in mind in case of [`common_makefile`](common_makefile) modification *** |
| **`HOST_AWS_DIR`**      |**`./`**                       | Root folder of example                           |
| **`HOST_BLD_DIR`**      |**`$(TARGET)/build/host`**     | Build folder for host application build artifacts|
| **`HOST_RUN_DIR`**      |**`$(TARGET)/run`**            | Run folder of host application                   |
| **`HOST_EXE`**          |**`$(HOST_RUN_DIR)/$(TEST_NAME)`** | Host application executable name with path   |

### Variables for kernel part ###

| Variable&nbsp;Name |Default value | Description |
| :------------- | :------------- |
| **`XOCC_OPTS`**         |-                              | Contains XOCC options. Please see default value in [`common_makefile`](common_makefile) |
| **`XOCC_INCL`**         |-                              | Contains paths to search header files. Please see default value in [`common_makefile`](common_makefile) |   
| **`KERNEL_BLD_DIR`**    |**`$(TARGET)/build/kernel`**   | Build folder for kernel build artifacts|
| **`KERNEL_RUN_DIR`**    |-                              | Folder to store kernel binary (`.xclbin`). Default value depends on target flow. Please see default value in [`common_makefile`](common_makefile)|


### Makefile targets ###

| Target&nbsp;label | Description |
| :------------- | :------------- |
| **`all`**      | Build host application and kernel for target flow specified by **`$(TARGET)`** variable |
| **`host`**     | Build host application only for target flow specified by **`$(TARGET)`** variable |   
| **`krnl`**     | Build kernel only for target flow specified by **`$(TARGET)`** variable |   
| **`clean`**    | Clean build artifacts of target flow specified by **`$(TARGET)`** variable. <br/>***Note: afi folder of FPGA flow ($(TARGET) == hw) kept untouched. You should clean it manually if needed *** |   



## REVISION HISTORY

Date      | Readme Version | Release Notes
--------  |----------------|-------------------------
May 2018  | 1.0            | Initial version.
