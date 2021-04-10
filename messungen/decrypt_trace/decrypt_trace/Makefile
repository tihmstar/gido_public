
.PHONY: all decrypt_trace

CXX := clang++

CXXFLAGS = -Wall -std=c++17 -O2 -DDONT_HAVE_FILESYSTEM -g
LDFLAGS = -lpthread -larchive -lirecovery

OBJ = main.o CPULoader.o LoadTraceFile.o

all: decrypt_trace

decrypt_trace: $(OBJ)
	$(CXX) $(CXXFLAGS) -o decrypt_trace $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf $(OBJ) decrypt_trace
