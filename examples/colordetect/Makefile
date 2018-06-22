APPSOURCES =  xf_colordetect_tb.cpp   xf_colordetect_accel.cpp
EXECUTABLE = colordetect.elf
VERBOSE = #-verbose
PLATFORM = <path-to-reVISION-platform>
SDSCC_FLAGS = -sds-pf ${PLATFORM} \
-sds-hw "xf::RGB2HSV< 7,2160, 3840,1 >"  xf_colordetect_accel.cpp -files ../../include/imgproc/xf_rgb2hsv.hpp -clkid 2 -sds-end -dmclkid 2\
-sds-hw "xf::colorthresholding< 7,0,3,2160, 3840,1>"  xf_colordetect_accel.cpp -files ../../include/imgproc/xf_inrange.hpp -clkid 2 -sds-end -dmclkid 2\
-sds-hw "xf::erode<0,0,2160, 3840,1>"  xf_colordetect_accel.cpp -files ../../include/imgproc/xf_erosion.hpp -clkid 2 -sds-end -dmclkid 2\
-sds-hw "xf::dilate<0,0,2160, 3840,1>"  xf_colordetect_accel.cpp -files ../../include/imgproc/xf_dilation.hpp -clkid 2 -sds-end -dmclkid 2\

IDIRS = -I. -I${SYSROOT}/usr/include -I ../../include
LDIRS = --sysroot=${SYSROOT} -L=/lib -L=/usr/lib -Wl,-rpath-link=${SYSROOT}/lib,-rpath-link=${SYSROOT}/usr/lib

CC = sds++ ${SDSCC_FLAGS}

LLIBS = -lopencv_imgcodecs -lopencv_core -llzma -ltiff -lpng16 -lz -ljpeg -lopencv_imgproc -ldl -lrt -lwebp -lopencv_features2d -lopencv_flann 
#LLIBS = -lopencv_core -lopencv_imgproc -lopencv_imgcodecs 
#OLEVEL = -ggdb
CFLAGS =  -O0 -Wall ${OLEVEL} -c ${IDIRS} -DHLS_NO_XIL_FPO_LIB -mstrict-align -hls-target 1
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
	${RM} -rf _sds sd_card ${EXECUTABLE}.bit ${EXECUTABLE} ${OBJECTS}
