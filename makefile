# Platform detection (default to linux)
PLATFORM ?= linux
# Compiler and flags
ifeq ($(PLATFORM), windows)
CXX := x86_64-w64-mingw32-g++
EXTENSION := .exe
# For Windows, we assume libraries are available via mingw or in the project
# You might need to adjust these paths if you have them in specific folders
LDFLAGS := -lSDL3_ttf -lSDL3_image -lSDL3 -lpng -lz -ljpeg -lwebp -ltiff -pthread -lm -lws2_32 -liphlpapi
INCLUDES := -Iinclude/common -Iinclude/client -Iinclude/server
else
CXX := g++
EXTENSION :=
SDLIMAGE_STATIC := ~/SDL_image/build/libSDL3_image_static.a
SDL_LIB := ~/SDL/build/libSDL3.a
LDFLAGS := -lSDL3_ttf $(SDLIMAGE_STATIC) $(SDL_LIB) -lpng -lz -ljpeg -lwebp -ltiff -pthread -lm -ldl
INCLUDES := -Iinclude/common -Iinclude/client -Iinclude/server -I/usr/include/SDL3 -I/usr/local/include/SDL3
endif
CXXFLAGS := -std=c++17 -O3 -pthread -MMD -MP
# Debug flags for memory leak detection
CXXFLAGS_DEBUG := -std=c++17 -g -O0 -pthread -fsanitize=address -MMD -MP
LDFLAGS_DEBUG := $(LDFLAGS) -fsanitize=address
# Targets Base Names
CLIENT_BASE := minecraft3d-client
SERVER_BASE := minecraft3d-server
# Final Target Names
CLIENT_TARGET := $(CLIENT_BASE)$(EXTENSION)
SERVER_TARGET := $(SERVER_BASE)$(EXTENSION)
CLIENT_DEBUG_TARGET := $(CLIENT_BASE)-debug$(EXTENSION)
SERVER_DEBUG_TARGET := $(SERVER_BASE)-debug$(EXTENSION)
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
$(SRC_CLIENT)/Renderer.cpp $(SRC_CLIENT)/GameManager.cpp $(SRC_CLIENT)/PlayerManager.cpp
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
              assets/shaders/UI.vert.spv assets/shaders/UI.frag.spv \
              assets/shaders/Text.vert.spv assets/shaders/Text.frag.spv
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
client-debug: $(CLIENT_DEBUG_TARGET)
server-debug: $(SERVER_DEBUG_TARGET)
$(CLIENT_DEBUG_TARGET): $(CLIENT_OBJS_DEBUG) $(COMMON_OBJS_DEBUG)
	$(CXX) $(CXXFLAGS_DEBUG) -o $@ $^ $(LDFLAGS_DEBUG)
$(SERVER_DEBUG_TARGET): $(SERVER_OBJS_DEBUG) $(COMMON_OBJS_DEBUG)
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
	rm -f $(CLIENT_DEBUG_TARGET) $(SERVER_DEBUG_TARGET)
	rm -f $(SHADER_SPV)
	rm -f $(ALL_OBJS:.o=.d) $(ALL_OBJS_DEBUG:.debug.o=.debug.d)

# Include generated dependency files
-include $(ALL_OBJS:.o=.d)
-include $(ALL_OBJS_DEBUG:.debug.o=.debug.d)