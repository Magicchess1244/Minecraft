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
  Uint8 LightLevel;
  bool Transparent;
};
struct TransparentDrawnFace {
  Vector3 blockPos;
  int side;
  int blockID;
  Uint8 light;
};
struct LightData {
  Uint8 sunlight : 4;   // 0-15
  Uint8 blockLight : 4; // 0-15
};

class ChunkPrefab {
public:
  static constexpr int xSize = 16;
  static constexpr int zSize = 16;
  static constexpr int ySize = 192;
  static constexpr int SeaLevel = 64;
  static constexpr float Frecuence = 0.03f;
  static constexpr int BaseHeight = 64;
  static constexpr int HeightVar = 40;

  int xPos = -1;
  int zPos = -1;

  // Flat 3D array for much faster access (10-100x faster than unordered_map)
  std::vector<DrawnFace> allFaces;
  std::vector<Uint8> blocks; // Persistent block data for collision/raycasting
  std::vector<LightData> lightData; // Same size as blocks

  bool isDirty = false; // Track if chunk needs saving
  bool needsMeshUpdate = true;
  inline bool isValidPos(int x, int y, int z) const {
    return x >= 0 && x < xSize && y >= 0 && y < ySize && z >= 0 && z < zSize;
  }
  void GenerateChunk(ChunkManager &manager);
  bool isSolidBlock(int worldX, int worldY, int worldZ, int terrainHeight);
  Uint8 GetBlockID(int worldX, int worldY, int worldZ, int terrainHeight);
  int GetHeight(Vector2 Pos);
  void GenerateMesh();
  void GenerateLighting();
  void PropagateLighting();

  ChunkManager *manager = nullptr;

private:
  void GenerateVegetation(const std::vector<int> &heightCache,
                          const std::vector<Biome> &biomeMap,
                          std::vector<bool> &solidCache);

  // Tree models
  void PlaceStandardTree(int x, int y, int z, int trunkHeight,
                         std::vector<bool> &solidCache);
  void PlacePineTree(int x, int y, int z, int trunkHeight,
                     std::vector<bool> &solidCache);
  void PlaceLargeTree(int x, int y, int z, int trunkHeight,
                      std::vector<bool> &solidCache);
  void PlaceSavannaTree(int x, int y, int z, int trunkHeight,
                        std::vector<bool> &solidCache);
  void PlaceJungleTree(int x, int y, int z, int trunkHeight,
                       std::vector<bool> &solidCache);

  Uint8 GetCombinedLight(int x, int y, int z);
};

#endif