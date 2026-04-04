#include "../../include/common/ChunkManager.hpp"
#include "../../include/common/Chunck.hpp"

#include <set>

constexpr HeightsDif ContinentalnessHeight[] = {
    {0.8f, 130},  {0.8f, 180},
    {0.8f, 160},  {0.5f, 130},
    {0.3f, 80},   {0.2f, (float)ChunkPrefab::SeaLevel},
    {0.1f, 70},   {0.0f, (float)ChunkPrefab::SeaLevel},
    {-0.2f, 40},  {-0.5f, 20},
    {-0.95f, 20}, {-1.0f, 190},
};

constexpr HeightsDif ErosionHeight[] = {
    {1.0f, (float)ChunkPrefab::ySize * 0.05f},
    {0.8f, (float)ChunkPrefab::ySize * 0.10f},
    {0.7f, (float)ChunkPrefab::ySize * 0.35f},
    {0.4f, (float)ChunkPrefab::ySize * 0.35f},
    {0.3f, (float)ChunkPrefab::ySize * 0.11f},
    {-0.2f, (float)ChunkPrefab::ySize * 0.20f},
    {-0.4f, (float)ChunkPrefab::ySize * 0.60f},
    {-0.5f, (float)ChunkPrefab::ySize * 0.50f},
    {-0.8f, (float)ChunkPrefab::ySize * 0.90f},
    {-1.0f, (float)ChunkPrefab::ySize},
};

constexpr HeightsDif PeaksAndValleysHeight[] = {
    {1.0f, (float)ChunkPrefab::ySize},
    {0.8f, (float)ChunkPrefab::ySize * 0.90f},
    {0.5f, (float)ChunkPrefab::ySize * 0.95f},
    {0.05f, (float)ChunkPrefab::ySize * 0.35f},
    {-0.4f, (float)ChunkPrefab::ySize * 0.30f},
    {-0.9f, (float)ChunkPrefab::ySize * 0.10f},
};

constexpr HeightsDif CaveThickness[] = {
    {(float)ChunkPrefab::ySize * 0.66f, 0.005f},
    {(float)ChunkPrefab::ySize * 0.33f, 0.015f},
    {(float)ChunkPrefab::SeaLevel * 0.5f, 0.025f},
    {10.0f, 0.015f},
    {0.0f, 0.005f},
};

constexpr HeightsDif CoalChance[] = {
    {(float)ChunkPrefab::ySize * 0.66f, 0.020f},
    {60.0f, 0.010f},
    {40.0f, 0.015f},
    {20.0f, 0.005f},
    {0.0f, 0.000f},
};

constexpr HeightsDif IronChance[] = {
    {(float)ChunkPrefab::ySize * 0.66f, 0.200f},
    {60.0f, 0.005f},
    {40.0f, 0.010f},
    {20.0f, 0.012f},
    {0.0f, 0.015f},
};

constexpr HeightsDif DiamondChance[] = {
    {(float)ChunkPrefab::ySize * 0.66f, 0.000f},
    {60.0f, 0.000f},
    {30.0f, 0.000f},
    {15.0f, 0.005f},
    {0.0f, 0.010f},
};

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
  float base = SampleSpline(Continentalness, ContinentalnessHeight, 12);
  float erosionFactor = SampleSpline(Erosion, ErosionHeight, 10);
  float peakFactor = SampleSpline(Peaks, PeaksAndValleysHeight, 6);

  float erosionMult = erosionFactor / (float)ChunkPrefab::ySize;
  float peakMult = peakFactor / (float)ChunkPrefab::ySize;

  return (int)(base + (erosionMult * peakMult * 20.0f));
}

float GetCaveThreshold(float y) { return SampleSpline(y, CaveThickness, 5); }
float GetCoalChance(float y) { return SampleSpline(y, CoalChance, 5); }
float GetIronChance(float y) { return SampleSpline(y, IronChance, 5); }
float GetDiamondChance(float y) { return SampleSpline(y, DiamondChance, 5); }

