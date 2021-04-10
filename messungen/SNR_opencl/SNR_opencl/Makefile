
.PHONY: all snrtest_opencl

CXXFLAGS = -Wall -std=c++17 -Dgpu_float_type=cl_double
LDFLAGS = -lpthread

LDFLAGS+= -lOpenCL
#LDFLAGS+= -framework OpenCL

CXX := clang++

OBJ = main.o Traces.o numerics.o

all: snrtest_opencl

snrtest_opencl: $(OBJ)
	$(CXX) $(CXXFLAGS) -o snrtest_opencl $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf $(OBJ) snrtest_opencl
