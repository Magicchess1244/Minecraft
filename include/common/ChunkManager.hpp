#ifndef __CHUNKMANAGER_HPP__
#define __CHUNKMANAGER_HPP__

#include "Common.hpp"
#include <SDL3/SDL_stdinc.h>

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
    {"Grass", 1, {255, 255, 255}, {0, 0}, true, false},
    {"Dirt", 2, {255, 255, 255}, {1, 3}, true, false},
    {"Stone", 3, {255, 255, 255}, {4, 64}, false, false},
    {"Bedrock", 4, {255, 255, 255}, {0, 3}, false, false},
    {"Water", 5, {0, 102, 204}, {0, 0}, false, true}};

struct RaycastResult {
  bool hit;
  Vector3 pos;
  Vector3 prevPos;
  Uint8 BlockID;
};
int BaseHeight(float ValueNoise, int Length, const HeightsDif *Heights);

class ChunkManager {
private:
  std::unordered_map<Vector3, ChunkPrefab> Chunks;
  ChunkCache *cache; // Chunk caching system
  std::unordered_map<Vector3, Uint8> Modifications;

public:
  ChunkManager();
  ~ChunkManager();

  ChunkPrefab &get_chunk(Vector3 key);
  Biome GetBiome(float Humudity, float Temperature);
  int GetHeight(float Continentalness, float Errotion, float PeakAndVallies);
  Block GetBlock(int BlockId) {
    if (BlockId < 0 || BlockId >= BlockNum) {
      std::cerr << "Invalid Block ID: " << BlockId << std::endl;
      return BlockDef[0];
    }
    return BlockDef[BlockId];
  }
  Uint8 GetMod(Vector3 Pos) {
    if (Modifications.find(Pos) != Modifications.end())
      return Modifications.find(Pos)->second;
    return 255; // I am sending 255 to indicate its not found might have to
                // change if I add more blocks
  }
  RaycastResult RayCast(Vector3 Origin, Vector3 NormalDir, float MaxDistance);
  bool IsSolid(Vector3 worldPos);
  void Place(Vector3 Pos, int BlockID);
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