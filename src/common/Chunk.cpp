#include "../../include/common/Chunck.hpp"
#include "../../include/common/ChunkManager.hpp"
#include "../../include/common/PerlinNoise.hpp"
#include <SDL3/SDL_stdinc.h>
#include <cstdlib>
#include <cstring>

constexpr float CaveThreshold = -0.18f;
constexpr int CaveMinY = 5;
constexpr int CaveMaxY = 100;

const Vector3 Direction[6] = {
    {0, 0, 1},  // Front
    {0, 0, -1}, // Back
    {1, 0, 0},  // Right
    {-1, 0, 0}, // Left
    {0, 1, 0},  // Top
    {0, -1, 0}  // Bottom
};

int ChunkPrefab::GetHeight(Vector2 Pos) {
  float cont = PerlinNoise2D(Pos, 3, 0.005); // Lowered period
  float eros = PerlinNoise2D(Pos, 3, 0.015f);
  float peak = PerlinNoise2D(Pos, 4, 0.02f);

  return GetBaseHeight(cont, eros, peak);
}

bool ChunkPrefab::isSolidBlock(int worldX, int worldY, int worldZ,
                               int terrainHeight) {
  int localX = worldX - xPos;
  int localY = worldY;
  int localZ = worldZ - zPos;

  if (isValidPos(localX, localY, localZ)) {
    if (blocks.empty())
      return false;
    Uint8 blockID = blocks[localX + localY * ChunkPrefab::xSize +
                           localZ * ChunkPrefab::xSize * ChunkPrefab::ySize];
    return blockID != 0 && blockID != 5 &&
           !BlockDef[blockID].collisionBoxes.has_value();
  }

  // If outside this chunk, use manager's global check
  return manager->IsSolid({(float)worldX, (float)worldY, (float)worldZ});
}

Uint8 ChunkPrefab::GetBlockID(int worldX, int worldY, int worldZ,
                              int terrainHeight) {
  int localX = worldX - xPos;
  int localY = worldY;
  int localZ = worldZ - zPos;

  if (isValidPos(localX, localY, localZ)) {
    if (blocks.empty())
      return 0;
    return blocks[localX + localY * ChunkPrefab::xSize +
                  localZ * ChunkPrefab::xSize * ChunkPrefab::ySize];
  }

  // For neighbor queries during generation or meshing, fall back to manager
  return manager->GetBlockID({(float)worldX, (float)worldY, (float)worldZ});
}

void ChunkPrefab::PrecomputeInterpolation(std::vector<float> &caveTr,
                                          std::vector<float> &coalCh,
                                          std::vector<float> &ironCh,
                                          std::vector<float> &diamCh) {
  for (int y = 0; y < (int)ySize; y++) {
    float fy = (float)y;
    caveTr[y] = GetCaveThreshold(fy);
    coalCh[y] = GetCoalChance(fy);
    ironCh[y] = GetIronChance(fy);
    diamCh[y] = GetDiamondChance(fy);
  }
}

void ChunkPrefab::GenerateHeightAndBiomes(std::vector<int> &heightCache,
                                          std::vector<Biome> &biomeCache) {
  for (int x = -1; x <= (int)xSize; x++) {
    for (int z = -1; z <= (int)zSize; z++) {
      int worldX = xPos + x;
      int worldZ = zPos + z;
      Vector2 worldPos = {(float)worldX, (float)worldZ};

      float cont = PerlinNoise2D(worldPos, 3, 0.005f);
      float eros = PerlinNoise2D(worldPos, 3, 0.015f);
      float peak = PerlinNoise2D(worldPos, 4, 0.02f);
      int terrainHeight = GetBaseHeight(cont, eros, peak);

      heightCache[(x + 1) + (z + 1) * (xSize + 2)] = terrainHeight;

      if (x >= 0 && x < (int)xSize && z >= 0 && z < (int)zSize) {
        float hum =
            PerlinNoise2D({worldPos.x * 0.02f, worldPos.y * 0.02f}, 2, 0.5f);
        float temp = PerlinNoise2D(
            {worldPos.x * 0.02f + 500.0f, worldPos.y * 0.02f + 500.0f}, 2,
            0.5f);

        float heightFactor =
            (float)(terrainHeight - SeaLevel) / (float)(ySize - SeaLevel);
        if (heightFactor > 0.0f) {
          temp = SDL_clamp(temp - heightFactor, -1.0f, 1.0f);
        }

        hum = (hum > 0) ? pow(hum, 0.7f) : -pow(-hum, 0.7f);
        temp = (temp > 0) ? pow(temp, 0.7f) : -pow(-temp, 0.7f);

        biomeCache[x + z * xSize] =
            manager->GetBiome((hum + 1.0f) * 50.0f, (temp + 1.0f) * 50.0f);
      }
    }
  }
}

