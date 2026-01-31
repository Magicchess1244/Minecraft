#ifndef __CHUNKMANAGER_HPP__
#define __CHUNKMANAGER_HPP__

#include "Common.hpp"
#include <unordered_map>

class ChunkPrefab;
class ChunkCache;

typedef struct {
  char Name[20];
  short BlockId;
  Color color;
  short SpawningLayer[2];
  bool Top;
  bool Water;
} Block;
typedef struct {
  int MaxHumidity;
  int MaxTemperature;
  int MinHumidity;
  int MinTemperature;
  int BaseHeight;
  int ChangeAmount;
} Biome;
typedef struct {
  float x;
  float y;
} HeightsDif;

constexpr int BlockNum = 6;
const Block BlockDef[BlockNum] = {
    {"Air", 0, {255, 178, 255}, {0, 0}, false, false},
    {"Grass", 1, {0, 166, 0}, {0, 0}, true, false},
    {"Dirt", 2, {153, 76, 25}, {1, 3}, true, false},
    {"Stone", 3, {128, 128, 128}, {4, 64}, false, false},
    {"Bedrock", 4, {51, 51, 51}, {0, 3}, false, false},
    {"Water", 5, {0, 102, 204}, {0, 0}, false, true}};

class ChunkManager {
private:
  std::unordered_map<Vector3, ChunkPrefab> Chunks;
  ChunkCache *cache; // Chunk caching system

public:
  ChunkManager();
  ~ChunkManager();

  ChunkPrefab &get_chunk(Vector3 key);
  int BaseHeight(float ValueNoise, int Length, const HeightsDif *Heights);
  Biome GetBiome(float Humudity, float Temperature);
  int GetHeight(float Continentalness, float Errotion, float PeakAndVallies);
  Block GetBlock(int BlockId) {
    if (BlockId < 0 || BlockId >= BlockNum) {
      std::cerr << "Invalid Block ID: " << BlockId << std::endl;
      return BlockDef[0];
    }
    return BlockDef[BlockId];
  }
};
/*
namespace ChunckManager {
        bool Collition(Vector3& PlayerPos, int FullRange, int yRange, bool Swim,
bool Block); bool PlaceBlock(int BlockType, Vector3 Position, int yRange,
Vector3 PlayerPosition, short& Type); void Size(int PixelSizeX, int PixelSizeY,
int yRange, int FullRange); void ShowInventor(SDL_Renderer* Renderer, int width,
int height, std::vector<Slot>& Inventory, int InventorySlot, TTF_Font* font);
        void SimulateWater(int chunkIndex);
};
*/
#endif