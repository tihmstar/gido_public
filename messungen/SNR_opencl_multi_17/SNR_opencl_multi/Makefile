CXXFLAGS = -Wall -std=c++17 -Dgpu_float_type=cl_double -g
LDFLAGS = -lpthread -larchive

LDFLAGS+= -lOpenCL
#LDFLAGS+= -framework OpenCL

CXX := clang++

OBJ = main.o Traces.o numerics.o CPUModelResults.o GPUDeviceWorker.o GPUPowerModel.o CPUMem.o GPUMem.o

all: snrtest_opencl_multi

snrtest_opencl_multi: $(OBJ)
	$(CXX) $(CXXFLAGS) -o snrtest_opencl_multi $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf $(OBJ) snrtest_opencl_multi