void ChunkPrefab::PopulateBlocks(const std::vector<int> &heightCache,
                                 const std::vector<Biome> &biomeCache,
                                 std::vector<bool> &solidCache) {
  std::vector<float> caveTrCache(ySize), coalChCache(ySize), ironChCache(ySize),
      diamChCache(ySize);
  PrecomputeInterpolation(caveTrCache, coalChCache, ironChCache, diamChCache);

  std::unordered_map<int, Uint8> localMods;
  manager->GetModificationsForChunk(xPos, zPos, localMods);

  // Precompute cave noise for the whole chunk in one pass
  // Stored as caveMap[x + y*xSize + z*xSize*ySize]
  // Only computed for cave Y range to save time
  std::vector<bool> caveMap(xSize * ySize * zSize, false);
  for (int y = CaveMinY; y <= CaveMaxY; y++) {
    for (int z = 0; z < (int)zSize; z++) {
      for (int x = 0; x < (int)xSize; x++) {
        float fy = (float)y;
        float wx = (float)(xPos + x);
        float wz = (float)(zPos + z);
        float n1 = PerlinNoise({wx * 0.08f, fy * 0.08f, wz * 0.08f}, 1, 0.5f);
        float n2 =
            PerlinNoise({wx * 0.1f + 100.0f, fy * 0.1f, wz * 0.1f}, 1, 0.5f);
        if ((n1 * n1 + n2 * n2) < caveTrCache[y])
          caveMap[x + y * xSize + z * xSize * ySize] = true;
      }
    }
  }

  const int beachLevel = SeaLevel + 2;

  for (int x = 0; x < (int)xSize; x++) {
    int worldX = xPos + x;
    for (int z = 0; z < (int)zSize; z++) {
      int worldZ = zPos + z;
      int terrainHeight = heightCache[(x + 1) + (z + 1) * (xSize + 2)];
      const Biome &biome = biomeCache[x + z * xSize];

      Uint8 surfaceBlockID = (int)BlockIDDef::Grass;
      switch (biome.Type) {
      case BiomeType::Desert:
        surfaceBlockID = (int)BlockIDDef::Sand;
        break;
      case BiomeType::Ice:
        surfaceBlockID = (int)BlockIDDef::Ice;
        break;
      case BiomeType::Tundra:
      case BiomeType::BigTaiga:
      case BiomeType::Taiga:
        surfaceBlockID = (int)BlockIDDef::Snow;
        break;
      default:
        break;
      }

      for (int y = 0; y < (int)ySize; y++) {
        int idx = x + y * xSize + z * xSize * ySize;
        Uint8 blockID = 0;
        bool isSolid = false;

        auto mit = localMods.find(idx);
        if (mit != localMods.end()) {
          blockID = mit->second;
          isSolid = (blockID != 0 && blockID != 5);
        } else if (y == 0) {
          blockID = (int)BlockIDDef::Bedrock;
          isSolid = true;
        } else if (y <= terrainHeight) {
          bool isCave = (y >= CaveMinY && y <= CaveMaxY) && caveMap[idx];

          if (isCave) {
            blockID = 0;
          } else {
            isSolid = true;
            if (terrainHeight - y > 3) {
              blockID = (int)BlockIDDef::Stone;
              float Ore = std::abs(PerlinNoise(
                  {(float)worldX, (float)y, (float)worldZ}, 1, 0.05f));
              if (Ore > 0.25f && Ore < 0.25f + coalChCache[y])
                blockID = (int)BlockIDDef::CoalOre;
              else if (Ore > 0.4f && Ore < 0.4f + ironChCache[y])
                blockID = (int)BlockIDDef::IronOre;
              else if (Ore > 0.5f && Ore < 0.5f + diamChCache[y])
                blockID = (int)BlockIDDef::DiamondOre;
            } else if (terrainHeight < SeaLevel ||
                       (terrainHeight <= beachLevel &&
                        terrainHeight >= beachLevel - 3)) {
              blockID = (int)BlockIDDef::Sand;
            } else if (terrainHeight - y > 0) {
              if (surfaceBlockID == (int)BlockIDDef::Snow)
                blockID = (int)BlockIDDef::Stone;
              else if (surfaceBlockID == (int)BlockIDDef::Sand)
                blockID = (int)BlockIDDef::SandStone;
              else
                blockID = (int)BlockIDDef::Dirt;
            } else {
              blockID = surfaceBlockID;
            }
          }
        } else if (y == SeaLevel - 1 && biome.Type == BiomeType::Ice) {
          blockID = (int)BlockIDDef::Ice;
        } else if (y < SeaLevel) {
          blockID = (int)BlockIDDef::Water;
        }

        blocks[idx] = blockID;
        solidCache[idx] = isSolid;
      }
    }
  }
}

