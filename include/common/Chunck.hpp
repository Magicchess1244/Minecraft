#ifndef __CHUNK__
#define __CHUNK__

#include "Common.hpp"
#include <array>
#include <string>

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
  static constexpr int totalBlocks = xSize * ySize * zSize;

  int xPos = -1;
  int zPos = -1;

  // Flat 3D array for much faster access (10-100x faster than unordered_map)
  std::array<int, totalBlocks> blocks;
  std::vector<DrawnFace> allFaces;

  bool isDirty = false; // Track if chunk needs saving

  ChunkPrefab() {
    blocks.fill(0); // Initialize all blocks to air
  }

  // Fast block access helpers
  inline int &getBlock(int x, int y, int z) {
    return blocks[x + y * xSize + z * xSize * ySize];
  }

  inline int getBlock(int x, int y, int z) const {
    return blocks[x + y * xSize + z * xSize * ySize];
  }

  inline bool isValidPos(int x, int y, int z) const {
    return x >= 0 && x < xSize && y >= 0 && y < ySize && z >= 0 && z < zSize;
  }

  // Serialization
  bool saveToFile(const std::string &path);
  bool loadFromFile(const std::string &path);

  void GenerateChunk();
  void VisableFaces(); // Made public so ChunkManager can call it after loading

private:
  void GenerateCollum(int x, int z);
  void GenerateChunkSurface();
  void GenerateChunkCaves();
};

#endif