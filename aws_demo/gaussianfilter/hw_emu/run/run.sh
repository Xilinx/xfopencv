emconfigutil -f $AWS_PLATFORM

export XCL_EMULATION_MODE=hw_emu

./gaussian_filter_test ../../img0.jpg