void ChunkPrefab::GenerateChunk() {
  // Guard: skip if already being processed (shouldn't happen, but be safe)
  bool expected = false;
  if (!isProcessing.compare_exchange_strong(expected, true))
    return;

  size_t totalBlocksSize = (size_t)xSize * ySize * zSize;
  if (this->blocks.size() != totalBlocksSize) {
    this->blocks.assign(totalBlocksSize, 0);
  }

  std::vector<int> heightCache((xSize + 2) * (zSize + 2));
  std::vector<Biome> biomeCache(xSize * zSize);
  std::vector<bool> solidCache(totalBlocksSize, false);

  GenerateHeightAndBiomes(heightCache, biomeCache);
  PopulateBlocks(heightCache, biomeCache, solidCache);

  GenerateVegetation(heightCache, biomeCache, solidCache);
  GenerateLighting();
  PropagateLighting();
  GenerateMesh();

  needsMeshUpdate = true;
  isGenerated = true;
  allFaces.shrink_to_fit();
  isDirty = true;
  isProcessing = false;
}

void ChunkPrefab::GenerateVegetation(const std::vector<int> &heightCache,
                                     const std::vector<Biome> &biomeMap,
                                     std::vector<bool> &solidCache) {
  const int heightCacheWidth = xSize + 2;
  for (int x = 2; x < xSize - 2; x++) {
    for (int z = 2; z < zSize - 2; z++) {
      int heightIdx = (x + 1) + (z + 1) * heightCacheWidth;
      int height = heightCache[heightIdx];

      if (height < CaveMinY || height > ySize - 10)
        continue;

      int worldX = xPos + x;
      int worldZ = zPos + z;

      Biome biome = biomeMap[x + z * xSize];

      float cont = PerlinNoise2D({(float)worldX, (float)worldZ}, 3, 0.005f);
      float eros = PerlinNoise2D({(float)worldX, (float)worldZ}, 3, 0.01f);

      float treeChanceThreshold = 0.9f; // Default (Plains/Sparse)

      if (biome.Type == BiomeType::Forest ||
          biome.Type == BiomeType::DarkForest ||
          biome.Type == BiomeType::Birch) {
        treeChanceThreshold = 0.4f; // High density Forests
      } else if (biome.Type == BiomeType::Jungle ||
                 biome.Type == BiomeType::Taiga ||
                 biome.Type == BiomeType::BigTaiga) {
        treeChanceThreshold = 0.6f; // Mid density
      } else if (biome.Type == BiomeType::Desert ||
                 biome.Type == BiomeType::Ice) {
        treeChanceThreshold = 2.0f; // No trees
      } else if (biome.Type == BiomeType::Savanna) {
        treeChanceThreshold = 0.7f; // Savannah density
      }

      // If it's relatively flat (high erosion), increase tree frequency
      if (eros > 0.0f && treeChanceThreshold < 1.0f) {
        treeChanceThreshold -= 0.2f; // More trees in flat areas
      }
      // If it's deep land (plains-like continentalness), further increase
      // chance
      if (cont < 0.0f && cont > -0.5f && treeChanceThreshold < 1.0f) {
        treeChanceThreshold -= 0.1f;
      }

      float treeNoise = PerlinNoise2D({(float)worldX, (float)worldZ}, 1, 0.4f);

      if (treeNoise > treeChanceThreshold) {
        int idx = x + height * xSize + z * xSize * ySize;
        if (this->blocks[idx] == 1 ||
            this->blocks[idx] == 2) { // On Grass or Dirt
          // Proximity check: don't place if there's wood nearby
          bool tooClose = false;
          for (int dx = -2; dx <= 2; dx++) {
            for (int dz = -2; dz <= 2; dz++) {
              if (dx == 0 && dz == 0)
                continue;
              int nx = x + dx;
              int nz = z + dz;
              if (nx >= 0 && nx < xSize && nz >= 0 && nz < zSize) {
                // Check a small vertical range for wood (BlockID 6)
                for (int dy = -1; dy <= 5; dy++) {
                  int checkY = height + dy;
                  if (checkY >= 0 && checkY < ySize) {
                    if (this->blocks[nx + checkY * xSize +
                                     nz * xSize * ySize] == 6) {
                      tooClose = true;
                      break;
                    }
                  }
                }
              }
              if (tooClose)
                break;
            }
            if (tooClose)
              break;
          }

          if (tooClose)
            continue;

          // Height varied by noise surplus
          int trunkHeight = 4 + (int)((treeNoise - treeChanceThreshold) * 6.0f);
          if (trunkHeight > 10)
            trunkHeight = 10;

          if (biome.Type == BiomeType::Taiga ||
              biome.Type == BiomeType::Tundra) {
            PlacePineTree(x, height, z, trunkHeight, solidCache);
          } else if (biome.Type == BiomeType::BigTaiga ||
                     biome.Type == BiomeType::DarkForest) {
            PlaceLargeTree(x, height, z, trunkHeight, solidCache);
          } else if (biome.Type == BiomeType::Jungle) {
            PlaceJungleTree(x, height, z, trunkHeight, solidCache);
          } else if (biome.Type == BiomeType::Savanna) {
            PlaceSavannaTree(x, height, z, trunkHeight, solidCache);
          } else if (biome.Type == BiomeType::Ice ||
                     biome.Type == BiomeType::Desert) {
            // No trees
          } else {
            PlaceStandardTree(x, height, z, trunkHeight, solidCache);
          }
        }
      }
    }
  }
}

