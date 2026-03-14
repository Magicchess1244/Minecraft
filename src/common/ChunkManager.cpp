#include "../../include/common/ChunkManager.hpp"
#include "../../include/common/Chunck.hpp"

#include <set>
#include <thread>

constexpr HeightsDif ContinentelnessHeight[] = {
    {0.8f, 130},                          // Peaks
    {0.8f, 180},                          // Peaks
    {0.8f, 160},                          // Peaks
    {0.5f, 130},                          // High Land
    {0.3f, 80},                           // Land
    {0.2f, (float)ChunkPrefab::SeaLevel}, // Shore (Sea Level)
    {0.1f, 70},                           // Land
    {0.0f, (float)ChunkPrefab::SeaLevel}, // Shore (Sea Level)
    {-0.2f, 40},                          // Shallow Ocean
    {-0.5f, 20},                          // Deep Ocean
    {-0.95f, 20},                         // Deep Ocean
    {-1.0f, 190}};

constexpr Biome Biomes[11] = {
    {"Ice", BiomeType::Ice, 30, 20, 0, 0, 20, 6},
    {"Tundra", BiomeType::Tundra, 50, 30, 15, 0, 20, 6},
    {"Taiga", BiomeType::Taiga, 100, 40, 60, 0, 20, 7},
    {"Big Taiga", BiomeType::BigTaiga, 100, 50, 70, 20, 20, 4},
    {"Plains", BiomeType::Plains, 50, 60, 30, 40, 20, 3},
    {"Forest", BiomeType::Forest, 80, 70, 50, 40, 20, 6},
    {"Birch", BiomeType::Birch, 80, 60, 50, 30, 20, 5},
    {"Dark Forest", BiomeType::DarkForest, 100, 60, 80, 30, 20, 5},
    {"Jungle", BiomeType::Jungle, 100, 100, 80, 60, 20, 7},
    {"Desert", BiomeType::Desert, 45, 100, 0, 70, 20, 4},
    {"Savanna", BiomeType::Savanna, 70, 95, 30, 60, 20, 5},
};

constexpr HeightsDif ErrotionHeight[10] = {
    {1.0f, (float)ChunkPrefab::ySize * 0.05f},
    {0.8f, (float)ChunkPrefab::ySize * 0.1f},
    {0.7f, (float)ChunkPrefab::ySize * 0.35f},
    {0.4f, (float)ChunkPrefab::ySize * 0.35f},
    {0.3f, (float)ChunkPrefab::ySize * 0.11f},
    {-0.2f, (float)ChunkPrefab::ySize * 0.2f},
    {-0.4f, (float)ChunkPrefab::ySize * 0.6f},
    {-0.5f, (float)ChunkPrefab::ySize * 0.5f},
    {-0.8f, (float)ChunkPrefab::ySize * 0.9f},
    {-1.0f, (float)ChunkPrefab::ySize},
};
constexpr HeightsDif PeaksAndValiesHeight[6] = {
    {1.0f, (float)ChunkPrefab::ySize},
    {0.8f, (float)ChunkPrefab::ySize * 0.9f},
    {0.5f, (float)ChunkPrefab::ySize * 0.95f},
    {0.05f, (float)ChunkPrefab::ySize * 0.35f},
    {-0.4f, (float)ChunkPrefab::ySize * 0.3f},
    {-0.9f, (float)ChunkPrefab::ySize * 0.1f},
};

constexpr HeightsDif CaveThickness[5] = {
    {(float)ChunkPrefab::ySize * 0.66f, 0.005f},
    {(float)ChunkPrefab::ySize * 0.33f, 0.015f},
    {(float)ChunkPrefab::SeaLevel * 0.5f, 0.025f},
    {10.0f, 0.015f},
    {0.0f, 0.005f}};

constexpr HeightsDif CoalChance[5] = {
    {(float)ChunkPrefab::ySize * 0.66f, 0.02f},
    {60.0f, 0.01f},
    {40.0f, 0.015f},
    {20.0f, 0.005f},
    {0.0f, 0.0f}};

