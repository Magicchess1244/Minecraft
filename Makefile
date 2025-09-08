## MinGW Makefile for Windows (SDL3 static/dynamic)
SHELL := cmd

# Compiler and flags
CXX := g++

# SDL root (adjust if installed elsewhere)
SDL_DIR := C:\Users\pumu\CLionProjects\Minecraft3D

# Library directories and includes
LIBDIRS := -L"$(SDL_DIR)" -L.
INCLUDES := -Iinclude/common -Iinclude/client -Iinclude/server -I"$(SDL_DIR)\include"

# Compiler flags
CXXFLAGS := -std=c++17 -O2 -g3 -gdwarf-4 -pipe -Wall -Wextra -Wno-unused-parameter

# Note: link ws2_32 for WinSock. Requires MinGW import libs: libSDL3.dll.a, libSDL3_ttf.dll.a in LIBDIRS
LDFLAGS := $(LIBDIRS) -lSDL3 -lSDL3_ttf -lws2_32 -static-libgcc -static-libstdc++

# Targets
CLIENT_TARGET := minecraft3d-client
SERVER_TARGET := minecraft3d-server

# Source directories
SRC_COMMON := src/common
SRC_CLIENT := src/client
SRC_SERVER := src/server

# Common source files
COMMON_SRCS := $(SRC_COMMON)/Chunk.cpp $(SRC_COMMON)/ChunkManager.cpp \
               $(SRC_COMMON)/PerlinNoise.cpp $(SRC_COMMON)/Physics.cpp \
               $(SRC_COMMON)/BlockTypes.cpp
COMMON_OBJS := $(COMMON_SRCS:.cpp=.o)

# Client source files
CLIENT_SRCS := $(SRC_CLIENT)/ClientMain.cpp $(SRC_CLIENT)/GameClient.cpp \
               $(SRC_CLIENT)/Renderer.cpp
CLIENT_OBJS := $(CLIENT_SRCS:.cpp=.o)

# Server source files
SERVER_SRCS := $(SRC_SERVER)/ServerMain.cpp $(SRC_SERVER)/GameServer.cpp
SERVER_OBJS := $(SERVER_SRCS:.cpp=.o)

# All object files
ALL_OBJS := $(COMMON_OBJS) $(CLIENT_OBJS) $(SERVER_OBJS)

# Shader files
SHADER_SRCS := assets/shaders/Shader.vert.glsl assets/shaders/Shader.frag.glsl
SHADER_SPV := assets/shaders/Shader.vert.spv assets/shaders/Shader.frag.spv

.PHONY: all client server clean shaders

all: shaders client server

client: shaders $(CLIENT_TARGET)

server: $(SERVER_TARGET)

shaders: $(SHADER_SPV)

$(CLIENT_TARGET): $(CLIENT_OBJS) $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(SERVER_TARGET): $(SERVER_OBJS) $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile shaders
assets/shaders/%.spv: assets/shaders/%.glsl
	where glslc >NUL 2>NUL && glslc -fshader-stage=$(if $(findstring vert,$*),vertex,fragment) $< -o $@ || echo Using precompiled shader $@

clean:
	-del /Q $(subst /,\\,$(ALL_OBJS)) 2>NUL || exit 0
	-del /Q $(CLIENT_TARGET) $(SERVER_TARGET) 2>NUL || exit 0
	-del /Q $(subst /,\\,$(SHADER_SPV)) 2>NUL || exit 0
