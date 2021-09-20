INCLUDES = -I../../inc/mic -I../../inc -I../../inc/openal -I../../inc/fftw
SOURCES = $(wildcard *.cpp)
OBJS = $(SOURCES:%.cpp=../../build/mic/%.o)
OBJ_NAME = ../../bin/mic.exe

../../build/mic/%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ $(LIBS)

all: $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) -o $(OBJ_NAME) $(LIBS)

clean:
	rm -f ../../build/mic/*
	rm -f $(OBJ_NAME)

