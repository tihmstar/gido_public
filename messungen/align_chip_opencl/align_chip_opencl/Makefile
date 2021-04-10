CXXFLAGS = -Wall -std=c++17 -msse4.1 -Dgpu_float_type=cl_double -g -I/usr/local/Cellar/libarchive/3.4.0/include
LDFLAGS = -lpthread -larchive

LDFLAGS+= -lOpenCL
#LDFLAGS+= -framework OpenCL

CXX := clang++

OBJ = CPULoader.o GPUDevice.o GPUMem.o GPUWorker.o LoadFile.o PosAligner.o TraceAligner.o combtrace.o main.o

all: align_chip_opencl

align_chip_opencl: $(OBJ)
	$(CXX) $(CXXFLAGS) -o align_chip_opencl $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf $(OBJ) align_chip_opencl
