APPSOURCES = xf_scharr_tb.cpp xf_scharr_accel.cpp
EXECUTABLE = scharr.elf
VERBOSE = #-verbose
PLATFORM = <path-to-reVISION-platform>
SDSCC_FLAGS = -sds-pf ${PLATFORM} -std=c++11 ${VERBOSE} \
-sds-hw "xf::Scharr<0,0,1,1080, 1920,1>" xf_scharr_accel.cpp -files ../../include/imgproc/xf_scharr.hpp -clkid 2 -sds-end -dmclkid 2
#Scharr<XF_BORDER_CONSTANT,XF_8UC1,XF_16UC1,HEIGHT, WIDTH,XF_NPPC8>(imgInput, imgOutputx,imgOutputy);

CC = sds++ ${SDSCC_FLAGS}

IDIRS = -I. -I${SYSROOT}/usr/include -I ../../include

LDIRS = --sysroot=${SYSROOT} -L=/lib -L=/usr/lib -Wl,-rpath-link=${SYSROOT}/lib,-rpath-link=${SYSROOT}/usr/lib

LLIBS = -lopencv_imgcodecs -lopencv_core -llzma -ltiff -lpng16 -lz -ljpeg -lopencv_imgproc -ldl -lrt -lwebp -lopencv_features2d -lopencv_flann 
#LLIBS = -lopencv_core -lopencv_imgproc -lopencv_imgcodecs 
#OLEVEL = -ggdb
CFLAGS = -O3 -Wall ${OLEVEL} -c ${IDIRS} -DHLS_NO_XIL_FPO_LIB
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
copy:
	/bin/cp -prf ${XLNX_SRC_PATH}/scharrfilter/example .
	/bin/cp -prf ${XLNX_SRC_PATH}/scharrfilter/src .
