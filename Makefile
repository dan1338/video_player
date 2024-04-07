XX = g++
CXXFLAGS = -Wall -Wreturn-type -Wextra -Wshadow -std=c++11 -Iinclude -O2 #-fsanitize=address

# Get libavcodec flags using pkg-config
AVCODEC_PKGS := libavcodec libavformat libavutil 
AVCODEC_CFLAGS := $(shell pkg-config --cflags $(AVCODEC_PKGS))
AVCODEC_LIBS := $(shell pkg-config --libs $(AVCODEC_PKGS))

OPENGL_PKGS := glfw3 glew
OPENGL_CFLAGS := $(shell pkg-config --cflags $(OPENGL_PKGS))
OPENGL_LIBS := $(shell pkg-config --libs $(OPENGL_PKGS))

# Source files
SRCS = $(wildcard src/*.cpp)

# Object files
OBJS = $(SRCS:src/%.cpp=build/%.o)

# Executable name
TARGET = main

all: prebuild $(TARGET)

prebuild:
	mkdir -p build

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -pthread $(AVCODEC_LIBS) $(OPENGL_LIBS) -o $@ 

build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $(AVCODEC_CFLAGS) $(OPENGL_CFLAGS) -c $< -o $@

clean:
	-@rm -r build $(TARGET)

.PHONY: clean

