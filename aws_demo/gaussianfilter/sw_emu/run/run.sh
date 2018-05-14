emconfigutil -f $AWS_PLATFORM

export XCL_EMULATION_MODE=sw_emu

./gaussian_filter_test ../../img0.jpg