ChunkPrefab &ChunkManager::get_chunk(Vector3 chunkKey) {
  chunkKey.y = 0;

  std::lock_guard<std::recursive_mutex> lock(chunks_mutex);

  auto it = Chunks.find(chunkKey);
  if (it == Chunks.end()) {
    auto newChunk = std::make_unique<ChunkPrefab>();
    newChunk->xPos = (int)chunkKey.x * ChunkPrefab::xSize;
    newChunk->zPos = (int)chunkKey.z * ChunkPrefab::zSize;
    newChunk->isDirty = true;
    newChunk->isGenerated = false;
    newChunk->manager = this;

    ChunkPrefab *ptr = newChunk.get();
    Chunks[chunkKey] = std::move(newChunk);

    pool.submit([ptr]() { ptr->GenerateChunk(); });
    return *ptr;
  }

  ChunkPrefab &chunk = *it->second;

  if (chunk.isGenerated && chunk.isDirty) {
    chunk.isDirty = false;
    pool.submit([this, chunkKey]() { refresh_ready_neighbours(chunkKey); });
  }

  return chunk;
}

void ChunkManager::refresh_ready_neighbours(Vector3 centerKey) {
  const Vector3 offsets[] = {
      {centerKey.x + 1, 0, centerKey.z},
      {centerKey.x - 1, 0, centerKey.z},
      {centerKey.x, 0, centerKey.z + 1},
      {centerKey.x, 0, centerKey.z - 1},
  };

  std::vector<ChunkPrefab *> readyNeighbours;
  {
    std::lock_guard<std::recursive_mutex> lock(chunks_mutex);
    for (const auto &key : offsets) {
      auto it = Chunks.find(key);
      if (it != Chunks.end() && it->second->isGenerated && !it->second->isDirty)
        readyNeighbours.push_back(it->second.get());
    }
  }

  for (ChunkPrefab *neighbour : readyNeighbours) {
    bool expected = false;
    if (!neighbour->isProcessing.compare_exchange_strong(expected, true))
      continue; // already being processed
    neighbour->GenerateLighting();
    neighbour->PropagateLighting();
    neighbour->GenerateMesh();

    neighbour->lightData.clear();
    neighbour->lightData.shrink_to_fit();

    neighbour->needsMeshUpdate = true;
    neighbour->isProcessing = false;
  }
}

// ─── Biome ───────────────────────────────────────────────────────────────────

Biome ChunkManager::GetBiome(float humidity, float temperature) {
  float minDistSq = 1e9f;
  int bestBiome = 4; // default: Plains

  for (int i = 0; i < 11; i++) {
    float centerH = (Biomes[i].MinHumidity + Biomes[i].MaxHumidity) * 0.5f;
    float centerT =
        (Biomes[i].MinTemperature + Biomes[i].MaxTemperature) * 0.5f;

    float dH = humidity - centerH;
    float dT = temperature - centerT;
    float distSq = dH * dH + dT * dT;

    if (distSq < minDistSq) {
      minDistSq = distSq;
      bestBiome = i;
    }
  }

  return Biomes[bestBiome];
}

// ─── Raycast ─────────────────────────────────────────────────────────────────

RaycastResult ChunkManager::RayCast(Vector3 origin, Vector3 direction,
                                    float maxDistance) {
  int numSteps = (int)(maxDistance * 10);
  Vector3 chunkKey = worldToChunkKey(origin);
  ChunkPrefab *chunk = &get_chunk(chunkKey);
  Vector3 lastPos = origin.Truncate();

  for (int i = 1; i <= numSteps; i++) {
    float t = ((float)i / (float)numSteps) * maxDistance;
    Vector3 newPos = (origin + direction * t).Truncate();

    if (newPos == lastPos)
      continue;

    Vector3 newKey = worldToChunkKey(newPos);
    if (newKey != chunkKey) {
      chunkKey = newKey;
      chunk = &get_chunk(newKey);
    }

    int height = chunk->GetHeight({newPos.x, newPos.z});
    if (chunk->isSolidBlock((int)newPos.x, (int)newPos.y, (int)newPos.z,
                            height)) {
      Uint8 blockID = chunk->GetBlockID((int)newPos.x, (int)newPos.y,
                                        (int)newPos.z, height);
      return {true, newPos, lastPos, blockID};
    }

    lastPos = newPos;
  }

  return {false, {0, 0, 0}, {0, 0, 0}};
}

// ─── IsSolid ─────────────────────────────────────────────────────────────────

