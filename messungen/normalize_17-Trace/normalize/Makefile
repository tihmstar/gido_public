
.PHONY: all normalize

CXX := clang++

CXXFLAGS = -Wall -std=c++17 -msse4.1 -O2 -DDONT_HAVE_FILESYSTEM -g
LDFLAGS = -lpthread -larchive

OBJ = main.o Traces.o

all: normalize

normalize: $(OBJ)
	$(CXX) $(CXXFLAGS) -o normalize $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf $(OBJ) normalize