constexpr HeightsDif IronChance[5] = {{(float)ChunkPrefab::ySize * 0.66f, 0.2f},
                                      {60.0f, 0.005f},
                                      {40.0f, 0.01f},
                                      {20.0f, 0.012f},
                                      {0.0f, 0.015f}};

constexpr HeightsDif DiamondChance[5] = {
    {(float)ChunkPrefab::ySize * 0.66f, 0.0f},
    {60.0f, 0.0f},
    {30.0f, 0.0f},
    {15.0f, 0.005f},
    {0.0f, 0.01f}};

float SampleSpline(float value, const HeightsDif *spline, int length) {
  if (value >= spline[0].x)
    return spline[0].y;
  for (int i = 0; i < length - 1; i++) {
    if (value >= spline[i + 1].x) {
      float t = (value - spline[i + 1].x) / (spline[i].x - spline[i + 1].x);
      return Lerp(spline[i + 1].y, spline[i].y, t);
    }
  }
  return spline[length - 1].y;
}

int GetBaseHeight(float Continentalness, float Erosion, float Peaks) {
  float base = SampleSpline(Continentalness, ContinentelnessHeight, 12);
  float erosionFactor = SampleSpline(Erosion, ErrotionHeight, 10);
  float peakFactor = SampleSpline(Peaks, PeaksAndValiesHeight, 6);

  // Combine them.
  // We can use the user's previous logic but enhanced.
  // For now, let's keep it simple: Continentalness is the base,
  // Erosion and Peaks modify it.

  float erosionMultiplier = (erosionFactor / (float)ChunkPrefab::ySize);
  float peakMultiplier = (peakFactor / (float)ChunkPrefab::ySize);

  return (int)(base + (erosionMultiplier * peakMultiplier * 20.0f));
}

float GetCaveThreshold(float y) { return SampleSpline(y, CaveThickness, 5); }

float GetCoalChance(float y) { return SampleSpline(y, CoalChance, 5); }
float GetIronChance(float y) { return SampleSpline(y, IronChance, 5); }
float GetDiamondChance(float y) { return SampleSpline(y, DiamondChance, 5); }

ChunkManager::ChunkManager() {
  // cache = new ChunkCache(); // Initialize cache system
}

ChunkManager::~ChunkManager() {
  /*
// Save all dirty chunks before destroying
for (auto &pair : Chunks) {
if (pair.second.isDirty) {
cache->saveChunk(pair.second, (int)pair.first.x, (int)pair.first.z);
}
}
delete cache;*/
}

ChunkPrefab &ChunkManager::get_chunk(Vector3 key) {
  key.y = 0;

  std::lock_guard<std::recursive_mutex> lock(chunks_mutex);
  auto it = Chunks.find(key);
  if (it == Chunks.end()) {
    auto newChunk = std::make_unique<ChunkPrefab>();
    newChunk->xPos = (int)key.x * ChunkPrefab::xSize;
    newChunk->zPos = (int)key.z * ChunkPrefab::zSize;
    newChunk->isDirty = true;
    newChunk->isGenerated = false;
    newChunk->manager = this;

    ChunkPrefab *ptr = newChunk.get();
    Chunks[key] = std::move(newChunk);

    std::thread(&ChunkPrefab::GenerateChunk, ptr).detach();
    return *ptr;
  }

  if (it->second->isGenerated && it->second->isDirty) {
    Vector3 currentKey = key;
    std::thread([this, currentKey]() {
      std::lock_guard<std::recursive_mutex> lock(chunks_mutex);
      Vector3 neighbors[] = {{currentKey.x + 1, 0, currentKey.z},
                             {currentKey.x - 1, 0, currentKey.z},
                             {currentKey.x, 0, currentKey.z + 1},
                             {currentKey.x, 0, currentKey.z - 1}};

      for (auto &nKey : neighbors) {
        auto nit = Chunks.find(nKey);
        if (nit != Chunks.end() && !nit->second->isDirty &&
            nit->second->isGenerated) {
          nit->second->PropagateLighting();
          nit->second->GenerateMesh();
          nit->second->needsMeshUpdate = true;
        }
      }
    }).detach();

    it->second->isDirty = false;
  }

  return *it->second;
}