bool ChunkManager::IsSolid(Vector3 worldPos) {
  Vector3 floorPos = worldPos.Truncate();
  Uint8 blockID = GetBlockID(floorPos);

  if (blockID == 0 || blockID == 5)
    return false;

  const Block &def = BlockDef[blockID];
  if (!def.IsSolid())
    return false;

  if (!def.collisionBoxes.has_value())
    return true; // full solid cube

  // Custom collision box(es)
  Vector3 relPos = worldPos - floorPos;
  for (const BoundingBox &box : def.collisionBoxes.value()) {
    if (relPos.x >= box.min.x && relPos.x <= box.max.x &&
        relPos.y >= box.min.y && relPos.y <= box.max.y &&
        relPos.z >= box.min.z && relPos.z <= box.max.z)
      return true;
  }
  return false;
}

// ─── Block placement
// ──────────────────────────────────────────────────────────

void ChunkManager::Place(Vector3 worldPos, int blockID) {
  if (worldPos.y == 0)
    return;

  SetBlock(worldPos, blockID, true);

  if (blockID == 5) {
    // Placed water: activate it
    AddActiveWater(worldPos);
  } else if (blockID == 0) {
    // Removed a block: re-activate any adjacent water
    const Vector3 neighbors[] = {
        {worldPos.x, worldPos.y + 1, worldPos.z},
        {worldPos.x + 1, worldPos.y, worldPos.z},
        {worldPos.x - 1, worldPos.y, worldPos.z},
        {worldPos.x, worldPos.y, worldPos.z + 1},
        {worldPos.x, worldPos.y, worldPos.z - 1},
    };
    for (const auto &n : neighbors) {
      if (GetBlockID(n) == 5)
        AddActiveWater(n);
    }
  }
}

// ─── SetBlock ────────────────────────────────────────────────────────────────

void ChunkManager::SetBlock(Vector3 worldPos, int blockID,
                            bool updateNeighbours) {
  if (worldPos.y < 0 || worldPos.y >= ChunkPrefab::ySize)
    return;

  Vector3 chunkKey = worldToChunkKey(worldPos);
  ChunkPrefab *ownerChunk = nullptr;
  std::vector<ChunkPrefab *> neighbourChunks;

  {
    std::lock_guard<std::recursive_mutex> lock(chunks_mutex);

    auto it = Chunks.find(chunkKey);
    if (it == Chunks.end())
      return;

    ChunkPrefab &chunk = *it->second;

    // Record the modification persistently (blockID 0 = air/removed is valid).
    Modifications[worldPos] = (Uint8)blockID;

    // Also write directly into blocks[] so rebuild_chunk (GenerateMesh etc.)
    // sees the change immediately without needing a full GenerateChunk pass.
    int lx = (int)std::floor(worldPos.x) - chunk.xPos;
    int ly = (int)std::floor(worldPos.y);
    int lz = (int)std::floor(worldPos.z) - chunk.zPos;
    if (lx >= 0 && lx < ChunkPrefab::xSize && ly >= 0 &&
        ly < ChunkPrefab::ySize && lz >= 0 && lz < ChunkPrefab::zSize) {
      chunk.blocks[lx + ly * ChunkPrefab::xSize +
                   lz * ChunkPrefab::xSize * ChunkPrefab::ySize] =
          (Uint8)blockID;
    }

    ownerChunk = &chunk;

    if (updateNeighbours) {
      const Vector3 offsets[] = {
          {chunkKey.x + 1, 0, chunkKey.z},
          {chunkKey.x - 1, 0, chunkKey.z},
          {chunkKey.x, 0, chunkKey.z + 1},
          {chunkKey.x, 0, chunkKey.z - 1},
      };
      for (const auto &key : offsets) {
        auto nit = Chunks.find(key);
        if (nit != Chunks.end())
          neighbourChunks.push_back(nit->second.get());
      }
    }
  }

  if (ownerChunk)
    rebuild_chunk(*ownerChunk);

  for (ChunkPrefab *neighbour : neighbourChunks)
    rebuild_chunk(*neighbour);
}

// ─── try_set_block_local ─────────────────────────────────────────────────────

