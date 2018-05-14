emconfigutil -f $AWS_PLATFORM

export XCL_EMULATION_MODE=hw_emu

./stereo_pipeline_test ../../left.png ../../right.png
