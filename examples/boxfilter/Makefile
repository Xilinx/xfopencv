APPSOURCES =  xf_box_filter_accel.cpp  xf_box_filter_tb.cpp 
EXECUTABLE = boxfilter.elf
VERBOSE = #-verbose
PLATFORM ?= <Path-to-reVISION-Platform>

URAM_ENA := 0

ifeq ($(URAM_ENA), 1)
   URAM_ENA_FLAG := -DXF_USE_URAM=true
else
   URAM_ENA_FLAG := -DXF_USE_URAM=false
endif

SDSCC_FLAGS = -sds-pf ${PLATFORM} ${URAM_ENA_FLAG} -sds-hw "xf::boxFilter<0, 3,2,2160, 3840,1,${URAM_ENA}>"  xf_box_filter_accel.cpp -files ../../include/imgproc/xf_box_filter.hpp -clkid 2 -sds-end -dmclkid 2

CC = sds++ ${SDSCC_FLAGS}

IDIRS = -I. -I${SYSROOT}/usr/include -I ../../include
LDIRS = --sysroot=${SYSROOT} -L=/lib -L=/usr/lib -Wl,-rpath-link=${SYSROOT}/lib,-rpath-link=${SYSROOT}/usr/lib
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
