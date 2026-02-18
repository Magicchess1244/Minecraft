# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++17 -O3 -pthread
SDLIMAGE_STATIC := ~/SDL_image/build/libSDL3_image_static.a
LDFLAGS := $(SDLIMAGE_STATIC) ~/SDL/build/libSDL3.a -lpng -lz -ljpeg -lwebp -ltiff -pthread -lm -ldl

# Debug flags for memory leak detection
CXXFLAGS_DEBUG := -std=c++17 -g -O0 -pthread -fsanitize=address
LDFLAGS_DEBUG := $(SDLIMAGE_STATIC) -lSDL3 -lpng -lz -ljpeg -lwebp -ltiff -pthread -lm -ldl -fsanitize=address

# Include directories
INCLUDES := -Iinclude/common -Iinclude/client -Iinclude/server -I/usr/include/SDL3 -I/usr/local/include/SDL3

# Targets
CLIENT_TARGET := minecraft3d-client
SERVER_TARGET := minecraft3d-server

# Source directories
SRC_COMMON := src/common
SRC_CLIENT := src/client
SRC_SERVER := src/server

# Common source files
COMMON_SRCS := $(SRC_COMMON)/Chunk.cpp $(SRC_COMMON)/ChunkManager.cpp \
               $(SRC_COMMON)/PerlinNoise.cpp
COMMON_OBJS := $(COMMON_SRCS:.cpp=.o)
COMMON_OBJS_DEBUG := $(patsubst %.cpp,%.debug.o,$(COMMON_SRCS))

# Client source files
CLIENT_SRCS := $(SRC_CLIENT)/ClientMain.cpp $(SRC_CLIENT)/GameClient.cpp \
               $(SRC_CLIENT)/Renderer.cpp
CLIENT_OBJS := $(CLIENT_SRCS:.cpp=.o)
CLIENT_OBJS_DEBUG := $(patsubst %.cpp,%.debug.o,$(CLIENT_SRCS))

# Server source files
SERVER_SRCS := $(SRC_SERVER)/ServerMain.cpp $(SRC_SERVER)/GameServer.cpp
SERVER_OBJS := $(SERVER_SRCS:.cpp=.o)
SERVER_OBJS_DEBUG := $(patsubst %.cpp,%.debug.o,$(SERVER_SRCS))

# All object files
ALL_OBJS := $(COMMON_OBJS) $(CLIENT_OBJS) $(SERVER_OBJS)
ALL_OBJS_DEBUG := $(COMMON_OBJS_DEBUG) $(CLIENT_OBJS_DEBUG) $(SERVER_OBJS_DEBUG)

# Shader files
SHADER_SPV := assets/shaders/Shader.vert.spv assets/shaders/Shader.frag.spv \
              assets/shaders/UI.vert.spv assets/shaders/UI.frag.spv

.PHONY: all shaders client server client-debug server-debug clean

# Default build (release)
all: shaders client server

client: $(CLIENT_TARGET)

server: $(SERVER_TARGET)

# Compile shaders
shaders: $(SHADER_SPV)

# Release builds
$(CLIENT_TARGET): $(CLIENT_OBJS) $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(SERVER_TARGET): $(SERVER_OBJS) $(COMMON_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Debug builds (AddressSanitizer)
client-debug: $(CLIENT_TARGET)-debug

server-debug: $(SERVER_TARGET)-debug

$(CLIENT_TARGET)-debug: $(CLIENT_OBJS_DEBUG) $(COMMON_OBJS_DEBUG)
	$(CXX) $(CXXFLAGS_DEBUG) -o $@ $^ $(LDFLAGS_DEBUG)

$(SERVER_TARGET)-debug: $(SERVER_OBJS_DEBUG) $(COMMON_OBJS_DEBUG)
	$(CXX) $(CXXFLAGS_DEBUG) -o $@ $^ $(LDFLAGS_DEBUG)

# Compile source files (release)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile source files (debug)
%.debug.o: %.cpp
	$(CXX) $(CXXFLAGS_DEBUG) $(INCLUDES) -c $< -o $@

# Compile vertex shaders
assets/shaders/%.vert.spv: assets/shaders/%.vert.glsl
	glslc -fshader-stage=vertex $< -o $@

# Compile fragment shaders
assets/shaders/%.frag.spv: assets/shaders/%.frag.glsl
	glslc -fshader-stage=fragment $< -o $@

# Clean
clean:
	rm -f $(ALL_OBJS) $(ALL_OBJS_DEBUG)
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET)
	rm -f $(CLIENT_TARGET)-debug $(SERVER_TARGET)-debug
	rm -f $(SHADER_SPV)