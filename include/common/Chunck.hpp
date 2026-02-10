#ifndef __CHUNK__
#define __CHUNK__

#include "../../include/common/ChunkManager.hpp"
#include "Common.hpp"
#include <SDL3/SDL_stdinc.h>
#include <sys/types.h>

struct DrawnFace {
  Uint16 blockPos;
  Uint8 side;
  Uint8 blockID;
  bool Transparent;
};
struct TransparentDrawnFace {
  Vector3 blockPos;
  int side;
  int blockID;
};

class ChunkPrefab {
public:
  static constexpr Uint8 xSize = 16;
  static constexpr Uint8 zSize = 16;
  static constexpr Uint8 ySize = 64;
  static constexpr float Frecuence = 0.03f;
  static constexpr Uint8 BaseHeight = 35;
  static constexpr Uint8 HeightVar = 30;

  int xPos = -1;
  int zPos = -1;

  // Flat 3D array for much faster access (10-100x faster than unordered_map)
  std::vector<DrawnFace> allFaces;

  bool isDirty = false; // Track if chunk needs saving
  bool needsMeshUpdate = true;
  inline bool isValidPos(int x, int y, int z) const {
    return x >= 0 && x < xSize && y >= 0 && y < ySize && z >= 0 && z < zSize;
  }
  void GenerateChunk(ChunkManager &manager);
  bool isSolidBlock(int worldX, int worldY, int worldZ, int terrainHeight, ChunkManager &manager);
  int GetHeight(Vector2 Pos);

private:
  // void GenerateChunkCaves();
  void VisableFaces();
};

#endif