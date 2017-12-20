APPSOURCES = xf_gaussian_diff_tb.cpp  xf_gaussian_diff_accel.cpp
EXECUTABLE = gaussiandiff.elf
VERBOSE = #-verbose
PLATFORM = <path-to-revision-platform>
SDSCC_FLAGS = -sds-pf ${PLATFORM} \
-sds-hw "xf::GaussianBlur<3, 0, 0, 1080, 1920, 1>" ./xf_gaussian_diff_accel.cpp -files ../../include/imgproc/xf_gaussian_filter.hpp -clkid 1 -sds-end -dmclkid 1\
-sds-hw "xf::Duplicatemats< 0, 1080, 1920, 1>" ./xf_gaussian_diff_accel.cpp -files ../../include/xf_duplicateimage.hpp -clkid 1 -sds-end -dmclkid 1\
-sds-hw "xf::delayimages< 0, 1080, 1920, 1,7860>" ./xf_gaussian_diff_accel.cpp -files ../../include/imgproc/xf_delay.hpp -clkid 1 -sds-end -dmclkid 1\
-sds-hw "xf::subtract<0, 0, 1080, 1920, 1 >" ./xf_gaussian_diff_accel.cpp -files ../../include/core/xf_arithm.hpp -clkid 1 -sds-end -dmclkid 1\

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
	/bin/cp -prf ${XLNX_SRC_PATH}/gaussiandifference/example .
	/bin/cp -prf ${XLNX_SRC_PATH}/gaussiandifference/src .
	/bin/cp -prf ${XLNX_SRC_PATH}/gaussianfilter/src .
	/bin/cp -prf ${XLNX_SRC_PATH}/arithm/src .