// Writes directly into chunk.blocks without touching Modifications.
// Returns false if worldPos is outside the chunk's bounds.
bool ChunkManager::try_set_block_local(ChunkPrefab &chunk, Vector3 worldPos,
                                       int blockID) {
  int lx = (int)std::floor(worldPos.x) - chunk.xPos;
  int ly = (int)std::floor(worldPos.y);
  int lz = (int)std::floor(worldPos.z) - chunk.zPos;

  if (lx < 0 || lx >= ChunkPrefab::xSize || ly < 0 ||
      ly >= ChunkPrefab::ySize || lz < 0 || lz >= ChunkPrefab::zSize)
    return false;

  chunk.blocks[lx + ly * ChunkPrefab::xSize +
               lz * ChunkPrefab::xSize * ChunkPrefab::ySize] = blockID;
  return true;
}

// ─── rebuild_chunk
// ────────────────────────────────────────────────────────────

void ChunkManager::rebuild_chunk(ChunkPrefab &chunk) {
  bool expected = false;
  if (!chunk.isProcessing.compare_exchange_strong(expected, true))
    return; // another thread is already rebuilding this chunk

  chunk.GenerateLighting();
  chunk.PropagateLighting();
  chunk.GenerateMesh();

  // Clear light data after mesh generation
  chunk.lightData.clear();
  chunk.lightData.shrink_to_fit();

  chunk.needsMeshUpdate = true;
  chunk.isProcessing = false;
}

// ─── Block / light queries
// ────────────────────────────────────────────────────

// Internal helper: locks, finds the chunk, validates bounds, and fills out the
// local (lx,ly,lz) coordinates.  Returns true on success.
// MUST be called with chunks_mutex already held.
static bool resolve_world_to_local(
    const std::unordered_map<Vector3, std::unique_ptr<ChunkPrefab>> &chunks,
    Vector3 worldPos, const ChunkPrefab **outChunk, int &lx, int &ly, int &lz) {
  Vector3 chunkKey = worldToChunkKey(worldPos);
  auto it = chunks.find(chunkKey);

  if (it == chunks.end() || !it->second->isGenerated)
    return false;

  const ChunkPrefab &chunk = *it->second;

  lx = (int)std::floor(worldPos.x) - chunk.xPos;
  ly = (int)std::floor(worldPos.y);
  lz = (int)std::floor(worldPos.z) - chunk.zPos;

  if (lx < 0 || lx >= ChunkPrefab::xSize || ly < 0 ||
      ly >= ChunkPrefab::ySize || lz < 0 || lz >= ChunkPrefab::zSize)
    return false;

  *outChunk = &chunk;
  return true;
}

Uint8 ChunkManager::GetBlockID(Vector3 worldPos) {
  std::lock_guard<std::recursive_mutex> lock(chunks_mutex);

  const ChunkPrefab *chunk;
  int lx, ly, lz;
  if (!resolve_world_to_local(Chunks, worldPos, &chunk, lx, ly, lz)) {
    return (worldPos.y < ChunkPrefab::SeaLevel) ? 5 : 0;
  }

  int idx = lx + ly * ChunkPrefab::xSize +
            lz * ChunkPrefab::xSize * ChunkPrefab::ySize;

  return chunk->blocks[idx];
}

Uint8 ChunkManager::GetLightLevel(Vector3 worldPos) {
  std::lock_guard<std::recursive_mutex> lock(chunks_mutex);

  const ChunkPrefab *chunk;
  int lx, ly, lz;
  if (!resolve_world_to_local(Chunks, worldPos, &chunk, lx, ly, lz))
    return 0;

  // After mesh generation, lightData is cleared and lighting is in faces
  if (chunk->lightData.empty()) {
    return chunk->GetLightFromFaces(lx, ly, lz);
  }

  int idx = lx + ly * ChunkPrefab::xSize +
            lz * ChunkPrefab::xSize * ChunkPrefab::ySize;

  return std::max(chunk->lightData[idx].sunlight,
                  chunk->lightData[idx].blockLight);
}

// ─── Modifications
// ────────────────────────────────────────────────────────────

// Returns the player-placed block at worldPos, or 255 if the position is
// unmodified.
Uint8 ChunkManager::GetMod(Vector3 worldPos) {
  std::lock_guard<std::recursive_mutex> lock(chunks_mutex);
  auto it = Modifications.find(worldPos);
  return (it != Modifications.end()) ? it->second : 255;
}

