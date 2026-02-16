#ifndef __CHUNK__
#define __CHUNK__

#include "../../include/common/ChunkManager.hpp"
#include "Common.hpp"
#include <SDL3/SDL_stdinc.h>
#include <sys/types.h>

struct DrawnFace {
  Vector3 blockPos;
  Uint8 side;
  Uint8 blockID;
  Uint8 w;
  Uint8 h;
  bool Transparent;
};
struct TransparentDrawnFace {
  Vector3 blockPos;
  int side;
  int blockID;
  int w;
  int h;
};

class ChunkPrefab {
public:
  static constexpr Uint8 xSize = 16;
  static constexpr Uint8 zSize = 16;
  static constexpr Uint8 ySize = 128;
  static constexpr float Frecuence = 0.03f;
  static constexpr Uint8 BaseHeight = 35;
  static constexpr Uint8 HeightVar = 30;

  int xPos = -1;
  int zPos = -1;

  // Flat 3D array for much faster access (10-100x faster than unordered_map)
  std::vector<DrawnFace> allFaces;
  std::vector<Uint8> blocks; // Persistent block data for collision/raycasting

  bool isDirty = false; // Track if chunk needs saving
  bool needsMeshUpdate = true;
  inline bool isValidPos(int x, int y, int z) const {
    return x >= 0 && x < xSize && y >= 0 && y < ySize && z >= 0 && z < zSize;
  }
  void GenerateChunk(ChunkManager &manager);
  bool isSolidBlock(int worldX, int worldY, int worldZ, int terrainHeight,
                    ChunkManager &manager);
  Uint8 GetBlockID(int worldX, int worldY, int worldZ, int terrainHeight,
                   ChunkManager &manager);
  int GetHeight(Vector2 Pos);

private:
  void GenerateHeightMap(std::vector<int> &heightCache);
  void GenerateCaveMap(std::vector<float> &caveDensityCache);
  void GenerateModMap(std::vector<Uint8> &modCache, ChunkManager &manager);
  void PopulateBlocks(const std::vector<int> &heightCache,
                      const std::vector<float> &caveDensityCache,
                      const std::vector<Uint8> &modCache,
                      std::vector<bool> &solidCache, ChunkManager &manager);
  void SimulateWaterSpread(std::vector<bool> &solidCache);
  void GenerateVegetation(const std::vector<int> &heightCache,
                          const std::vector<Uint8> &modCache,
                          std::vector<bool> &solidCache);
  void GenerateMesh(ChunkManager &manager);
};

#endif