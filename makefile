ECHO = echo
MAKE = mingw32-make
# CC = mingw32-g++.exe
CC = g++.exe
CFLAGS = -Wall -std=c++17
#CC = gcc.exe
#CFLAGS = -Wall
LIBS = -L../../lib -lOpenAL32 -lfftw3-3

export ECHO
export CC
export INCLUDES
export CFLAGS
export LIBS

all: $(OBJS)
	rm -rf build/mic/*
	rm -rf bin/*.exe
	rm -rf mic.exe
	mkdir -p build/mic/
	mkdir -p bin
	$(MAKE) -C ./src/mic all
#	$(MAKE) -C ./src/unit_tests all

clean:
	$(MAKE) -C ./src/mic clean
#	$(MAKE) -C ./src/unit_tests clean

