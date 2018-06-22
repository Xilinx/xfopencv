APPSOURCES =  xf_corner_tracker_accel.cpp  xf_corner_tracker_tb.cpp
EXECUTABLE = cornerTracker.elf
CLK_ID = -clkid 2
DMCLK = -dmclkid 2
# SYSROOT = /proj/fisdata/fisusr/sandboxes/ckohn/2017.4/sysroot
VERBOSE = 
# -verbose
IDIRS = -I. -I${SYSROOT}/usr/include -I ../../include
LDIRS = --sysroot=${SYSROOT} -L=/lib -L=/usr/lib -Wl,-rpath-link=${SYSROOT}/lib,-rpath-link=${SYSROOT}/usr/lib
PLATFORM = <path-to-reVISION-platform>
SDSCC_FLAGS = -sds-pf ${PLATFORM}   \
-sds-hw "xf::pyrDown<0,1080,1920,1>"  xf_corner_tracker_accel.cpp -files ../../include/imgproc/xf_pyr_down.hpp ${CLK_ID} -sds-end\
-sds-hw "xf::densePyrOpticalFlow<5,50,11,16,10,0,1080,1920,1>"  xf_corner_tracker_accel.cpp -files ../../include/imgproc/xf_pyr_dense_optical_flow_wrapper.hpp ${CLK_ID} -sds-end\
-sds-hw "xf::cornerUpdate<10000,3,1080,1920,1>"  xf_corner_tracker_accel.cpp -files ../../include/imgproc/xf_corner_update.hpp ${CLK_ID} -sds-end\
-sds-hw "xf::cornerHarris<3,3,1,0,1080,1920,1>"  xf_corner_tracker_accel.cpp -files ../../include/features/xf_harris.hpp ${CLK_ID} -sds-end\
-sds-hw "xf::cornersImgToList<10000,0,1080,1920,1>"  xf_corner_tracker_accel.cpp -files ../../include/imgproc/xf_corner_img_to_list.hpp ${CLK_ID} -sds-end\

CC = sds++ ${SDSCC_FLAGS} ${DMCLK} ${VERBOSE}
LLIBS = -lopencv_imgcodecs -lopencv_core -llzma -ltiff -lpng16 -lz -ljpeg -lopencv_imgproc -ldl -lrt -lwebp -lopencv_features2d -lopencv_flann -lopencv_highgui -lopencv_videoio
OLEVEL = -ggdb
CFLAGS =  -O0 -Wall ${OLEVEL} -c ${IDIRS} -DHLS_NO_XIL_FPO_LIB -mstrict-align -hls-target 1
LFLAGS = ${OLEVEL} ${LDIRS} ${LLIBS} -maxjobs 8
SOURCES := $(patsubst %,./%,$(APPSOURCES))
OBJECTS := $(APPSOURCES:.cpp=.o)
DEPS := $(OBJECTS:.o=.d)
.PHONY: all
all: ${EXECUTABLE}
${EXECUTABLE}: ${OBJECTS}
	${CC} ${OBJECTS} ${LFLAGS} -o $@
-include ${DEPS}
%.o: %.cpp
	${CC} ${CFLAGS} $< -o $@
clean:
	${RM} ${EXECUTABLE} ${OBJECTS}
ultraclean: clean
	${RM} -rf _sds sd_card ${EXECUTABLE}.bit   ../../include/
