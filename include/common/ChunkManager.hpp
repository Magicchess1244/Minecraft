#ifndef __CHUNKMANAGER_HPP__
#define __CHUNKMANAGER_HPP__

#include "BlockDef.hpp"
#include "Common.hpp"
#include <SDL3/SDL_stdinc.h>

class ChunkPrefab;
class ChunkCache;

typedef struct {
  float x;
  float y;
} HeightsDif;

float SampleSpline(float value, const HeightsDif *spline, int length);
int GetBaseHeight(float Continentalness, float Erosion, float Peaks);
float GetCaveThreshold(float y);
float GetCoalChance(float y);
float GetIronChance(float y);
float GetDiamondChance(float y);

enum class BiomeType {
  Ice,
  Tundra,
  Taiga,
  BigTaiga,
  Plains,
  Forest,
  Birch,
  DarkForest,
  Jungle,
  Desert,
  Savanna
};

typedef struct {
  const char *Name;
  BiomeType Type;
  int MaxHumidity;
  int MaxTemperature;
  int MinHumidity;
  int MinTemperature;
  int BaseHeight;
  int ChangeAmount;
} Biome;

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
  std::vector<std::pair<Vector3, int>> activeWater;
  float waterTickTimer = 0.0f;

public:
  ChunkManager();
  ~ChunkManager();
  int DayLightLevel = 15;

  void TickWater();
  void AddActiveWater(Vector3 pos);
  void AddActiveWater(Vector3 pos, int level);

  ChunkPrefab &get_chunk(Vector3 key);
  Biome GetBiome(float Humudity, float Temperature);
  int GetHeight(float Continentalness, float Errotion, float PeakAndVallies);
  Block GetBlock(int BlockId) {
    if (BlockId < 0 || BlockId >= (int)BlockDefinitionsAmount) {
      std::cerr << "Invalid Block ID: " << BlockId << std::endl;
      return BlockDefinitions[0];
    }
    return BlockDefinitions[BlockId];
  }
  Uint8 GetMod(Vector3 Pos) {
    auto it = Modifications.find(Pos);
    if (it != Modifications.end())
      return it->second;
    return 255;
  }
  void GetModificationsForChunk(int xStart, int zStart,
                                std::unordered_map<int, Uint8> &localMods) {
    for (auto const &[pos, blockID] : Modifications) {
      if (pos.x >= xStart && pos.x < xStart + 16 && pos.z >= zStart &&
          pos.z < zStart + 16 && pos.y >= 0 && pos.y < 192) {
        int lx = (int)pos.x - xStart;
        int ly = (int)pos.y;
        int lz = (int)pos.z - zStart;
        localMods[lx + ly * 16 + lz * 16 * 192] = blockID;
      }
    }
  }
  RaycastResult RayCast(Vector3 Origin, Vector3 NormalDir, float MaxDistance);
  bool IsSolid(Vector3 worldPos);
  void Place(Vector3 Pos, int BlockID);
  void SetBlock(Vector3 Pos, int BlockID, bool updateNeighbors = true);
  Uint8 GetBlockID(Vector3 Pos);
  Uint8 GetLightLevel(Vector3 Pos);
  Uint8 GetSunlightLevel(Vector3 Pos);
  Uint8 GetBlockLightLevel(Vector3 Pos);
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