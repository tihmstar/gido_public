
.PHONY: all attack

CXXFLAGS = -Wall -std=c++17
LDFLAGS = -lpthread

OBJ = main.o Traces.o numerics.o

all: attack

attack: $(OBJ)
	clang++ $(CXXFLAGS) $(LDFLAGS) -o attack $(OBJ)

%.o: %.cpp
	clang++ $(CXXFLAGS) -c $<

clean:
	rm -rf $(OBJ) attack
