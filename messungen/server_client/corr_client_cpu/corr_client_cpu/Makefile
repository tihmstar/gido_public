CXXFLAGS = -Wall -std=c++17 -Dgpu_float_type=cl_double -g
LDFLAGS = -lpthread -larchive

LDFLAGS+= -lOpenCL
#LDFLAGS+= -framework OpenCL

CXX := clang++

OBJ = CorrWorker.o CPULoader.o CPUModelResults.o GPUDevice.o GPUMem.o GPUPowerModel.o GPUPowerModelExecutionWorker.o LoadFile.o combtrace.o main.o numerics.o

all: corr_client_opencl

corr_client_opencl: $(OBJ)
	$(CXX) $(CXXFLAGS) -o corr_client_opencl $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf $(OBJ) corr_client_opencl