Biome ChunkManager::GetBiome(float Humidity, float Temperature) {
  float minDistance = 1e9f;
  int bestBiome = 4; // Default to Plains

  for (int i = 0; i < 11; i++) {
    float centerH = (Biomes[i].MinHumidity + Biomes[i].MaxHumidity) * 0.5f;
    float centerT =
        (Biomes[i].MinTemperature + Biomes[i].MaxTemperature) * 0.5f;

    float dH = (Humidity - centerH);
    float dT = (Temperature - centerT);
    float distSq = dH * dH + dT * dT;

    if (distSq < minDistance) {
      minDistance = distSq;
      bestBiome = i;
    }
  }

  return Biomes[bestBiome];
}

int ChunkManager::GetHeight(float Continentalness, float Erosion, float Peaks) {
  return GetBaseHeight(Continentalness, Erosion, Peaks);
}

RaycastResult ChunkManager::RayCast(Vector3 Origin, Vector3 NormalDir,
                                    float MaxDistance) {
  Vector3 Dir = NormalDir;

  // Calculate number of steps based on MaxDistance
  int numSteps = (int)(MaxDistance * 10); // More checks for better precision

  Vector3 RelChunkPos = (Origin / (float)ChunkPrefab::xSize).Truncate();
  ChunkPrefab *CurrentChunk = &get_chunk(RelChunkPos);

  Vector3 LastPos = Origin.Truncate();

  for (int i = 0; i <= numSteps; i++) {
    float t = ((float)i / (float)numSteps) * MaxDistance;
    Vector3 NewPos = (Origin + Dir * t).Truncate();

    if (i > 0 && NewPos != LastPos) {
      Vector3 NewRelChunkPos = (NewPos / (float)ChunkPrefab::xSize).Truncate();
      if (NewRelChunkPos != RelChunkPos) {
        RelChunkPos = NewRelChunkPos;
        CurrentChunk = &get_chunk(NewRelChunkPos);
      }

      int Height = CurrentChunk->GetHeight({NewPos.x, NewPos.z});
      if (CurrentChunk->isSolidBlock((int)NewPos.x, (int)NewPos.y,
                                     (int)NewPos.z, Height)) {
        Uint8 blockID = CurrentChunk->GetBlockID((int)NewPos.x, (int)NewPos.y,
                                                 (int)NewPos.z, Height);
        return {true, NewPos, LastPos, blockID};
      }

      LastPos = NewPos;
    }
  }
  return {false, {0, 0, 0}, {0, 0, 0}};
}

bool ChunkManager::IsSolid(Vector3 worldPos) {
  Vector3 floorPos = worldPos.Truncate();
  Uint8 blockID = GetBlockID(floorPos);

  if (blockID == 0 || blockID == 5)
    return false;

  const Block &def = BlockDef[blockID];
  if (!def.IsSolid())
    return false;

  if (!def.collisionBoxes.has_value()) {
    // If collisionBoxes is nullptr, check as a full block
    return true;
  } else {
    // Check if the point is within any of the collision boxes (relative to
    // block)
    Vector3 relPos = worldPos - floorPos;
    // Assuming a max of 2 boxes for now as defined in the header previously
    for (int i = 0; i < def.collisionBoxes->size(); i++) {
      const BoundingBox &box = def.collisionBoxes.value()[i];
      if (relPos.x >= box.min.x && relPos.x <= box.max.x &&
          relPos.y >= box.min.y && relPos.y <= box.max.y &&
          relPos.z >= box.min.z && relPos.z <= box.max.z) {
        return true;
      }
    }
    return false;
  }
}

