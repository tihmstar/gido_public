
.PHONY: all normalize

CXX := clang++

CXXFLAGS = -Wall -std=c++17 -msse4.1 -O3
LDFLAGS = -lpthread

OBJ = main.o Traces.o

all: normalize

normalize: $(OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o normalize $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf $(OBJ) normalize
