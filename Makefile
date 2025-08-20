CXX := g++
CXXFLAGS := -std=c++17 -O2 -g3 -pthread -gdwarf-4 -fPIC -Wno-deprecated -pipe -fno-elide-type -fdiagnostics-show-template-tree -Wall -Werror -Wextra -Wpedantic -Wvla -Wextra-semi -Wnull-dereference -Wswitch-enum -fvar-tracking-assignments -Wduplicated-cond -Wduplicated-branches -rdynamic -Wsuggest-override
LDFLAGS := -lSDL3 -lSDL3_ttf
TARGET := 2Dminecraft

SRCS := Main.cpp GameClient.cpp GameServer.cpp Chunk.cpp ChunkManager.cpp PerlinNoise.cpp Renderer.cpp
OBJS := $(SRCS:.cpp=.o)
DEPS := Chunk.h ChunkManager.h GameClient.h GameServer.h PerlinNoise.h Renderer.h common.hpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