void ChunkManager::Place(Vector3 Pos, int BlockID) {
  if (Pos.y == 0)
    return; // Protect bedrock

  SetBlock(Pos, BlockID, true);

  // Water Simulation: If we place water, start simulation.
  // If we place air, check neighbors for water that might flow in.
  if (BlockID == 5) {
    AddActiveWater(Pos);
  } else if (BlockID == 0) {
    Vector3 neighbors[5] = {{Pos.x, Pos.y + 1, Pos.z},
                            {Pos.x + 1, Pos.y, Pos.z},
                            {Pos.x - 1, Pos.y, Pos.z},
                            {Pos.x, Pos.y, Pos.z + 1},
                            {Pos.x, Pos.y, Pos.z - 1}};
    for (auto &n : neighbors) {
      if (GetBlockID(n) == 5)
        AddActiveWater(n);
    }
  }
}

void ChunkManager::AddActiveWater(Vector3 pos) {
  // Check if already in list to avoid duplicates
  for (auto &p : activeWater) {
    if (p.first == pos)
      return;
  }
  activeWater.push_back({pos, 8}); // Level 8 is source
}

void ChunkManager::AddActiveWater(Vector3 pos, int level) {
  if (level <= 0)
    return;
  for (auto &p : activeWater) {
    if (p.first == pos) {
      if (p.second < level)
        p.second = level;
      return;
    }
  }
  activeWater.push_back({pos, level});
}

void ChunkManager::TickWater() {
  if (activeWater.empty())
    return;

  std::vector<std::pair<Vector3, int>> nextActive;
  std::vector<std::pair<Vector3, int>> toPlace;

  // Process a limited number of water blocks per tick to maintain performance
  int processed = 0;
  int limit = 200;

  for (auto const &water : activeWater) {
    Vector3 pos = water.first;
    int level = water.second;

    if (processed++ > limit) {
      nextActive.push_back(water);
      continue;
    }

    // Check if it's still water
    if (GetBlockID(pos) != 5)
      continue;

    // 1. Try flow down
    Vector3 down = {pos.x, pos.y - 1, pos.z};
    if (pos.y > 0 && GetBlockID(down) == 0) {
      toPlace.push_back({down, 5});
      nextActive.push_back(
          {down,
           8}); // Flowing down resets/maintains source power?
                // Actually usually it keeps same level or 8.
                // Let's use 8 for vertical flow to allow full spread below.
    } else if (level > 1) {
      // 2. Try flow sideways
      Vector3 sides[4] = {{pos.x + 1, pos.y, pos.z},
                          {pos.x - 1, pos.y, pos.z},
                          {pos.x, pos.y, pos.z + 1},
                          {pos.x, pos.y, pos.z - 1}};

      for (auto &s : sides) {
        if (GetBlockID(s) == 0) {
          toPlace.push_back({s, 5});
          nextActive.push_back({s, level - 1});
        }
      }
    }
  }

  // Apply changes using SetBlock to avoid redundant generation
  std::set<Vector3> chunksToUpdate;

  for (auto &tp : toPlace) {
    SetBlock(tp.first, tp.second,
             false); // Don't update neighbors during simulation to be faster

    Vector3 cKey = {(float)floor(tp.first.x / (float)ChunkPrefab::xSize), 0,
                    (float)floor(tp.first.z / (float)ChunkPrefab::zSize)};
    chunksToUpdate.insert(cKey);
  }

  // Now regenerate only the affected chunks once in background
  for (auto const &cKey : chunksToUpdate) {
    std::thread([this, cKey]() {
      std::lock_guard<std::recursive_mutex> lock(chunks_mutex);
      get_chunk(cKey).GenerateChunk();
    }).detach();
  }

  activeWater = nextActive;
}

