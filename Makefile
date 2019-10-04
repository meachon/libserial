OBJS = serial.o
CC = gcc
CXX = g++
CFLAGS = -Wall -g -O
CXXFLAGS = -Wall -Wno-write-strings -lrt -lpthread -ldl -lstdc++ -lm -lc
INC = ./


all: libserial.so

libserial.so: $(OBJS)
	$(CXX) -shared  $^  -o  $@

serial.o: serial.cpp
	$(CXX) $(CFLAGS) -I$(INC) -c -fPIC $< -o $@

.PYONY: clean

clean:
	rm -rf $(OBJS) libserial.so
