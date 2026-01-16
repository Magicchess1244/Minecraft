# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -O3 -pthread # Optimized build with threading support
LDFLAGS := -lSDL3 -lSDL3_ttf -pthread

# Include directories
INCLUDES := -Iinclude/common -Iinclude/client -Iinclude/server

# Targets
CLIENT_TARGET := minecraft3d-client
SERVER_TARGET := minecraft3d-server

# Source directories
SRC_COMMON := src/common
SRC_CLIENT := src/client
SRC_SERVER := src/server

# Common source files
COMMON_SRCS := $(SRC_COMMON)/Chunk.cpp $(SRC_COMMON)/ChunkManager.cpp \
               $(SRC_COMMON)/PerlinNoise.cpp $(SRC_COMMON)/ChunkCache.cpp
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

.PHONY: all shaders client server clean

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
	glslc -fshader-stage=$(if $(findstring vert,$*),vertex,fragment) $< -o $@

clean:
	rm -f $(ALL_OBJS) $(CLIENT_TARGET) $(SERVER_TARGET) $(SHADER_SPV)