void ChunkPrefab::PlaceStandardTree(int x, int y, int z, int trunkHeight,
                                    std::vector<bool> &solidCache) {
  for (int ty = 1; ty <= trunkHeight; ty++) {
    if (isValidPos(x, y + ty, z)) {
      int tidx = x + (y + ty) * xSize + z * xSize * ySize;
      if (manager->GetMod(
              {(float)(xPos + x), (float)(y + ty), (float)(zPos + z)}) == 255) {
        this->blocks[tidx] = 6; // Wood
        solidCache[tidx] = true;
      }
    }
  }
  for (int ly = trunkHeight - 1; ly <= trunkHeight + 1; ly++) {
    for (int lx = -2; lx <= 2; lx++) {
      for (int lz = -2; lz <= 2; lz++) {
        int nx = x + lx, ny = y + ly, nz = z + lz;
        if (isValidPos(nx, ny, nz)) {
          int lidx = nx + ny * xSize + nz * xSize * ySize;
          if (this->blocks[lidx] == 0 &&
              manager->GetMod(
                  {(float)(xPos + nx), (float)ny, (float)(zPos + nz)}) == 255) {
            if (lx * lx + lz * lz + (ly - trunkHeight) * (ly - trunkHeight) <=
                5) {
              solidCache[lidx] = true;
              this->blocks[lidx] = 7; // Leaves
            }
          }
        }
      }
    }
  }
}

void ChunkPrefab::PlacePineTree(int x, int y, int z, int trunkHeight,
                                std::vector<bool> &solidCache) {
  trunkHeight = (trunkHeight < 6) ? 6 : trunkHeight;
  for (int ty = 1; ty <= trunkHeight; ty++) {
    if (isValidPos(x, y + ty, z)) {
      int tidx = x + (y + ty) * xSize + z * xSize * ySize;
      this->blocks[tidx] = 6;
      solidCache[tidx] = true;
    }
  }
  for (int ly = 2; ly <= trunkHeight + 1; ly++) {
    int radius = (ly < trunkHeight - 2) ? 2 : (ly < trunkHeight) ? 1 : 0;
    if (ly % 2 == 0 || ly == trunkHeight + 1) { // Layered spruce style
      for (int lx = -radius; lx <= radius; lx++) {
        for (int lz = -radius; lz <= radius; lz++) {
          int nx = x + lx, ny = y + ly, nz = z + lz;
          if (isValidPos(nx, ny, nz)) {
            int lidx = nx + ny * xSize + nz * xSize * ySize;
            if (this->blocks[lidx] == 0) {
              this->blocks[lidx] = 7;
              solidCache[lidx] = true;
            }
          }
        }
      }
    }
  }
}

