#ifndef __CHUNK__
#define __CHUNK__

#include "common.hpp"

struct DrawnFace {
    Vector3 blockPos;
    int side;
    int blockID;
    double maxZ;
};

class ChunkPrefab {
public:
    static constexpr int xSize = 32;
    static constexpr int zSize = 32;
    static constexpr int ySize = 64;

    int xPos = -1;
    int zPos = -1;

    Biome biome;

    std::unordered_map<std::tuple<int, int, int>, int> Blocks;
    std::vector<DrawnFace> allFaces;

    ChunkPrefab() = default;
    void ShowChunk();
    void GenerateChunk();

private:
    void GenerateChunkSurface();
    void GenerateChunkCaves();
    void VisableFaces();
};

#endif