// Fills localMods with every modification that falls within the chunk whose
// world-space origin is (xStart, *, zStart), using the flat block index as key.
void ChunkManager::GetModificationsForChunk(
    int xStart, int zStart, std::unordered_map<int, Uint8> &localMods) {
  std::lock_guard<std::recursive_mutex> lock(chunks_mutex);

  for (const auto &[pos, blockID] : Modifications) {
    if (pos.x < xStart || pos.x >= xStart + ChunkPrefab::xSize)
      continue;
    if (pos.z < zStart || pos.z >= zStart + ChunkPrefab::zSize)
      continue;
    if (pos.y < 0 || pos.y >= ChunkPrefab::ySize)
      continue;

    int lx = (int)pos.x - xStart;
    int ly = (int)pos.y;
    int lz = (int)pos.z - zStart;
    localMods[lx + ly * ChunkPrefab::xSize +
              lz * ChunkPrefab::xSize * ChunkPrefab::ySize] = blockID;
  }
}

// ─── Water simulation
// ─────────────────────────────────────────────────────────

// Adds a source-level water block (level 8) at pos, ignoring duplicates.
void ChunkManager::AddActiveWater(Vector3 pos) {
  for (const auto &p : activeWater) {
    if (p.first == pos)
      return;
  }
  activeWater.push_back({pos, 8});
}

// Adds water at the given level; upgrades the level if a lower one already
// exists.
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

  // Process a limited number of water blocks per tick to keep performance
  // smooth.
  const int limit = 200;
  int processed = 0;

  for (const auto &[pos, level] : activeWater) {
    if (processed++ > limit) {
      nextActive.push_back({pos, level});
      continue;
    }

    // Skip if this block is no longer water.
    if (GetBlockID(pos) != 5)
      continue;

    Vector3 down = {pos.x, pos.y - 1, pos.z};
    if (pos.y > 0 && GetBlockID(down) == 0) {
      // Flow downward; vertical flow keeps full power (level 8).
      toPlace.push_back({down, 5});
      nextActive.push_back({down, 8});
    } else if (level > 1) {
      // Flow sideways at reduced level.
      const Vector3 sides[] = {
          {pos.x + 1, pos.y, pos.z},
          {pos.x - 1, pos.y, pos.z},
          {pos.x, pos.y, pos.z + 1},
          {pos.x, pos.y, pos.z - 1},
      };
      for (const auto &s : sides) {
        if (GetBlockID(s) == 0) {
          toPlace.push_back({s, 5});
          nextActive.push_back({s, level - 1});
        }
      }
    }
  }

  // Apply all block changes and collect the unique set of chunks to rebuild.
  std::set<Vector3> chunksToRebuild;
  for (const auto &[pos, bid] : toPlace) {
    SetBlock(pos, bid, false); // skip neighbour updates during simulation
    chunksToRebuild.insert(worldToChunkKey(pos));
  }

  // Kick off a background regeneration for each affected chunk.
  for (const auto &key : chunksToRebuild) {
    ChunkPrefab *ptr = nullptr;
    {
      std::lock_guard<std::recursive_mutex> lock(chunks_mutex);
      auto it = Chunks.find(key);
      if (it != Chunks.end())
        ptr = it->second.get();
    }
    if (ptr) {
      pool.submit([ptr]() {
        bool expected = false;
        if (ptr->isProcessing.compare_exchange_strong(expected, true)) {
          ptr->GenerateChunk();
          // GenerateChunk resets isProcessing when it finishes.
        }
      });
    }
  }

  activeWater = std::move(nextActive);
}

void ChunkManager::UnloadFarChunks(Vector3 playerPos, float maxDistance) {
  std::lock_guard<std::recursive_mutex> lock(chunks_mutex);
  float maxDistSq = maxDistance * maxDistance;

  for (auto it = Chunks.begin(); it != Chunks.end();) {
    Vector3 chunkPos = {(float)it->second->xPos + ChunkPrefab::xSize / 2.0f, 0,
                        (float)it->second->zPos + ChunkPrefab::zSize / 2.0f};
    if ((chunkPos - playerPos).LengthSquared() > maxDistSq) {
      it = Chunks.erase(it);
    } else {
      ++it;
    }
  }
}
