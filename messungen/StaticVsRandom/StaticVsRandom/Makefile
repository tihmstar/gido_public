.PHONY: all svrtest

CXXFLAGS = -Wall -O2 -std=c++17 -g
LDFLAGS = -lpthread -larchive

OBJ = main.o Traces.o numerics.o

all: svrtest

svrtest: $(OBJ)
	clang++ $(CXXFLAGS) -o svrtest $(OBJ) $(LDFLAGS)

%.o: %.cpp
	clang++ $(CXXFLAGS) -g -c $<

clean:
	rm -rf $(OBJ) svrtest
