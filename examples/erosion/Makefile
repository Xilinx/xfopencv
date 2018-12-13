APPSOURCES =  xf_erosion_accel.cpp  xf_erosion_tb.cpp 
EXECUTABLE = erosion.elf
VERBOSE = #-verbose
PLATFORM = <path-to-reVISION-platform>
SDSCC_FLAGS = -sds-pf ${PLATFORM}  ${VERBOSE} \
-sds-hw "xf::erode<1, 0, 2160, 3840, 2 ,3, 3, 1,1>"  xf_erosion_accel.cpp -files ../../include/imgproc/xf_erosion.hpp -clkid 2 -sds-end -dmclkid 2

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
