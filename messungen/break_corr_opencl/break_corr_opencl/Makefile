CXXFLAGS = -Wall -std=c++17 -Dgpu_float_type=cl_double -g
LDFLAGS = -lpthread -larchive

LDFLAGS+= -lOpenCL
#LDFLAGS+= -framework OpenCL

CXX := clang++

OBJ = CPULoader.o CPUModelResults.o GPUDevice.o GPUMem.o GPUPowerModel.o GPUPowerModelExecutionWorker.o LoadFile.o combtrace.o main.o numerics.o

all: break_corr_opencl

break_corr_opencl: $(OBJ)
	$(CXX) $(CXXFLAGS) -o break_corr_opencl $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf $(OBJ) break_corr_opencl
