#include "../../include/common/Chunck.hpp"
#include "../../include/common/ChunkManager.hpp"
#include "../../include/common/PerlinNoise.hpp"
#include <SDL3/SDL_stdinc.h>
#include <algorithm>
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
  float cont = PerlinNoise2D(Pos, 3, 0.005);
  float eros = PerlinNoise2D(Pos, 3, 0.015f);
  float peak = PerlinNoise2D(Pos, 4, 0.02f);

  return GetBaseHeight(cont, eros, peak);
}

bool ChunkPrefab::isSolidBlock(int worldX, int worldY, int worldZ) {
  int localX = worldX - xPos;
  int localY = worldY;
  int localZ = worldZ - zPos;

  if (isValidPos(localX, localY, localZ)) {
    Uint8 blockID = GetBlock(localX, localY, localZ);
    return blockID != 0 && blockID != 5 &&
           !BlockDef[blockID].collisionBoxes.has_value();
  }

  // If outside this chunk, use manager's global check
  return manager->IsSolid({(float)worldX, (float)worldY, (float)worldZ});
}

Uint8 ChunkPrefab::GetBlockID(int worldX, int worldY, int worldZ) {
  int localX = worldX - xPos;
  int localY = worldY;
  int localZ = worldZ - zPos;

  if (isValidPos(localX, localY, localZ)) {
    return GetBlock(localX, localY, localZ);
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

      int terrainHeight = GetHeight(worldPos);

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
void ChunkPrefab::GenerateCaves(std::vector<bool> &caveMap,
                                std::vector<float> &caveTrCache) {
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
}
void ChunkPrefab::PopulateBlocks(const std::vector<int> &heightCache,
                                 const std::vector<Biome> &biomeCache,
                                 std::vector<bool> &solidCache) {
  std::vector<float> caveTrCache(ySize), coalChCache(ySize), ironChCache(ySize),
      diamChCache(ySize);
  PrecomputeInterpolation(caveTrCache, coalChCache, ironChCache, diamChCache);

  std::unordered_map<int, Uint8> localMods;
  manager->GetModificationsForChunk(xPos, zPos, localMods);

  std::vector<bool> caveMap(xSize * ySize * zSize, false);

  GenerateCaves(caveMap, caveTrCache);

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

        SetBlock(x, y, z, blockID);
        solidCache[idx] = isSolid;
      }
    }
  }
}

void ChunkPrefab::GenerateChunk() {
  bool expected = false;
  if (!isProcessing.compare_exchange_strong(expected, true))
    return;

  size_t totalBlocksSize = (size_t)xSize * ySize * zSize;

  std::vector<int> heightCache((xSize + 2) * (zSize + 2));
  std::vector<Biome> biomeCache(xSize * zSize);
  std::vector<bool> solidCache(totalBlocksSize, false);

  GenerateHeightAndBiomes(heightCache, biomeCache);
  PopulateBlocks(heightCache, biomeCache, solidCache);

  GenerateVegetation(heightCache, biomeCache, solidCache);
  std::vector<LightData> localLight;
  GenerateLighting(localLight);
  PropagateLighting(localLight);
  GenerateMesh(localLight);

  needsMeshUpdate = true;
  isGenerated = true;
  isDirty = true;
  isProcessing = false;
}

