CXX := g++
CXXFLAGS := -std=c++17 -O2 -g3 -pthread -gdwarf-4 -fPIC \
	-Wno-deprecated -pipe -fno-elide-type -fdiagnostics-show-template-tree \
	-Wall -Werror -Wextra -Wpedantic -Wvla -Wextra-semi -Wnull-dereference \
	-Wswitch-enum -fvar-tracking-assignments -Wduplicated-cond -Wduplicated-branches \
	-rdynamic -Wsuggest-override

LDFLAGS := -lSDL3 -lSDL3_ttf

# Targets
CLIENT_TARGET := client.exe
SERVER_TARGET := server.exe

# Source files (with subdirs)
CLIENT_SRCS := Client.cpp client/GameClient.cpp client/Renderer.cpp \
               core/Chunk.cpp core/ChunkManager.cpp
SERVER_SRCS := Server.cpp server/GameServer.cpp server/PerlinNoise.cpp \
               core/Chunk.cpp core/ChunkManager.cpp

CLIENT_OBJS := $(CLIENT_SRCS:.cpp=.o)
SERVER_OBJS := $(SERVER_SRCS:.cpp=.o)

DEPS := core/Chunk.hpp core/ChunkManager.hpp \
        client/GameClient.hpp client/Renderer.hpp \
        server/GameServer.hpp server/PerlinNoise.hpp \
        net/common.hpp core/common.hpp

.PHONY: all clean client server

all: $(CLIENT_TARGET) $(SERVER_TARGET)

client: $(CLIENT_TARGET)
server: $(SERVER_TARGET)

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Generic rule: build .o from .cpp regardless of subdir
%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(CLIENT_OBJS) $(SERVER_OBJS) $(CLIENT_TARGET) $(SERVER_TARGET)
