
.PHONY: all attack

CXXFLAGS = -Wall -Ofast -std=c++17
LDFLAGS = -lpthread

OBJ = main.o Traces.o numerics.o

all: attack

attack: $(OBJ)
	clang++ $(CXXFLAGS) $(LDFLAGS) -o attack $(OBJ)

%.o: %.cpp
	clang++ $(CXXFLAGS) -g -c $<

clean:
	rm -rf $(OBJ) attack