void ChunkPrefab::GenerateVegetation(const std::vector<int> &heightCache,
                                     const std::vector<Biome> &biomeMap,
                                     std::vector<bool> &solidCache) {
  const int heightCacheWidth = xSize + 2;
  for (int x = 2; x < xSize - 2; x += 2) {
    for (int z = 2; z < zSize - 2; z += 2) {
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
        if (GetBlock(x, height, z) == 1 ||
            GetBlock(x, height, z) == 2) { // On Grass or Dirt
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
                    if (GetBlock(nx, checkY, nz) == 6) {
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
      if (manager->GetMod(
              {(float)(xPos + x), (float)(y + ty), (float)(zPos + z)}) == 255) {
        SetBlock(x, y + ty, z, 6); // Wood
        int tidx = x + (y + ty) * xSize + z * xSize * ySize;
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
          if (GetBlock(nx, ny, nz) == 0 &&
              manager->GetMod(
                  {(float)(xPos + nx), (float)ny, (float)(zPos + nz)}) == 255) {
            if (lx * lx + lz * lz + (ly - trunkHeight) * (ly - trunkHeight) <=
                5) {
              solidCache[lidx] = true;
              SetBlock(nx, ny, nz, 7); // Leaves
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
      SetBlock(x, y + ty, z, 6);
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
            if (GetBlock(nx, ny, nz) == 0) {
              SetBlock(nx, ny, nz, 7);
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
      SetBlock(x, y + ty, z, 6);
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
          if (GetBlock(nx, ny, nz) == 0) {
            SetBlock(nx, ny, nz, 7);
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
      SetBlock(x, y + ty, z, 6);
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
        if (GetBlock(nx, ny, nz) == 0) {
          SetBlock(nx, ny, nz, 7);
          solidCache[lidx] = true;
        }
      }
      if (lx * lx + lz * lz < 5 && isValidPos(nx, ny + 1, nz)) {
        int lidx = nx + (ny + 1) * xSize + nz * xSize * ySize;
        if (GetBlock(nx, ny + 1, nz) == 0) {
          SetBlock(nx, ny + 1, nz, 7);
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
      SetBlock(x, y + ty, z, 6);
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
          if (GetBlock(nx, ny, nz) == 0) {
            SetBlock(nx, ny, nz, 7);
            solidCache[lidx] = true;
          }
        }
      }
    }
  }
}

void ChunkPrefab::GenerateMesh(const std::vector<LightData> &localLight) {
  std::lock_guard<std::recursive_mutex> lockManager(manager->chunks_mutex);
  std::lock_guard<std::recursive_mutex> lockFace(faceMutex);
  this->opaqueFaces.clear();
  this->transparentFaces.clear();
  int estimatedFaces = 5000;
  this->opaqueFaces.reserve(estimatedFaces);
  this->transparentFaces.reserve(500);

  for (int z = 0; z < ChunkPrefab::zSize; z++) {
    for (int x = 0; x < ChunkPrefab::xSize; x++) {
      for (int y = 0; y < ChunkPrefab::ySize; y++) {
        int idx = x + y * ChunkPrefab::xSize +
                  z * ChunkPrefab::xSize * ChunkPrefab::ySize;
        Uint8 blockID = GetBlock(x, y, z);
        if (blockID == (int)BlockIDDef::Air)
          continue;

        for (int side = 0; side < 6; side++) {
          if (y == 0 && side != 4)
            continue;
          int nx = x + (int)Direction[side].x;
          int ny = y + (int)Direction[side].y;
          int nz = z + (int)Direction[side].z;

          Uint8 nBid = 0;

          if (nx >= 0 && nx < ChunkPrefab::xSize && ny >= 0 &&
              ny < ChunkPrefab::ySize && nz >= 0 && nz < ChunkPrefab::zSize) {
            nBid = GetBlock(nx, ny, nz);
          } else {
            nBid = manager->GetBlockID(
                {(float)(nx + xPos), (float)ny, (float)(nz + zPos)});
          }

          bool visible = false;
          if (BlockDef[blockID].isTransparent())
            visible = (nBid == (int)BlockIDDef::Air);
          else
            visible = BlockDef[nBid].isTransparent();

          if (visible) {
            Uint8 light = GetCombinedLight(nx, ny, nz, localLight);
            // Light is 0-15, store full 4 bits
            Uint8 packedLight = light & 0xF;
            Uint32 packed = DrawnFace::Pack((Uint16)idx, (Uint8)side,
                                            packedLight, (Uint16)blockID);

            if (BlockDef[blockID].isTransparent()) {
              this->transparentFaces.push_back(packed);
            } else {
              this->opaqueFaces.push_back(packed);
            }
          }
        }
      }
    }
  }

  // Sort faces by posIndex (lowest 16 bits) to enable binary search
  auto compareFaces = [](Uint32 a, Uint32 b) {
    return (a & 0xFFFF) < (b & 0xFFFF);
  };
  std::sort(opaqueFaces.begin(), opaqueFaces.end(), compareFaces);
  std::sort(transparentFaces.begin(), transparentFaces.end(), compareFaces);

  opaqueFaces.shrink_to_fit();
  transparentFaces.shrink_to_fit();
}
Uint8 ChunkPrefab::GetCombinedLight(int x, int y, int z,
                                    const std::vector<LightData> &localLight) {
  // If within bounds, get from this chunk directly
  if (x >= 0 && x < ChunkPrefab::xSize && y >= 0 && y < ChunkPrefab::ySize &&
      z >= 0 && z < ChunkPrefab::zSize) {
    if (!localLight.empty()) {
      int idx = x + y * ChunkPrefab::xSize +
                z * ChunkPrefab::xSize * ChunkPrefab::ySize;
      return std::max(localLight[idx].sunlight, localLight[idx].blockLight);
    }
    // If lightData is cleared, use binary search on faces
    return GetLightFromFaces(x, y, z);
  }

  // Out of bounds - query manager safely (no recursion)
  return manager->GetLightLevel(
      {(float)(x + xPos), (float)y, (float)(z + zPos)});
}

int ChunkPrefab::BinarySearchFace(Uint16 posIndex,
                                  const std::vector<Uint32> &faces) const {
  if (faces.empty())
    return -1;

  int low = 0;
  int high = (int)faces.size() - 1;

  while (low <= high) {
    int mid = low + (high - low) / 2;
    Uint16 midPos = (Uint16)(faces[mid] & 0xFFFF);

    if (midPos == posIndex)
      return mid;
    if (midPos < posIndex)
      low = mid + 1;
    else
      high = mid - 1;
  }
  return -1;
}

Uint8 ChunkPrefab::GetLightFromFaces(int x, int y, int z) const {
  // std::lock_guard<std::recursive_mutex> lockManager(manager->chunks_mutex);
  // std::lock_guard<std::recursive_mutex> lockFace(faceMutex);
  Uint16 posIndex = (Uint16)(x + y * xSize + z * xSize * ySize);

  // Check opaque faces first
  int idx = BinarySearchFace(posIndex, opaqueFaces);
  if (idx != -1) {
    return (Uint8)((opaqueFaces[idx] >> 19) & 0xF);
  }

  // Check transparent faces
  idx = BinarySearchFace(posIndex, transparentFaces);
  if (idx != -1) {
    return (Uint8)((transparentFaces[idx] >> 19) & 0xF);
  }

  return 0;
}
// FIX Using execisve mem for light
void ChunkPrefab::GenerateLighting(std::vector<LightData> &localLight) {
  localLight.assign(xSize * ySize * zSize, {0, 0});

  for (int x = 0; x < ChunkPrefab::xSize; x++) {
    for (int z = 0; z < ChunkPrefab::zSize; z++) {
      Uint8 sun = manager->DayLightLevel;
      for (int y = ChunkPrefab::ySize - 1; y >= 0; y--) {
        int idx = x + y * ChunkPrefab::xSize +
                  z * ChunkPrefab::xSize * ChunkPrefab::ySize;
        Uint8 blockID = GetBlock(x, y, z);

        if (blockID == (int)BlockIDDef::Air)
          localLight[idx].sunlight = sun;
        else if (blockID == (int)BlockIDDef::Water) {
          sun = sun > 1 ? sun - 0.5f : 0;
          localLight[idx].sunlight = sun;
        } else {
          sun = 0;
        }

        localLight[idx].blockLight = BlockDef[blockID].Luminance;
      }
    }
  }
}
void ChunkPrefab::PropagateLighting(std::vector<LightData> &localLight) {
  const int SX = xSize;
  const int SY = ySize;
  const int SZ = zSize;
  const int SXY = SX * SY;

  std::vector<int> sunQueue;
  std::vector<int> blockQueue;
  sunQueue.reserve(4096);
  blockQueue.reserve(512);

  // 1. Seed queues — unchanged
  for (int i = 0, TOTAL = SX * SY * SZ; i < TOTAL; i++) {
    if (localLight[i].sunlight > 0)
      sunQueue.push_back(i);
    if (localLight[i].blockLight > 0)
      blockQueue.push_back(i);
  }

  // 2. Border ingress — same logic as original, just loop only border faces
  for (int i = 0; i < 4; i++) {
    int dx = (int)Direction[i].x;
    int dz = (int)Direction[i].z;

    for (int y = 0; y < SY; y++) {
      for (int a = 0; a < (dx != 0 ? SZ : SX); a++) {
        // x is fixed at border if dx!=0, else varies; same for z
        int x = (dx == -1) ? 0 : (dx == 1) ? SX - 1 : a;
        int z = (dz == -1) ? 0 : (dz == 1) ? SZ - 1 : a;

        int idx = x + y * SX + z * SXY;
        Uint8 borderBlock = GetBlock(x, y, z);
        if (borderBlock != (int)BlockIDDef::Air &&
            borderBlock != (int)BlockIDDef::Water)
          continue;

        Vector3 worldPos = {(float)(x + xPos) + Direction[i].x, (float)y,
                            (float)(z + zPos) + Direction[i].z};
        Uint8 nSun = manager->GetLightLevel(worldPos);
        if (nSun > 1) {
          Uint8 newSun = nSun - 1;
          if (BlockDef[borderBlock].isWater())
            newSun = (newSun > 2) ? newSun - 1 : 0;
          if (newSun > localLight[idx].sunlight) {
            localLight[idx].sunlight = newSun;
            sunQueue.push_back(idx);
          }
        }
      }
    }
  }

  // 3. BFS — flat offsets only, this is where the real speedup is
  auto propagate = [&](std::vector<int> &q, bool isSun) {
    int head = 0;
    while (head < (int)q.size()) {
      const int idx = q[head++];
      const Uint8 cur =
          isSun ? localLight[idx].sunlight : localLight[idx].blockLight;
      if (cur <= 1)
        continue;

      const int x = idx % SX;
      const int y = (idx / SX) % SY;
      const int z = idx / SXY;

      const int nbr[6] = {idx - 1,  idx + 1,   idx - SX,
                          idx + SX, idx - SXY, idx + SXY};
      const bool valid[6] = {x > 0,      x < SX - 1, y > 0,
                             y < SY - 1, z > 0,      z < SZ - 1};

      for (int d = 0; d < 6; d++) {
        if (!valid[d])
          continue;
        const int nIdx = nbr[d];
        const int nx = nIdx % SX;
        const int ny = (nIdx / SX) % SY;
        const int nz = nIdx / SXY;

        Uint8 neighborBlock = GetBlock(nx, ny, nz);
        if (neighborBlock != 0 && !BlockDef[neighborBlock].isTransparent() &&
            BlockDef[neighborBlock].Luminance == 0)
          continue;

        Uint8 newLight = cur - 1;
        if (BlockDef[neighborBlock].isWater())
          newLight = (newLight > 2) ? newLight - 1 : 0;

        // Match original: both passes update sunlight
        // (preserve your original behavior exactly)
        if (newLight > localLight[nIdx].sunlight) {
          localLight[nIdx].sunlight = newLight;
          q.push_back(nIdx);
        }
      }
    }
  };

  propagate(sunQueue, true);
  propagate(blockQueue, false);
}