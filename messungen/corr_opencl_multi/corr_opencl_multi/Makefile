CXXFLAGS = -Wall -std=c++17 -Dgpu_float_type=cl_double -g
LDFLAGS = -lpthread -larchive

LDFLAGS+= -lOpenCL
#LDFLAGS+= -framework OpenCL

CXX := clang++

OBJ = CPULoader.o CPUModelResults.o GPUDevice.o GPUMem.o GPUPowerModel.o GPUPowerModelExecutionWorker.o LoadFile.o combtrace.o main.o numerics.o

all: corr_opencl_multi

corr_opencl_multi: $(OBJ)
	$(CXX) $(CXXFLAGS) -o corr_opencl_multi $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf $(OBJ) corr_opencl_multi
