APPSOURCES =xf_corner_tracker_accel.cpp xf_corner_tracker_tb.cpp
EXECUTABLE = cornerTracker.elf
CLK_ID = -clkid 2
DMCLK = -dmclkid 2
VERBOSE = #-verbose
PLATFORM = <path-to-reVISION-platform>
SDSCC_FLAGS = -sds-pf ${PLATFORM} -std=c++11  \
-sds-hw "xf::cornerHarris < 3,3,1,0,1080,1920,1>" xf_corner_tracker_accel.cpp -files ../../include/features/xf_harris.hpp ${CLK_ID} -sds-end\
-sds-hw "xf::cornersImgToList < 10000,0,1080,1920,1>" xf_corner_tracker_accel.cpp -files ../../include/imgproc/xf_corner_img_to_list.hpp ${CLK_ID} -sds-end\
-sds-hw "xf::pyrDown < 0,1080,1920,1>" xf_corner_tracker_accel.cpp -files ../../include/imgproc/xf_pyr_down.hpp ${CLK_ID} -sds-end\
-sds-hw "xf::densePyrOpticalFlow < 5,50,11,16,10,0,1080,1920,1>" xf_corner_tracker_accel.cpp -files ../../include/imgproc/xf_pyr_dense_optical_flow_wrapper.hpp ${CLK_ID} -sds-end\
-sds-hw "xf::cornerUpdate < 10000,3,1080,1920,1>" xf_corner_tracker_accel.cpp -files ../../include/imgproc/xf_corner_update.hpp ${CLK_ID} -sds-end

CC = sds++ ${SDSCC_FLAGS}

IDIRS = -I. -I${SYSROOT}/usr/include -I ../../include

LDIRS = --sysroot=${SYSROOT} -L=/lib -L=/usr/lib -Wl,-rpath-link=${SYSROOT}/lib,-rpath-link=${SYSROOT}/usr/lib ${DMCLK} ${VERBOSE}
LLIBS = -lopencv_core -lopencv_videoio -llzma -ltiff -lpng16 -lz -ljpeg -lopencv_imgproc -lopencv_imgcodecs -ldl -lrt -lwebp -lopencv_features2d -lopencv_flann
OLEVEL = -ggdb
CFLAGS = -Wall ${OLEVEL} -c ${IDIRS} -DHLS_NO_XIL_FPO_LIB -DXI_REL=${XI_REL}
LFLAGS = ${OLEVEL} ${LDIRS} ${LLIBS}
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
	${RM} -rf _sds sd_card ${EXECUTABLE}.bit