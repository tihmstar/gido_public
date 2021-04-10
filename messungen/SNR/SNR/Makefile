
.PHONY: all snrtest

CXXFLAGS = -Wall -std=c++17
CFLAGS = -Wall -std=c11
LDFLAGS = -lpthread

CXX = clang++
CC = clang

OBJ = main.o Traces.o numerics.o

OBJC = aes_github.o


all: snrtest

snrtest: $(OBJ) $(OBJC)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJ) $(OBJC) -o snrtest


%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf $(OBJ) snrtest
