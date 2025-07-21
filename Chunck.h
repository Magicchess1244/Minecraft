#ifndef __CHUNK__
#define __CHUNK__

#include "common.hpp"
#include "PerlinNoise.h"

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

    //Biome biome;

    std::unordered_map<Vector3, int> Blocks;
    std::vector<DrawnFace> allFaces;

    ChunkPrefab() = default;
    void GenerateChunk();

private:
    void GenerateChunkSurface();
    void GenerateChunkCaves();
    void VisableFaces();
};

#endif