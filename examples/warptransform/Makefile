APPSOURCES =xf_warp_transform_tb.cpp xf_warp_transform_accel.cpp
EXECUTABLE = warptransform.elf
VERBOSE = #-verbose
PLATFORM = <path-to-reVISION-platform>
SDSCC_FLAGS = -sds-pf ${PLATFORM} -std=c++11 ${VERBOSE} \
-sds-hw "xf::warpTransform<100,50,0,1,0,1080,1920,1>" xf_warp_transform_accel.cpp -files ../../include/imgproc/xf_warp_transform.hpp -clkid 2 -sds-end -dmclkid 2
CC = sds++ ${SDSCC_FLAGS}

IDIRS = -I. -I${SYSROOT}/usr/include -I ../../include

LDIRS = --sysroot=${SYSROOT} -L=/lib -L=/usr/lib -Wl,-rpath-link=${SYSROOT}/lib,-rpath-link=${SYSROOT}/usr/lib
LLIBS = -lopencv_core -llzma -ltiff -lpng16 -lz -ljpeg -lopencv_imgproc -lopencv_imgcodecs -ldl -lrt -lwebp -lopencv_features2d -lopencv_flann
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
copy:
	/bin/cp -prf ${XLNX_SRC_PATH}/warptransform/example .
	/bin/cp -prf ${XLNX_SRC_PATH}/warptransform/src .