void ChunkManager::SetBlock(Vector3 Pos, int BlockID, bool updateNeighbors) {
  std::lock_guard<std::recursive_mutex> lock(chunks_mutex);
  Vector3 chunkKey = {(float)floor(Pos.x / (float)ChunkPrefab::xSize), 0,
                      (float)floor(Pos.z / (float)ChunkPrefab::zSize)};
  chunkKey.y = 0;

  if (Pos.y < 0 || Pos.y >= ChunkPrefab::ySize)
    return;

  Modifications[Pos] = BlockID;

  auto it = Chunks.find(chunkKey);
  if (it == Chunks.end())
    return;

  // Update the block in the chunk immediately if it exists
  if (!it->second->blocks.empty()) {
    int lx = (int)floor(Pos.x) - it->second->xPos;
    int ly = (int)floor(Pos.y);
    int lz = (int)floor(Pos.z) - it->second->zPos;
    if (lx >= 0 && lx < ChunkPrefab::xSize && ly >= 0 &&
        ly < ChunkPrefab::ySize && lz >= 0 && lz < ChunkPrefab::zSize) {
      it->second->blocks[lx + ly * ChunkPrefab::xSize +
                         lz * ChunkPrefab::xSize * ChunkPrefab::ySize] =
          BlockID;

      // Re-calculate everything for current chunk
      it->second->GenerateLighting();
      it->second->PropagateLighting();
      it->second->GenerateMesh();
      it->second->needsMeshUpdate = true;
    }
  }

  if (updateNeighbors) {
    Vector3 neighbors[4] = {{chunkKey.x + 1, 0, chunkKey.z},
                            {chunkKey.x - 1, 0, chunkKey.z},
                            {chunkKey.x, 0, chunkKey.z + 1},
                            {chunkKey.x, 0, chunkKey.z - 1}};
    for (auto &nKey : neighbors) {
      auto nit = Chunks.find(nKey);
      if (nit != Chunks.end() && !nit->second->blocks.empty()) {
        nit->second->GenerateLighting();
        nit->second->PropagateLighting();
        nit->second->GenerateMesh();
        nit->second->needsMeshUpdate = true;
      }
    }
  }
}

