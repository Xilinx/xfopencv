
# compiler tools

XILINX_SDX ?= /opt/Xilinx/SDx/2017.1.op
XILINX_VIVADO_HLS ?= $(XILINX_SDX)/Vivado_HLS


SDX_CXX ?= $(XILINX_SDX)/bin/xcpp
XOCC ?= $(XILINX_SDX)/bin/xocc
RM = rm -f
RMDIR = rm -rf

SDX_PLATFORM = $(AWS_PLATFORM)

XFOPENCV ?= /home/centos/src/project_data/xfopencv

########################################
#                                      #
#             Host section             #
#                                      #
########################################

XF_EXAMPLE_DIR = $(XFOPENCV)/examples/gaussianfilter

XCL2_LIB_DIR = $(SDACCEL_DIR)/examples/xilinx/libs/xcl2
XCL_LIB_DIR  = $(SDACCEL_DIR)/examples/xilinx/libs/xcl

CXXFLAGS += -DSDX_PLATFORM=$(SDX_PLATFORM) -D__USE_XOPEN2K8 
CXXFLAGS += -I/opt/Xilinx/SDx/2017.1.op/runtime/include/1_2/ 
CXXFLAGS += -I/opt/Xilinx/SDx/2017.1.op/include/         
CXXFLAGS += -I$(XFOPENCV)/include/  
CXXFLAGS += -I$(XF_EXAMPLE_DIR)/
CXXFLAGS += -I$(XCL2_LIB_DIR)/
CXXFLAGS += -I/opt/Xilinx/SDx/2017.1.op/Vivado_HLS/include
CXXFLAGS += -O2 -Wall -c -fmessage-length=0 -std=c++14

#--- Specify OpenCV libraries ---#

LDFLAGS += -L/opt/Xilinx/SDx/2017.1.op/lnx64/tools/opencv
LDFLAGS += -lopencv_core
LDFLAGS += -lopencv_imgproc
LDFLAGS += -lopencv_highgui

#--- Specify common libraries ---#

LDFLAGS += -L/opt/Xilinx/SDx/2017.1.op/lib/lnx64.o 
LDFLAGS += -lstdc++
LDFLAGS += -lpthread 
LDFLAGS += -lrt

#--- Specify AWS libraries ---#

LDFLAGS += -L/opt/Xilinx/2017.1.rte.4ddr/runtime/lib/x86_64 
LDFLAGS += -lxilinxopencl

#--- Specify runtime libraries ---#

LDFLAGS += -Wl,-rpath,/opt/Xilinx/SDx/2017.1.op/lnx64/tools/opencv
LDFLAGS += -Wl,-rpath,/opt/Xilinx/SDx/2017.1.op/lib/lnx64.o 
LDFLAGS += -Wl,-rpath,/opt/Xilinx/2017.1.rte.4ddr/runtime/lib/x86_64

#--- Specify objects ---#

HOST_DEP = ../src/xcl2.hpp

HOST_AWS_SRC +=  xf_gaussian_filter_accel_aws
HOST_SDx_SRC +=  xcl2
HOST_XF_SRC  +=  xf_gaussian_filter_tb

HOST_AWS_DIR = ./
HOST_BLD_DIR = build/host

HOST_AWS_OBJ +=  $(addsuffix .o, $(addprefix $(HOST_BLD_DIR)/, $(HOST_AWS_SRC)) )
HOST_SDx_OBJ +=  $(addsuffix .o, $(addprefix $(HOST_BLD_DIR)/, $(HOST_SDx_SRC)) )
HOST_XF_OBJ  +=  $(addsuffix .o, $(addprefix $(HOST_BLD_DIR)/, $(HOST_XF_SRC)) )

HOST_OBJ = $(HOST_XF_OBJ) $(HOST_AWS_OBJ) $(HOST_SDx_OBJ)

HOST_EXE ?= xf_arg

BUILD_SUBDIRS += $(HOST_BLD_DIR)


########################################
#                                      #
#            Kernel section            #
#                                      #
########################################


XOCC_OPTS = -t hw --platform $(SDX_PLATFORM) --save-temps  --report system

#
# OpenCL kernel files
#

