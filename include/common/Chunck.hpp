#pragma once

#include "ChunkManager.hpp"
#include "Common.hpp"
#include <atomic>

struct DrawnFace {
  Uint32 data;

  static Uint32 Pack(Uint16 posIndex, Uint8 side, Uint8 light, Uint16 blockID) {
    return (Uint32)(posIndex & 0xFFFF) | ((Uint32)(side & 0x7) << 16) |
           ((Uint32)(light & 0x7) << 19) | ((Uint32)(blockID & 0x3FF) << 22);
  }

  static void Unpack(Uint32 packed, Uint16 &posIndex, Uint8 &side, Uint8 &light,
                     Uint16 &blockID) {
    posIndex = (Uint16)(packed & 0xFFFF);
    side = (Uint8)((packed >> 16) & 0x7);
    light = (Uint8)((packed >> 19) & 0x7);
    blockID = (Uint16)((packed >> 22) & 0x3FF);
  }
};
struct LightData {
  Uint8 sunlight : 4;   // 0-15
  Uint8 blockLight : 4; // 0-15
};
class ChunkManager;
class Biome;

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

  std::vector<Uint32> opaqueFaces;
  std::vector<Uint32> transparentFaces;
  std::vector<Uint8> blocks;
  std::vector<LightData> lightData;

  std::atomic<bool> isGenerated{false};
  std::atomic<bool> isDirty{false};
  std::atomic<bool> needsMeshUpdate{true};
  std::atomic<bool> isProcessing{false};
  inline bool isValidPos(int x, int y, int z) const {
    return x >= 0 && x < xSize && y >= 0 && y < ySize && z >= 0 && z < zSize;
  }
  void GenerateChunk();
  bool isSolidBlock(int worldX, int worldY, int worldZ, int terrainHeight);
  Uint8 GetBlockID(int worldX, int worldY, int worldZ, int terrainHeight);
  int GetHeight(Vector2 Pos);
  void GenerateMesh();
  void GenerateLighting();
  void PropagateLighting();

  ChunkManager *manager = nullptr;
  ChunkPrefab() {}
  ~ChunkPrefab() {}

private:
  void GenerateVegetation(const std::vector<int> &heightCache,
                          const std::vector<Biome> &biomeMap,
                          std::vector<bool> &solidCache);

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

  static void PrecomputeInterpolation(std::vector<float> &caveTr,
                                      std::vector<float> &coalCh,
                                      std::vector<float> &ironCh,
                                      std::vector<float> &diamCh);
  void GenerateHeightAndBiomes(std::vector<int> &heightCache,
                               std::vector<Biome> &biomeCache);
  void PopulateBlocks(const std::vector<int> &heightCache,
                      const std::vector<Biome> &biomeCache,
                      std::vector<bool> &solidCache);
  void GenerateCaves(std::vector<bool> &caveMap,
                     std::vector<float> &caveTrCache);
};