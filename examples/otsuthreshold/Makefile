APPSOURCES =xf_otsuthreshold_tb.cpp xf_otsuthreshold_accel.cpp
EXECUTABLE = otsuthreshold.elf
VERBOSE = #-verbose
PLATFORM = <path-to-reVISION-platform>

SDSCC_FLAGS = -sds-pf ${PLATFORM} \
-sds-hw "xf::OtsuThreshold<0,1080,1920,1>" xf_otsuthreshold_accel.cpp -files ../../include/imgproc/xf_otsuthreshold.hpp -files ../../include/imgproc/xf_histogram.hpp -clkid 2 -sds-end -dmclkid 2 ${VERBOSE}


CC = sds++ ${SDSCC_FLAGS}

IDIRS = -I. -I${SYSROOT}/usr/include -I ../../include

LDIRS = --sysroot=${SYSROOT} -L=/lib -L=/usr/lib -Wl,-rpath-link=${SYSROOT}/lib,-rpath-link=${SYSROOT}/usr/lib
LLIBS = -lopencv_core -llzma -ltiff -lpng16 -lz -ljpeg -lopencv_imgproc -lopencv_imgcodecs -ldl -lrt -lwebp -lopencv_features2d -lopencv_flann
OLEVEL = -ggdb
CFLAGS = -Wall ${OLEVEL} -c ${IDIRS} -DHLS_NO_XIL_FPO_LIB
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
copy:
	/bin/cp -prf ${XLNX_SRC_PATH}/otsuthreshold/example .
	/bin/cp -prf ${XLNX_SRC_PATH}/otsuthreshold/src .
	/bin/cp -prf ${XLNX_SRC_PATH}/histogram/src .
