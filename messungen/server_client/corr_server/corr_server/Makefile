CXXFLAGS = -Wall -std=c++17 -g
LDFLAGS = -lpthread -larchive

CXX := clang++

OBJ = CPUModelResults.o CorrClient.o CorrServer.o LoadFile.o combtrace.o main.o myfile.o numerics.o

all: corr_server

corr_server: $(OBJ)
	$(CXX) $(CXXFLAGS) -o corr_server $(OBJ) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -rf $(OBJ) corr_server