Uint8 ChunkManager::GetBlockID(Vector3 Pos) {
  std::lock_guard<std::recursive_mutex> lock(chunks_mutex);
  Vector3 chunkKey = {(float)floor(Pos.x / (float)ChunkPrefab::xSize), 0,
                      (float)floor(Pos.z / (float)ChunkPrefab::zSize)};
  auto it = Chunks.find(chunkKey);

  // Safety check: Chunk must exist and have data
  if (it == Chunks.end() || !it->second->isGenerated ||
      it->second->blocks.empty()) {
    return (Pos.y < ChunkPrefab::SeaLevel) ? 5 : 0;
  }

  int lx = (int)floor(Pos.x) - it->second->xPos;
  int ly = (int)floor(Pos.y);
  int lz = (int)floor(Pos.z) - it->second->zPos;

  // Strict bounds check
  if (lx < 0 || lx >= ChunkPrefab::xSize || ly < 0 ||
      ly >= ChunkPrefab::ySize || lz < 0 || lz >= ChunkPrefab::zSize) {
    return 0; // Out of chunk bounds
  }

  int idx = lx + ly * ChunkPrefab::xSize +
            lz * ChunkPrefab::xSize * ChunkPrefab::ySize;

  // Double safety check on vector size
  if (idx < 0 || idx >= (int)it->second->blocks.size()) {
    return 0;
  }

  return it->second->blocks[idx];
}
Uint8 ChunkManager::GetLightLevel(Vector3 Pos) {
  std::lock_guard<std::recursive_mutex> lock(chunks_mutex);
  Vector3 chunkKey = {(float)floor(Pos.x / ChunkPrefab::xSize), 0,
                      (float)floor(Pos.z / ChunkPrefab::zSize)};

  auto it = Chunks.find(chunkKey);
  if (it == Chunks.end() || !it->second->isGenerated ||
      it->second->lightData.empty())
    return 0; // Prevent light leaking from unloaded chunks

  const ChunkPrefab &chunk = *it->second;

  int lx = (int)floor(Pos.x) - chunk.xPos;
  int ly = (int)floor(Pos.y);
  int lz = (int)floor(Pos.z) - chunk.zPos;

  if (lx < 0 || lx >= chunk.xSize || ly < 0 || ly >= chunk.ySize || lz < 0 ||
      lz >= chunk.zSize)
    return 0;

  int idx = lx + ly * chunk.xSize + lz * chunk.xSize * chunk.ySize;
  if (idx >= (int)chunk.lightData.size())
    return 0;

  return std::max(chunk.lightData[idx].sunlight,
                  chunk.lightData[idx].blockLight);
}
Uint8 ChunkManager::GetSunlightLevel(Vector3 Pos) {
  std::lock_guard<std::recursive_mutex> lock(chunks_mutex);
  Vector3 chunkKey = {(float)floor(Pos.x / ChunkPrefab::xSize), 0,
                      (float)floor(Pos.z / ChunkPrefab::zSize)};

  auto it = Chunks.find(chunkKey);
  if (it == Chunks.end() || !it->second->isGenerated ||
      it->second->lightData.empty())
    return 0;

  const ChunkPrefab &chunk = *it->second;
  int lx = (int)floor(Pos.x) - chunk.xPos;
  int ly = (int)floor(Pos.y);
  int lz = (int)floor(Pos.z) - chunk.zPos;

  if (lx < 0 || lx >= chunk.xSize || ly < 0 || ly >= chunk.ySize || lz < 0 ||
      lz >= chunk.zSize)
    return 0;

  int idx = lx + ly * chunk.xSize + lz * chunk.xSize * chunk.ySize;
  if (idx >= (int)chunk.lightData.size())
    return 0;

  return chunk.lightData[idx].sunlight;
}
Uint8 ChunkManager::GetBlockLightLevel(Vector3 Pos) {
  std::lock_guard<std::recursive_mutex> lock(chunks_mutex);
  Vector3 chunkKey = {(float)floor(Pos.x / ChunkPrefab::xSize), 0,
                      (float)floor(Pos.z / ChunkPrefab::zSize)};

  auto it = Chunks.find(chunkKey);
  if (it == Chunks.end() || !it->second->isGenerated ||
      it->second->lightData.empty())
    return 0;

  const ChunkPrefab &chunk = *it->second;
  int lx = (int)floor(Pos.x) - chunk.xPos;
  int ly = (int)floor(Pos.y);
  int lz = (int)floor(Pos.z) - chunk.zPos;

  if (lx < 0 || lx >= chunk.xSize || ly < 0 || ly >= chunk.ySize || lz < 0 ||
      lz >= chunk.zSize)
    return 0;

  int idx = lx + ly * chunk.xSize + lz * chunk.xSize * chunk.ySize;
  if (idx >= (int)chunk.lightData.size())
    return 0;

  return chunk.lightData[idx].blockLight;
}

