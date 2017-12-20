APPSOURCES = xf_warpaffine_tb.cpp xf_warpaffine_cref.cpp xf_warpaffine_accel.cpp
EXECUTABLE = warpaffine.elf
VERBOSE = #-verbose
PLATFORM = <path-to-reVISION-platform>
SDSCC_FLAGS = -sds-pf ${PLATFORM} \
-sds-hw "xf::warpAffine<1, 0, 1080, 1920,1>" xf_warpaffine_accel.cpp -files ../../include/imgproc/xf_warpaffine.hpp -clkid 2 -sds-end -dmclkid 2\

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
	/bin/cp -prf ${XLNX_SRC_PATH}/warpaffine/example .
	/bin/cp -prf ${XLNX_SRC_PATH}/warpaffine/src .
