
.PHONY: all attack_opencl

CXXFLAGS = -Wall -std=c++17
LDFLAGS = -lpthread -lOpenCL

CXX := clang++

OBJ = main.o Traces.o CLTraces.o numerics.o helpers.cpp

all: attack_opencl

attack_opencl: $(OBJ)
	$(CXX) $(CXXFLAGS) -o attack_opencl $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf $(OBJ) attack_opencl