/*
namespace ChunckManager {
        static bool isTransparent(int blockID)
        {
                return blockID == 0 || blockID == 5;
        }
        static bool canSwim(int blockID)
        {
                return blockID == 5;
        }
        bool Collition(Vector3& PlayerPos, int FullRange, int yRange, bool Swim,
bool Block)
        {
                int newX = (int)(PlayerPos.x + (int)(FullRange / 2.0f - 1));
                int newY = (int)(PlayerPos.y + (int)(yRange / 2.0f));
                int newZ = (int)(PlayerPos.z + (int)(FullRange / 2.0f - 1));

                Vector3 chunk = { floorf((float)(newX) / 32),
floorf((float)(newZ) / 32) }; int localX = (int)(newX % 32); int localZ =
(int)(newZ % 32);

                //std::cout << "Chunk: " << chunk << ", LocalX: " << localX <<
", FootY: " << footY << ", HeadY: " << headY << ", Block Info: " <<
Chunks[chunk].Blocks[localX][footY] << std::endl;

                bool blockHead = !isTransparent(Chunks[{(int)chunk.x,
(int)chunk.z}].Blocks[{localX, newY, localZ}]);

                if (Swim && !blockHead) {
                        blockHead = canSwim(Chunks[{(int)chunk.x,
(int)chunk.z}].Blocks[{localX, newY, newZ}]);
                }

                if (Block) {
                        blockHead = false;
                }

                return blockHead;
        }
        bool PlaceBlock(int BlockType, Vector3 Position, int yRange, Vector3
PlayerPosition, short& Type)
        {

                Position.x = (int)(Position.x / BlockSize) + PlayerPosition.x;
                Position.y = (yRange - (int)(Position.y / BlockSize) - 1) +
PlayerPosition.y;


                Vector3 CurrrentChunk = { floorf((Position.x) / 32),
floorf((Position.z) / 32) }; int RelativeX = (int)(Position.x) % 32; int
RelativeZ = (int)(Position.z) % 32;


                bool notBedRock =
(Chunks[(int)CurrrentChunk.x][(int)CurrrentChunk.z].Blocks[RelativeX][(int)Position.y][RelativeZ]
!= 4); bool water =
(Chunks[(int)CurrrentChunk.x][(int)CurrrentChunk.z].Blocks[RelativeX][(int)Position.y][RelativeZ]
== 5); bool canPlace = (BlockType != 0 &&
(Chunks[(int)CurrrentChunk.x][(int)CurrrentChunk.z].Blocks[RelativeX][(int)Position.y]
== 0 || water)); bool canBreak = (BlockType == 0 &&
(Chunks[(int)CurrrentChunk.x][(int)CurrrentChunk.z].Blocks[RelativeX][(int)Position.y]
!= 0 && !water));

                //std::cout << canBreak << " " << canPlace << std::endl;

                if (notBedRock && (canPlace || canBreak)) {
                        std::cout << "Placing Block: " << BlockType << ",
Position: " << Position.x << ", " << Position.y << "Chunk: " << CurrrentChunk.x
<< "; " << CurrrentChunk.z << std::endl; Type =
(short)Chunks[(int)CurrrentChunk.x][(int)CurrrentChunk.z].Blocks[RelativeX][(int)Position.y];
                        //Chunks[CurrrentChunk].Blocks[RelativeX][(int)Position.y]
= BlockType; return true;
                }
                return false;
        }
        void Size(int PixelSizeX, int PixelSizeY, int yRange, int FullRange)
        {
                if (PixelSizeX > PixelSizeY)
                {
                        BlockSize = (int)(PixelSizeY / yRange);
                }
                else
                {
                        BlockSize = (int)(PixelSizeX / FullRange);
                }
        }
        void ShowInventor(SDL_Renderer* Renderer, int width, int height,
std::vector<Slot>& Inventory, int InventorySlot, TTF_Font* font)
        {
                SDL_SetRenderDrawColor(Renderer, 157, 76, 0, 255);
                SDL_FRect InventoryRect = { (float)(width / 2) - (BlockSize *
4), (height - (BlockSize * 1.3f)), (BlockSize * 1.1f) * 8 + (BlockSize * .1f),
(BlockSize * 1.2f) }; SDL_RenderFillRect(Renderer, &InventoryRect);

                for (int i = 0; i < 8; i++)
                {
                        if (InventorySlot == i)
                        {
                                SDL_SetRenderDrawColor(Renderer, 255, 153, 56,
255);
                        }
                        else
                        {
                                SDL_SetRenderDrawColor(Renderer, 204, 102, 0,
255);
                        }
                        SDL_FRect InventoryRect = { (float)(((width / 2) -
(BlockSize * 4)) + (BlockSize * 1.1f * i) + (BlockSize * 0.1f)), (float)((height
- (BlockSize * 1.3f)) + (BlockSize * 0.1f)), (float)BlockSize, (float)BlockSize
}; SDL_RenderFillRect(Renderer, &InventoryRect);

                        if (Inventory[i].Type != 0)
                        {
                                SDL_SetRenderDrawColor(Renderer,
BlockDef[Inventory[i].Type].Color.r, BlockDef[Inventory[i].Type].Color.g,
BlockDef[Inventory[i].Type].Color.b, 1); SDL_FRect BlockRect = { ((width / 2) -
(BlockSize * 4)) + (BlockSize * 1.1f * i) + (BlockSize * 0.3f), (height -
(BlockSize * 1.3f)) + (BlockSize * 0.3f), BlockSize * 0.6f, BlockSize * 0.6f };
                                SDL_RenderFillRect(Renderer, &BlockRect);
                                //Add be a number
                                if (Inventory[i].Amount > 1)
                                {
                                        std::string text =
std::to_string(Inventory[i].Amount); SDL_Color White = { 200, 200, 200 };
                                        SDL_Surface* surface =
TTF_RenderText_Blended(font, text.c_str(), sizeof(text.c_str()), White);
                                        SDL_Texture* texture =
SDL_CreateTextureFromSurface(Renderer, surface); SDL_DestroySurface(surface);
                                        SDL_FRect dstRect{ (float)(((width / 2)
- (BlockSize * 4)) + (BlockSize * 1.1f * i) + (BlockSize * 0.8f)),
(float)((height - (BlockSize * 1.1f)) + (BlockSize * 0.4f)), BlockSize * 0.5f ,
BlockSize * 0.5f}; SDL_RenderTexture(Renderer, texture, NULL, &dstRect);
                                        SDL_DestroyTexture(texture);
                                }
                        }

                }
        }
        void SimulateWater(int chunkIndex) {
                std::queue<std::pair<int, int> > q;

                for (int x = 0; x < 16; x++) {
                        for (int y = 0; y < 64; y++) {
                                if (Chunks[chunkIndex].Blocks[x][y] == 5) {
                                        int globalX = chunkIndex * 16 + x;
                                        q.push(std::make_pair(globalX, y));
                                }
                        }
                }

                while (!q.empty()) {
                        std::pair<int, int> pos = q.front();
                        q.pop();

                        int x = pos.first;
                        int y = pos.second;

                        int chunk = x / 16;
                        int localX = x % 16;

                        // Move down
                        if (y - 1 >= 0 && (Chunks[chunk].Blocks[localX][y - 1]
== 0 || Chunks[chunk].Blocks[localX][y - 1] == 5)) {
                                Chunks[chunk].Blocks[localX][y - 1] = 5;
                                q.push(std::make_pair(x, y - 1));
                        }
                        else {
                                // Try left
                                if (x - 1 >= 0) {
                                        int leftChunk = (x - 1) / 16;
                                        int leftX = (x - 1) % 16;
                                        if (Chunks[leftChunk].Blocks[leftX][y]
== 0) { Chunks[leftChunk].Blocks[leftX][y] = 5; q.push(std::make_pair(x - 1,
y));
                                        }
                                }
                                // Try right
                                if (x + 1 < 1600) {
                                        int rightChunk = (x + 1) / 16;
                                        int rightX = (x + 1) % 16;
                                        if (Chunks[rightChunk].Blocks[rightX][y]
== 0) { Chunks[rightChunk].Blocks[rightX][y] = 5; q.push(std::make_pair(x + 1,
y));
                                        }
                                }
                        }
                }
        }
}
*/
Uint8 ChunkManager::GetMod(Vector3 Pos) {
  std::lock_guard<std::recursive_mutex> lock(chunks_mutex);
  auto it = Modifications.find(Pos);
  if (it != Modifications.end())
    return it->second;
  return 255;
}

void ChunkManager::GetModificationsForChunk(
    int xStart, int zStart, std::unordered_map<int, Uint8> &localMods) {
  std::lock_guard<std::recursive_mutex> lock(chunks_mutex);
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
