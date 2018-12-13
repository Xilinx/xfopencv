APPSOURCES =  xf_kalmanfilter_accel.cpp  xf_kalmanfilter_tb.cpp 
EXECUTABLE = kalmanfilter.elf
VERBOSE = #-verbose
PLATFORM = <path-to-reVISION-platform>

#Flag to enable/disable usage of URAMs
URAM_ENA := 0

ifeq ($(URAM_ENA), 1)
   URAM_ENA_FLAG := -DXF_USE_URAM=true
else
   URAM_ENA_FLAG := -DXF_USE_URAM=false
endif

SDSCC_FLAGS = -sds-pf ${PLATFORM}  -xp param:compiler.skipTimingCheckAndFrequencyScaling=1 \
	-xp vivado_param:project.writeIntermediateCheckpoints=1 \
	${VERBOSE} \
-sds-hw "KalmanFilter<128, 128, 128, 16, 16, $(URAM_ENA), 4, 1>"  xf_kalmanfilter_accel.cpp -files ../../include/imgproc/xf_kalmanfilter.hpp -clkid 2 -sds-end -dmclkid 2

CC = sds++ ${SDSCC_FLAGS} 

IDIRS = -I. -I${SYSROOT}/usr/include -I ../../include
LDIRS = --sysroot=${SYSROOT} -L=/lib -L=/usr/lib -Wl,-rpath-link=${SYSROOT}/lib,-rpath-link=${SYSROOT}/usr/lib

LLIBS = -lopencv_imgcodecs -lopencv_core  -lopencv_video -llzma -ltiff -lpng16 -lz -ljpeg -lopencv_imgproc -ldl -lrt -lwebp -lopencv_features2d -lopencv_flann 
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
