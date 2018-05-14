emconfigutil -f $AWS_PLATFORM

export XCL_EMULATION_MODE=sw_emu 

./stereo_pipeline_test ../../left.png ../../right.png