BINARY_CONTAINERS += xf_gaussian_filter.xclbin

BUILD_SUBDIRS += build/kernel
BINARY_CONTAINER_1_OBJS += build/kernel/xf_gaussian_filter.xo
ALL_KERNEL_OBJS += build/kernel/xf_gaussian_filter.xo

ALL_MESSAGE_FILES = $(subst .xo,.mdb,$(ALL_KERNEL_OBJS)) $(subst .xclbin,.mdb,$(BINARY_CONTAINERS))


#
# primary build targets
#

.PHONY: all clean

all: krnl host

host: $(HOST_EXE)

krnl: $(BINARY_CONTAINERS)

clean:
	$(RMDIR) $(HOST_BLD_DIR) build/kernel
#	-$(RM) $(BINARY_CONTAINERS) $(ALL_KERNEL_OBJS) $(ALL_MESSAGE_FILES)  $(HOST_EXE) src/$(HOST_OBJ)
#	-$(RM) *.xclbin.sh
#	-$(RMDIR) $(BUILD_SUBDIRS)
#	-$(RMDIR) _xocc*
#	-$(RMDIR) .Xil
#	-$(RM) xf_arg

.PHONY: incremental
incremental: all


nothing:

#
# binary container: binary_container_1.xclbin
#

build/kernel/xf_gaussian_filter.xo: xf_gaussian_filter_kernel_aws.cpp
	@mkdir -p $(@D)
	$(XOCC) $(XOCC_OPTS) -c -k xf_gaussian_filter --max_memory_ports xf_gaussian_filter --messageDb $(subst .xo,.mdb,$@) -I"$(<D)" --xp misc:solution_name=build/kernel/xocc_compile_kernel_xf_gaussian_filter -I"$(XFOPENCV)/include" -I"$(XF_EXAMPLE_DIR)" -I/opt/Xilinx/SDx/2017.4/include/ocv -o"$@" "$<"
#	@echo -$(RM) $@

xf_gaussian_filter.xclbin: $(BINARY_CONTAINER_1_OBJS)
	-@echo $(XOCC) $(XOCC_OPTS) -l --nk xf_gaussian_filter:1  --messageDb $(subst .xclbin,.mdb,$@) --xp misc:solution_name=build/kernel/xocc_link_kernel --remote_ip_cache /root/workspace/ip_cache -o"$@" $(+) > xf_gaussian_filter.xclbin.sh
	$(XOCC) $(XOCC_OPTS) -l --jobs 20 --nk xf_gaussian_filter:1 --messageDb $(subst .xclbin,.mdb,$@) --xp misc:solution_name=build/kernel/xocc_link_kernel --remote_ip_cache /root/workspace/ip_cache -o"$@" $(+)

#
# host rules
#

$(HOST_AWS_OBJ): $(HOST_BLD_DIR)/%.o : $(HOST_AWS_DIR)/%.cpp 
	@echo "================================================================"
	@echo "Compilation of $< to $@"
	@echo "================================================================"
	@mkdir -p $(HOST_BLD_DIR)                                       
	$(SDX_CXX) $(CXXFLAGS) -o "$@" "$<"

	
$(HOST_SDx_OBJ): $(HOST_BLD_DIR)/%.o : $(XCL2_LIB_DIR)/%.cpp 
	@echo "================================================================"
	@echo "Compilation of $< to $@"
	@echo "================================================================"
	@mkdir -p $(HOST_BLD_DIR)                                       
	$(SDX_CXX) $(CXXFLAGS) -o "$@" "$<"

$(HOST_XF_OBJ): $(HOST_BLD_DIR)/%.o : $(XF_EXAMPLE_DIR)/%.cpp 
	@echo "================================================================"
	@echo "Compilation of $< to $@"
	@echo "================================================================"
	@mkdir -p $(HOST_BLD_DIR)                                       
	$(SDX_CXX) $(CXXFLAGS) -o "$@" "$<"

	
$(HOST_EXE): $(HOST_OBJ)
	@echo "================================================================"
	@echo "Linking of $@"
	@echo "================================================================"
	$(SDX_CXX) -o "$@" $(+) $(LDFLAGS) 
	


