#ifndef __CHUNK__
#define __CHUNK__

#include "BiomeBuilder.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <tuple>

namespace std {
    template<>
    struct hash<std::tuple<int, int, int>> {
        size_t operator()(const std::tuple<int, int, int>& t) const noexcept {
            int x = std::get<0>(t);
            int y = std::get<1>(t);
            int z = std::get<2>(t);
            return ((hash<int>()(x) ^ (hash<int>()(y) << 1)) >> 1) ^ (hash<int>()(z) << 1);
        }
    };
}

class ChunkPrefab {
public:
    static constexpr int xSize = 32;
    static constexpr int zSize = 32;
    static constexpr int ySize = 64;

    int xPos = -1;
    int zPos = -1;

    Biome biome;

    std::unordered_map<std::tuple<int, int, int>, int> Blocks;

    ChunkPrefab() = default;
    void ShowChunk();
    void GenerateChunk();

private:
    void GenerateChunkSurface();
    void GenerateChunkCaves();
};

#endif