void ChunkPrefab::PlaceLargeTree(int x, int y, int z, int trunkHeight,
                                 std::vector<bool> &solidCache) {
  trunkHeight += 2;
  for (int ty = 1; ty <= trunkHeight; ty++) {
    if (isValidPos(x, y + ty, z)) {
      int tidx = x + (y + ty) * xSize + z * xSize * ySize;
      this->blocks[tidx] = 6;
      solidCache[tidx] = true;
    }
  }
  for (int ly = trunkHeight - 3; ly <= trunkHeight + 2; ly++) {
    int r = (ly > trunkHeight) ? 1 : 3;
    for (int lx = -r; lx <= r; lx++) {
      for (int lz = -r; lz <= r; lz++) {
        if (lx * lx + lz * lz > r * r + 1)
          continue;
        int nx = x + lx, ny = y + ly, nz = z + lz;
        if (isValidPos(nx, ny, nz)) {
          int lidx = nx + ny * xSize + nz * xSize * ySize;
          if (this->blocks[lidx] == 0) {
            this->blocks[lidx] = 7;
            solidCache[lidx] = true;
          }
        }
      }
    }
  }
}

void ChunkPrefab::PlaceSavannaTree(int x, int y, int z, int trunkHeight,
                                   std::vector<bool> &solidCache) {
  for (int ty = 1; ty <= trunkHeight; ty++) {
    if (isValidPos(x, y + ty, z)) {
      int tidx = x + (y + ty) * xSize + z * xSize * ySize;
      this->blocks[tidx] = 6;
      solidCache[tidx] = true;
    }
  }
  for (int lx = -3; lx <= 3; lx++) {
    for (int lz = -3; lz <= 3; lz++) {
      if (lx * lx + lz * lz > 10)
        continue;
      int nx = x + lx, ny = y + trunkHeight, nz = z + lz;
      if (isValidPos(nx, ny, nz)) {
        int lidx = nx + ny * xSize + nz * xSize * ySize;
        if (this->blocks[lidx] == 0) {
          this->blocks[lidx] = 7;
          solidCache[lidx] = true;
        }
      }
      if (lx * lx + lz * lz < 5 && isValidPos(nx, ny + 1, nz)) {
        int lidx = nx + (ny + 1) * xSize + nz * xSize * ySize;
        if (this->blocks[lidx] == 0) {
          this->blocks[lidx] = 7;
          solidCache[lidx] = true;
        }
      }
    }
  }
}

void ChunkPrefab::PlaceJungleTree(int x, int y, int z, int trunkHeight,
                                  std::vector<bool> &solidCache) {
  trunkHeight += 10;
  for (int ty = 1; ty <= trunkHeight; ty++) {
    if (isValidPos(x, y + ty, z)) {
      int tidx = x + (y + ty) * xSize + z * xSize * ySize;
      this->blocks[tidx] = 6;
      solidCache[tidx] = true;
    }
  }
  for (int ly = trunkHeight - 2; ly <= trunkHeight + 1; ly++) {
    int r = (ly > trunkHeight) ? 1 : 2;
    for (int lx = -r; lx <= r; lx++) {
      for (int lz = -r; lz <= r; lz++) {
        int nx = x + lx, ny = y + ly, nz = z + lz;
        if (isValidPos(nx, ny, nz)) {
          int lidx = nx + ny * xSize + nz * xSize * ySize;
          if (this->blocks[lidx] == 0) {
            this->blocks[lidx] = 7;
            solidCache[lidx] = true;
          }
        }
      }
    }
  }
}

