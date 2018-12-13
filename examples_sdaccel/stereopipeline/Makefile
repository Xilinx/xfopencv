# stereo pipeline Application

# board
CLC:=$(XILINX_SDX)/bin/xocc
LDCLC:=$(CLC)
DEVICES:= $(PLATFORM_PATH)/xilinx_vcu1525_dynamic_5_1/xilinx_vcu1525_dynamic_5_1.xpfm
KERNEL_NAME:=stereopipeline

#xcl
xcl2_SRCS:=xcl2.cpp
xcl2_HDRS:=xcl2.hpp
xcl2_CXXFLAGS:=-I.

#openCL
# Library directories
SDA_LIB:=$(XILINX_SDX)/lib/lnx64.o
OPENCL_LIB:=$(XILINX_CL_PATH)/lib64
opencl_LDFLAGS=-L$(OPENCL_LIB) -L$(SDA_LIB) -lOpenCL -pthread
opencv_LDFLAGS=-L$(XILINX_SDX)/lnx64/tools/opencv -lopencv_core -lopencv_imgproc -lopencv_highgui

# Host Application
$(KERNEL_NAME)_SRCS=xf_stereo_pipeline_tb.cpp $(xcl2_SRCS)
$(KERNEL_NAME)_HDRS=$(xcl2_HDRS)
$(KERNEL_NAME)_CXXFLAGS=-I. -I$(XILINX_CL_PATH)/include -I$(XILINX_SDX)/include -I$(XILINX_HLS)/include -I$(XLNX_SRC_PATH)/include $(xcl2_CXXFLAGS)
$(KERNEL_NAME)_LDFLAGS=$(opencv_LDFLAGS) $(opencl_LDFLAGS) -Wall -lrt
EXES=$(KERNEL_NAME)

# Kernel
krnl_$(KERNEL_NAME)_SRCS=xf_stereo_pipeline_accel.cpp
krnl_$(KERNEL_NAME)_CLFLAGS=--kernel stereopipeline_accel -I. -I$(XLNX_SRC_PATH)/include --xp prop:kernel.stereopipeline_accel.kernel_flags="-std=c++0x"
XOS=krnl_$(KERNEL_NAME)

# xclbin
krnl_$(KERNEL_NAME)_XOS=krnl_$(KERNEL_NAME)
XCLBINS=krnl_$(KERNEL_NAME)

# Defines the prefix for each kernel.
XCLBIN_DIR=xclbin

#  $(1) - base name for this kernel
#  $(2) - compilation target (i.e. hw, hw_emu, sw_emu)
#  $(3) - device name (i.e. xilinx:adm-pcie-ku3:1ddr:3.0)

# mk_exe - build an exe from host code
define mk_exe
$(1): $($(1)_SRCS) $($(1)_HDRS)
	$(CXX) $(CXXFLAGS) $($(1)_CXXFLAGS) $($(1)_SRCS) -o $$@ $($(1)_LDFLAGS) $(LDFLAGS) -DHLS_NO_XIL_FPO_LIB
EXE_GOALS+= $(1)
endef

# mk_xo - create an xo from a set of kernel sources
define mk_xo
$(XCLBIN_DIR)/$(1).xo: $($(1)_SRCS) $($(1)_HDRS) $(call device2dep,$(3))
	mkdir -p ${XCLBIN_DIR}
	$(CLC) -c $(CLFLAGS) $($(1)_CLFLAGS) $($(1)_$(call device2sandsa,$(3))_CLFLAGS) -o $$@ -t $(2) --platform $(3) $($(1)_SRCS) -DHLS_NO_XIL_FPO_LIB
XO_GOALS+= $(XCLBIN_DIR)/$(1).xo
endef

# mk_xclbin - create an xclbin from a set of krnl sources
define mk_xclbin
$(XCLBIN_DIR)/$(1).xclbin: $(XCLBIN_DIR)/$(1).xo
	mkdir -p ${XCLBIN_DIR}
	$(LDCLC) -l $(LDCLFLAGS) $($(1)_LDCLFLAGS) $($(1)_$(call device2sandsa,$(3))_LDCLFLAGS) -o $$@ -t $(2) --platform $(3) $(XCLBIN_DIR)/$(1).xo -DHLS_NO_XIL_FPO_LIB
XCLBIN_GOALS+= $(XCLBIN_DIR)/$(1).xclbin
endef

$(foreach exe,$(EXES),$(eval $(call mk_exe,$(exe))))
$(foreach xo,$(XOS),$(foreach target,$(TARGETS),$(foreach device,$(DEVICES),$(eval $(call mk_xo,$(xo),$(target),$(device))))))
$(foreach xclbin,$(XCLBINS),$(foreach target,$(TARGETS),$(foreach device,$(DEVICES),$(eval $(call mk_xclbin,$(xclbin),$(target),$(device))))))

.PHONY: all
all: $(EXE_GOALS) $(XCLBIN_GOALS)

.PHONY: clean
clean:
	rm -rf $(EXE_GOALS) $(XCLBIN_DIR)/* _x .run sdaccel_* TempConfig system_estimate.xtxt *.rpt *.png
	rm -rf src/*.ll _xocc_* .Xil emconfig.json $(EXTRA_CLEAN) dltmp* xmltmp* *.log *.jou *.wcfg *.wdb

