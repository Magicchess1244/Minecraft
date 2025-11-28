#ifndef __CHUNK__
#define __CHUNK__

#include "Common.hpp"

struct DrawnFace {
  Vector3 blockPos;
  int side;
  int blockID;
  float maxZ;
  bool Transparent;
};

class ChunkPrefab {
public:
  static constexpr int xSize = 32;
  static constexpr int zSize = 32;
  static constexpr int ySize = 64;

  int xPos = -1;
  int zPos = -1;
  bool isLoaded = false;

  // Biome biome;

  std::vector<int> Blocks;
  std::vector<DrawnFace> allFaces;

  ChunkPrefab() { Blocks.resize(xSize * ySize * zSize, 0); }
  void GenerateChunk();
  void RebuildMesh() { VisableFaces(); }

  int GetBlock(int x, int y, int z) const {
    if (x < 0 || x >= xSize || y < 0 || y >= ySize || z < 0 || z >= zSize)
      return 0;
    return Blocks[x + z * xSize + y * xSize * zSize];
  }

  void SetBlock(int x, int y, int z, int id) {
    if (x < 0 || x >= xSize || y < 0 || y >= ySize || z < 0 || z >= zSize)
      return;
    Blocks[x + z * xSize + y * xSize * zSize] = id;
  }

private:
  void GenerateChunkSurface();
  void GenerateChunkCaves();
  void VisableFaces();
};

#endif