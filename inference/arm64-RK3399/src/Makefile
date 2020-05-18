
VPATH = ./

INCLUDES = -I ./ -I ./include
LIB = ../libs

DEFINES =
COMPILE_OPTIONS = -fstack-check -fexceptions -ggdb
LINK_OPTIONS = -pthread

LIBRARIES = -Wl,-rpath,'$$ORIGIN/../libs' -Wl,-Bdynamic -lc -ldl -lm -lpthread -lrt `pkg-config --libs --cflags opencv` -L$(LIB) -lpaddle_light_api_shared

METER_LIST = meter.o global.o detection.o segmentation.o readvalue.o
FULL_OBJECT_LIST = $(METER_LIST) version.o
	
TARGETS = meter

.PHONY: all
	
all: $(TARGETS) 

meter : $(METER_LIST) version.o
	g++ $(LINK_OPTIONS) -o $@ $(METER_LIST) version.o $(LIBRARIES) 

.PHONY: updateBuildTime
updateBuildTime:
	rm -f version.o

%.o: %.cpp
	g++ -c -std=c++11 $(COMPILE_OPTIONS) $(DEFINES) $(INCLUDES) $<
	
.PHONY: clean
clean:
	rm -f $(FULL_OBJECT_LIST) $(TARGETS)

