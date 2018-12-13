APPSOURCES =  xf_canny_accel.cpp  xf_canny_tb.cpp 
EXECUTABLE = canny.elf
VERBOSE = #-verbose
PLATFORM ?= <Path-to-reVISION-Platform>

URAM_ENA := 0

ifeq ($(URAM_ENA), 1)
   URAM_ENA_FLAG := -DXF_USE_URAM=true
else
   URAM_ENA_FLAG := -DXF_USE_URAM=false
endif


IDIRS = -I. -I${SYSROOT}/usr/include -I ../../include
LDIRS = --sysroot=${SYSROOT} -L=/lib -L=/usr/lib -Wl,-rpath-link=${SYSROOT}/lib,-rpath-link=${SYSROOT}/usr/lib

SDSCC_FLAGS = -sds-pf ${PLATFORM}  ${VERBOSE} ${URAM_ENA_FLAG} \
-sds-hw "xf::Canny<3, 0,0,8,2160, 3840,1,4,${URAM_ENA}>"  xf_canny_accel.cpp -files ../../include/imgproc/xf_canny.hpp -clkid 2 -sds-end -dmclkid 2\
-sds-hw "xf::EdgeTracing<8, 0,2160, 3840,32,8,${URAM_ENA}>"  xf_canny_accel.cpp -files ../../include/imgproc/xf_edge_tracing.hpp -clkid 2 -sds-end -dmclkid 2

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

