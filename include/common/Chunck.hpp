#ifndef __CHUNK__
#define __CHUNK__

#include "Common.hpp"
#include <SDL3/SDL_stdinc.h>
#include <sys/types.h>
#include <unordered_map>

struct DrawnFace {
  Vector3 blockPos;
  int side;
  int blockID;
  bool Transparent;
};

class ChunkPrefab {
public:
  static constexpr Uint8 xSize = 16;
  static constexpr Uint8 zSize = 16;
  static constexpr Uint8 ySize = 64;

  int xPos = -1;
  int zPos = -1;

  // Flat 3D array for much faster access (10-100x faster than unordered_map)
  std::vector<DrawnFace> allFaces;
  std::unordered_map<int, Uint8> Modifications;

  bool isDirty = false; // Track if chunk needs saving
  inline bool isValidPos(int x, int y, int z) const {
    return x >= 0 && x < xSize && y >= 0 && y < ySize && z >= 0 && z < zSize;
  }
  void GenerateChunk();

private:
  //void GenerateChunkCaves();
  
};

#endif