void ChunkPrefab::GenerateMesh() {
  this->allFaces.clear();
  int estimatedFaces = xSize * zSize * 6;
  this->allFaces.reserve(estimatedFaces);

  // Precompute per-column max solid Y to skip empty air above terrain
  std::vector<int> colTopY(xSize * zSize, -1);
  for (int z = 0; z < (int)zSize; z++) {
    for (int x = 0; x < (int)xSize; x++) {
      for (int y = (int)ySize - 1; y >= 0; y--) {
        if (blocks[x + y * xSize + z * xSize * ySize] != 0) {
          colTopY[x + z * xSize] = y;
          break;
        }
      }
    }
  }

  // Global max Y across all columns for the Y loop bound
  int globalTopY = 0;
  for (int v : colTopY)
    if (v > globalTopY)
      globalTopY = v;

  for (int y = 0; y <= globalTopY; y++) {
    for (int z = 0; z < (int)ChunkPrefab::zSize; z++) {
      for (int x = 0; x < (int)ChunkPrefab::xSize; x++) {
        int idx = x + y * ChunkPrefab::xSize +
                  z * ChunkPrefab::xSize * ChunkPrefab::ySize;
        Uint8 bid = blocks[idx];
        if (bid == 0)
          continue;

        for (int side = 0; side < 6; side++) {
          int nx = x + (int)Direction[side].x;
          int ny = y + (int)Direction[side].y;
          int nz = z + (int)Direction[side].z;

          Uint8 nBid = 0;

          if (nx >= 0 && nx < ChunkPrefab::xSize && ny >= 0 &&
              ny < ChunkPrefab::ySize && nz >= 0 && nz < ChunkPrefab::zSize) {
            int nIdx = nx + ny * ChunkPrefab::xSize +
                       nz * ChunkPrefab::xSize * ChunkPrefab::ySize;
            nBid = this->blocks[nIdx];
          } else {
            nBid = manager->GetBlockID(
                {(float)(nx + xPos), (float)ny, (float)(nz + zPos)});
          }

          bool visible = false;
          if (BlockDef[bid].isTransparent()) {
            visible = (nBid == 0);
          } else {
            visible = (nBid == 0 || BlockDef[nBid].isTransparent());
          }

          if (visible) {
            Uint8 light = GetCombinedLight(nx, ny, nz);
            this->allFaces.push_back(DrawnFace{{(float)x, (float)y, (float)z},
                                               (Uint8)side,
                                               bid,
                                               light,
                                               BlockDef[bid].isWater()});
          }
        }
      }
    }
  }
}
Uint8 ChunkPrefab::GetCombinedLight(int x, int y, int z) {
  // If within bounds, get from this chunk directly
  if (x >= 0 && x < ChunkPrefab::xSize && y >= 0 && y < ChunkPrefab::ySize &&
      z >= 0 && z < ChunkPrefab::zSize) {
    if (!lightData.empty()) {
      int idx = x + y * ChunkPrefab::xSize +
                z * ChunkPrefab::xSize * ChunkPrefab::ySize;
      return std::max(lightData[idx].sunlight, lightData[idx].blockLight);
    }
    return 0;
  }

  // Out of bounds - query manager safely (no recursion)
  return manager->GetLightLevel(
      {(float)(x + xPos), (float)y, (float)(z + zPos)});
}
void ChunkPrefab::GenerateLighting() {
  lightData.assign(blocks.size(), {0, 0});

  for (int x = 0; x < ChunkPrefab::xSize; x++) {
    for (int z = 0; z < ChunkPrefab::zSize; z++) {
      Uint8 sun = manager->DayLightLevel;
      for (int y = ChunkPrefab::ySize - 1; y >= 0; y--) {
        int idx = x + y * ChunkPrefab::xSize +
                  z * ChunkPrefab::xSize * ChunkPrefab::ySize;
        Uint8 bid = blocks[idx];

        // Seed Sunlight
        if (bid == 0)
          lightData[idx].sunlight = sun;
        else if (bid == 5) {
          sun = sun > 1 ? sun - 0.5f : 0;
          lightData[idx].sunlight = sun;
        } else {
          sun = 0;
          lightData[idx].sunlight = 0;
        }

        // Seed Block Light (Luminescence)
        if (bid < BlockNum) {
          lightData[idx].blockLight = BlockDef[bid].Luminance;
        }
      }
    }
  }
}
void ChunkPrefab::PropagateLighting() {
  // Use flat int indices instead of Vector3 — 4 bytes vs 12 per queue item,
  // dramatically improves cache behaviour on BFS over 49k cells.
  const int C = xSize;
  const int CS = xSize * ySize;
  std::vector<int> sunQueue;
  std::vector<int> blockQueue;

  const int dirIdx[6] = {1,  -1,   // x+1, x-1
                         C,  -C,   // y+1, y-1 (step of xSize)
                         CS, -CS}; // z+1, z-1 (step of xSize*ySize)

  sunQueue.reserve(4096);
  blockQueue.reserve(512);

  // 1. Seed with existing light
  int total = xSize * ySize * zSize;
  for (int i = 0; i < total; i++) {
    if (lightData[i].sunlight > 0)
      sunQueue.push_back(i);
    if (lightData[i].blockLight > 0)
      blockQueue.push_back(i);
  }

  // 2. Pull light from neighbors on chunk borders
  for (int x = 0; x < ChunkPrefab::xSize; x++) {
    for (int y = 0; y < ChunkPrefab::ySize; y++) {
      for (int z = 0; z < ChunkPrefab::zSize; z++) {
        if (x > 0 && x < ChunkPrefab::xSize - 1 && z > 0 &&
            z < ChunkPrefab::zSize - 1)
          continue;

        for (int i = 0; i < 4; i++) {
          int nx = (int)x + (int)Direction[i].x;
          int nz = (int)z + (int)Direction[i].z;

          if (nx < 0 || nx >= ChunkPrefab::xSize || nz < 0 ||
              nz >= ChunkPrefab::zSize) {
            Vector3 worldPos = {(float)(nx + xPos), (float)y,
                                (float)(nz + zPos)};

            int idx = x + y * ChunkPrefab::xSize +
                      z * ChunkPrefab::xSize * ChunkPrefab::ySize;
            if (blocks[idx] != 0 && blocks[idx] != 5 && blocks[idx] != 9)
              continue;

            Uint8 nSun = manager->GetSunlightLevel(worldPos);
            if (nSun > 1) {
              Uint8 newSun = nSun - 1;
              if (BlockDef[blocks[idx]].isWater())
                newSun = (newSun > 2) ? newSun - 1 : 0;
              if (newSun > lightData[idx].sunlight) {
                lightData[idx].sunlight = newSun;
                sunQueue.push_back(idx);
              }
            }

            Uint8 nBlock = manager->GetBlockLightLevel(worldPos);
            if (nBlock > 1) {
              Uint8 newBlock = nBlock - 1;
              if (BlockDef[blocks[idx]].isWater())
                newBlock = (newBlock > 2) ? newBlock - 1 : 0;
              if (newBlock > lightData[idx].blockLight) {
                lightData[idx].blockLight = newBlock;
                blockQueue.push_back(idx);
              }
            }
          }
        }
      }
    }
  }

  // 3. BFS propagation using flat indices
  auto propagate = [&](std::vector<int> &q, bool isSunlight) {
    int head = 0;
    while (head < (int)q.size()) {
      int idx = q[head++];

      int x = idx % xSize;
      int y = (idx / xSize) % ySize;
      int z = idx / (xSize * ySize);

      Uint8 currentLight =
          isSunlight ? lightData[idx].sunlight : lightData[idx].blockLight;

      if (currentLight <= 1)
        continue;

      for (int i = 0; i < 6; i++) {
        int nx = x + (int)Direction[i].x;
        int ny = y + (int)Direction[i].y;
        int nz = z + (int)Direction[i].z;

        if (nx >= 0 && nx < ChunkPrefab::xSize && ny >= 0 &&
            ny < ChunkPrefab::ySize && nz >= 0 && nz < ChunkPrefab::zSize) {
          int nIdx = nx + ny * ChunkPrefab::xSize +
                     nz * ChunkPrefab::xSize * ChunkPrefab::ySize;
          Uint8 neighborBlock = blocks[nIdx];

          if (neighborBlock != 0 && !BlockDef[neighborBlock].isTransparent() &&
              BlockDef[neighborBlock].Luminance == 0)
            continue;

          Uint8 newLight = currentLight - 1;
          if (BlockDef[neighborBlock].isWater())
            newLight = (newLight > 2) ? newLight - 1 : 0;

          if (isSunlight) {
            if (newLight > lightData[nIdx].sunlight) {
              lightData[nIdx].sunlight = newLight;
              q.push_back(nIdx);
            }
          } else {
            if (newLight > lightData[nIdx].blockLight) {
              lightData[nIdx].blockLight = newLight;
              q.push_back(nIdx);
            }
          }
        }
      }
    }
  };

  propagate(sunQueue, true);
  propagate(blockQueue, false);
}