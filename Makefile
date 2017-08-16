GSL_LITE_INCLUDE=gsl-lite/include

CPPFLAGS=-I/usr/include/gdal -I/usr/include/mapserver -I$(GSL_LITE_INCLUDE)
LDLIBS=-lmapserver -lboost_filesystem -lboost_system
CXXFLAGS=-std=gnu++14 -Os
 
TARGETS=map_deps

all: $(TARGETS)

map_deps: map_deps.o mapfile.o
	$(CXX) $(LDFLAGS) -o map_deps map_deps.o mapfile.o $(LDLIBS)

mapfile.o: mapfile.cc mapfile.hh

clean:
	$(RM) *.o $(TARGETS)
