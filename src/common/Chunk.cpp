#include "../../include/common/Chunck.hpp"
#include "../../include/common/PerlinNoise.hpp"
#include <SDL3/SDL_stdinc.h>
#include <cstdlib>
#include <cstring>
#include <queue>

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
    return blockID != 0 && BlockDef[blockID].isSolid;
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

void ChunkPrefab::GenerateChunk(ChunkManager &manager) {
  this->manager = &manager;
  this->allFaces.clear();

  if (this->blocks.size() !=
      (size_t)ChunkPrefab::xSize * ChunkPrefab::ySize * ChunkPrefab::zSize) {
    this->blocks.assign(
        ChunkPrefab::xSize * ChunkPrefab::ySize * ChunkPrefab::zSize, 0);
  }

  std::vector<int> heightCache((xSize + 2) * (zSize + 2));
  std::vector<Biome> biomeCache(xSize * zSize);
  std::vector<bool> solidCache(xSize * ySize * zSize, false);

  std::vector<float> caveTrCache(ySize);
  std::vector<float> coalChCache(ySize);
  std::vector<float> ironChCache(ySize);
  std::vector<float> diamChCache(ySize);
  for (int y = 0; y < (int)ySize; y++) {
    caveTrCache[y] = GetCaveThreshold((float)y);
    coalChCache[y] = GetCoalChance((float)y);
    ironChCache[y] = GetIronChance((float)y);
    diamChCache[y] = GetDiamondChance((float)y);
  }

  // Unified generation loop: iterates over all columns including padding for
  // heightmap
  for (int x = -1; x <= (int)xSize; x++) {
    for (int z = -1; z <= (int)zSize; z++) {
      int worldX = xPos + x;
      int worldZ = zPos + z;

      // 1. Calculate Terrain Shape (Height)
      float cont = PerlinNoise2D({(float)worldX, (float)worldZ}, 3, 0.005f);
      float eros = PerlinNoise2D({(float)worldX, (float)worldZ}, 3, 0.015f);
      float peak = PerlinNoise2D({(float)worldX, (float)worldZ}, 4, 0.02f);
      int terrainHeight = GetBaseHeight(cont, eros, peak);

      heightCache[(x + 1) + (z + 1) * (xSize + 2)] = terrainHeight;

      // Only perform full block population for actual chunk columns
      if (isValidPos(x, 0, z)) {
        // 2. Determine Biome
        float hum = PerlinNoise2D(
            {(float)worldX * 0.02f, (float)worldZ * 0.02f}, 2, 0.5f);
        float temp = PerlinNoise2D(
            {(float)worldX * 0.02f + 500.0f, (float)worldZ * 0.02f + 500.0f}, 2,
            0.5f);

        // Height-based temperature (lapse rate)
        float seaLevel = (float)ChunkPrefab::SeaLevel;
        float heightFactor = (float)(terrainHeight - seaLevel) /
                             (float)(ChunkPrefab::ySize - seaLevel);
        if (heightFactor > 0.0f) {
          temp -= heightFactor * 1.0f;
          temp = SDL_clamp(temp, -1.0f, 1.0f);
        }

        // Apply contrast for more variety
        hum = (hum > 0) ? pow(hum, 0.7f) : -pow(-hum, 0.7f);
        temp = (temp > 0) ? pow(temp, 0.7f) : -pow(-temp, 0.7f);

        Biome biome =
            manager.GetBiome((hum + 1.0f) * 50.0f, (temp + 1.0f) * 50.0f);
        biomeCache[x + z * xSize] = biome;

        Uint8 surfaceBlockID = 1; // Grass
        if (biome.Type == BiomeType::Desert)
          surfaceBlockID = 8; // Sand
        else if (biome.Type == BiomeType::Savanna)
          surfaceBlockID = 2; // Dirt
        else if (biome.Type == BiomeType::Ice)
          surfaceBlockID = 21;
        else if (biome.Type == BiomeType::Tundra ||
                 biome.Type == BiomeType::BigTaiga ||
                 biome.Type == BiomeType::Taiga)
          surfaceBlockID = 22;

        // 3. Populate Vertical Blocks
        for (int y = 0; y < (int)ChunkPrefab::ySize; y++) {
          int idx = x + y * ChunkPrefab::xSize +
                    z * ChunkPrefab::xSize * ChunkPrefab::ySize;
          Uint8 blockID = 0;
          bool isSolid = false;

          // Priority 1: User modifications
          Uint8 mod = manager.GetMod({(float)worldX, (float)y, (float)worldZ});
          if (mod != 255) {
            blockID = mod;
            isSolid = (mod != 0 && mod != 5);
          } else {
            if (y == 0) {
              blockID = 4; // Bedrock
              isSolid = true;
            } else if (y <= terrainHeight) {
              const int seaLevelInt = ChunkPrefab::SeaLevel;
              bool isCave = false;

              // Priority 2: Caves
              if (y >= CaveMinY && y <= CaveMaxY) {
                float n1 = PerlinNoise({(float)worldX * 0.08f, (float)y * 0.08f,
                                        (float)worldZ * 0.08f},
                                       1, 0.5f);
                float n2 = PerlinNoise({(float)worldX * 0.1f + 100.0f,
                                        (float)y * 0.1f, (float)worldZ * 0.1f},
                                       1, 0.5f);
                if ((n1 * n1 + n2 * n2) < caveTrCache[y])
                  isCave = true;
              }

              if (isCave) {
                blockID = 0; // Air
                isSolid = false;
              } else {
                isSolid = true;
                const int beachLevel = ChunkPrefab::SeaLevel + 2;
                bool isBeach = (terrainHeight <= beachLevel &&
                                terrainHeight >= beachLevel - 3 && cont < 0.1f);
                bool isUnderwaterFloor = (terrainHeight < seaLevelInt);

                if (terrainHeight - y > 3) {
                  blockID = 3; // Stone
                  float Ore = std::abs(PerlinNoise(
                      {(float)worldX, (float)y, (float)worldZ}, 1, 0.05f));
                  float coalChance = coalChCache[y];
                  float ironChance = ironChCache[y];
                  float diamondChance = diamChCache[y];
                  if (Ore > 0.25f && Ore < 0.25f + coalChance)
                    blockID = 11;
                  else if (Ore > 0.4f && Ore < 0.4f + ironChance)
                    blockID = 13;
                  else if (Ore > 0.5f && Ore < 0.5f + diamondChance)
                    blockID = 10;
                } else if (isUnderwaterFloor || isBeach)
                  blockID = 8; // Sand
                else if (terrainHeight - y > 0)
                  if (surfaceBlockID == 22 || surfaceBlockID == 8)
                    blockID = 3;
                  else
                    blockID = 2; // Dirt
                else
                  blockID = surfaceBlockID;
              }
            } else if (y == ChunkPrefab::SeaLevel - 1 &&
                       biome.Type == BiomeType::Ice) {
              blockID = 21;
            } else if (y < ChunkPrefab::SeaLevel) { // Below sea level but above
                                                    // terrain
              blockID = 5;                          // Water
              isSolid = false;
            }
          }
          this->blocks[idx] = blockID;
          solidCache[idx] = isSolid;
        }
      }
    }
  }

  GenerateVegetation(heightCache, biomeCache, solidCache);
  isDirty = false;
  GenerateLighting();
  PropagateLighting();
  GenerateMesh();

  needsMeshUpdate = true;
  allFaces.shrink_to_fit();
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

  std::vector<Uint8> mask(xSize * ySize, 0); // Max possible size
  std::vector<Uint8> lightMask(xSize * ySize, 0);
  std::vector<bool> visited(xSize * ySize, false);

  for (int side = 0; side < 6; side++) {
    int d = (side < 2) ? 2 : (side < 4 ? 0 : 1); // normal axis
    int u = (d == 0) ? 2 : 0;                    // u axis
    int v = (d == 1) ? 2 : 1;                    // v axis
    if (d == 2) {
      u = 0;
      v = 1;
    }

    int dims[3] = {ChunkPrefab::xSize, ChunkPrefab::ySize, ChunkPrefab::zSize};

    for (int slice = 0; slice < dims[d]; slice++) {
      std::fill(mask.begin(), mask.begin() + (dims[u] * dims[v]), 0);
      std::fill(lightMask.begin(), lightMask.begin() + (dims[u] * dims[v]), 0);

      for (int i = 0; i < dims[u]; i++) {
        for (int j = 0; j < dims[v]; j++) {
          int x[3];
          x[d] = slice;
          x[u] = i;
          x[v] = j;

          int idx = x[0] + x[1] * ChunkPrefab::xSize +
                    x[2] * ChunkPrefab::xSize * ChunkPrefab::ySize;
          Uint8 bid = this->blocks[idx];

          if (bid != 0) {
            bool visible = false;
            int nx[3] = {x[0], x[1], x[2]};

            if (side == 0)
              nx[2]++;
            else if (side == 1)
              nx[2]--;
            else if (side == 2)
              nx[0]++;
            else if (side == 3)
              nx[0]--;
            else if (side == 4)
              nx[1]++;
            else if (side == 5)
              nx[1]--;

            int worldX = nx[0] + xPos;
            int worldY = nx[1];
            int worldZ = nx[2] + zPos;

            Uint8 nBid = 0;

            if (nx[0] >= 0 && nx[0] < ChunkPrefab::xSize && nx[1] >= 0 &&
                nx[1] < ChunkPrefab::ySize && nx[2] >= 0 &&
                nx[2] < ChunkPrefab::zSize) {
              int nIdx = nx[0] + nx[1] * ChunkPrefab::xSize +
                         nx[2] * ChunkPrefab::xSize * ChunkPrefab::ySize;
              nBid = this->blocks[nIdx];
            } else {
              nBid = manager->GetBlockID(
                  {(float)worldX, (float)worldY, (float)worldZ});
            }

            if (BlockDef[bid].isTransparent) {
              // Transparent blocks like water only show faces against air
              visible = (nBid == 0);
            } else {
              // Opaque blocks show faces against air or transparent blocks
              visible = (nBid == 0 || BlockDef[nBid].isTransparent);
            }

            if (visible) {
              mask[i + j * dims[u]] = bid;

              // FIXED: Get light level from neighbor, works for chunk
              // boundaries now
              Uint8 light = GetCombinedLight(nx[0], nx[1], nx[2]);

              lightMask[i + j * dims[u]] = light;
            }
          }
        }
      }

      // Greedy Meshing
      std::fill(visited.begin(), visited.begin() + (dims[u] * dims[v]), false);
      for (int j = 0; j < dims[v]; j++) {
        for (int i = 0; i < dims[u]; i++) {
          int mIdx = i + j * dims[u];
          if (mask[mIdx] != 0 && !visited[mIdx]) {
            Uint8 bid = mask[mIdx];
            Uint8 light = lightMask[mIdx];
            int w = 1, h = 1;

            // Expand width
            for (int i2 = i + 1; i2 < dims[u]; i2++) {
              int idx2 = i2 + j * dims[u];
              if (mask[idx2] == bid && !visited[idx2] &&
                  lightMask[idx2] == light) {
                w++;
              } else {
                break;
              }
            }

            // Expand height
            for (int j2 = j + 1; j2 < dims[v]; j2++) {
              bool rowMatch = true;
              for (int i2 = i; i2 < i + w; i2++) {
                int idx2 = i2 + j2 * dims[u];
                if (mask[idx2] != bid || visited[idx2] ||
                    lightMask[idx2] != light) {
                  rowMatch = false;
                  break;
                }
              }
              if (rowMatch) {
                h++;
              } else {
                break;
              }
            }

            int xOrigin[3];
            xOrigin[d] = slice;
            xOrigin[u] = i;
            xOrigin[v] = j;

            Vector3 Pos = Vector3((float)xOrigin[0], (float)xOrigin[1],
                                  (float)xOrigin[2]);

            this->allFaces.push_back(DrawnFace{Pos, (Uint8)side, bid, (Uint8)w,
                                               (Uint8)h, light,
                                               BlockDef[bid].isWater});

            for (int j2 = j; j2 < j + h; j2++) {
              for (int i2 = i; i2 < i + w; i2++) {
                visited[i2 + j2 * dims[u]] = true;
              }
            }
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
  std::queue<Vector3> sunQueue;
  std::queue<Vector3> blockQueue;

  // 1. Seed with existing light
  for (int x = 0; x < xSize; x++) {
    for (int y = 0; y < ySize; y++) {
      for (int z = 0; z < zSize; z++) {
        int idx = x + y * xSize + z * xSize * ySize;
        if (lightData[idx].sunlight > 0)
          sunQueue.push({(float)x, (float)y, (float)z});
        if (lightData[idx].blockLight > 0)
          blockQueue.push({(float)x, (float)y, (float)z});
      }
    }
  }

  // 2. Pull light from neighbors
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

            // Pull Sunlight
            Uint8 nSun = manager->GetSunlightLevel(worldPos);
            if (nSun > 1) {
              Uint8 newSun = nSun - 1;
              if (BlockDef[blocks[idx]].isWater)
                newSun = (newSun > 2) ? newSun - 1 : 0;

              if (newSun > lightData[idx].sunlight) {
                lightData[idx].sunlight = newSun;
                sunQueue.push({(float)x, (float)y, (float)z});
              }
            }

            // Pull Block Light
            Uint8 nBlock = manager->GetBlockLightLevel(worldPos);
            if (nBlock > 1) {
              Uint8 newBlock = nBlock - 1;
              if (BlockDef[blocks[idx]].isWater)
                newBlock = (newBlock > 2) ? newBlock - 1 : 0;

              if (newBlock > lightData[idx].blockLight) {
                lightData[idx].blockLight = newBlock;
                blockQueue.push({(float)x, (float)y, (float)z});
              }
            }
          }
        }
      }
    }
  }

  // Generalized BFS propagation function
  auto propagate = [&](std::queue<Vector3> &q, bool isSunlight) {
    while (!q.empty()) {
      Vector3 pos = q.front();
      q.pop();

      int idx = (int)pos.x + (int)pos.y * xSize + (int)pos.z * xSize * ySize;
      Uint8 currentLight =
          isSunlight ? lightData[idx].sunlight : lightData[idx].blockLight;

      if (currentLight <= 1)
        continue;

      for (int i = 0; i < 6; i++) {
        int nx = (int)pos.x + (int)Direction[i].x;
        int ny = (int)pos.y + (int)Direction[i].y;
        int nz = (int)pos.z + (int)Direction[i].z;

        if (nx >= 0 && nx < ChunkPrefab::xSize && ny >= 0 &&
            ny < ChunkPrefab::ySize && nz >= 0 && nz < ChunkPrefab::zSize) {
          int nIdx = nx + ny * ChunkPrefab::xSize +
                     nz * ChunkPrefab::xSize * ChunkPrefab::ySize;
          Uint8 neighborBlock = blocks[nIdx];

          // Light only passes through non-solid/transparent blocks or emitters
          if (neighborBlock != 0 && !BlockDef[neighborBlock].isTransparent &&
              BlockDef[neighborBlock].Luminance == 0)
            continue;

          Uint8 newLight = currentLight - 1;
          if (BlockDef[neighborBlock].isWater)
            newLight = (newLight > 2) ? newLight - 1 : 0;

          if (isSunlight) {
            if (newLight > lightData[nIdx].sunlight) {
              lightData[nIdx].sunlight = newLight;
              q.push({(float)nx, (float)ny, (float)nz});
            }
          } else {
            if (newLight > lightData[nIdx].blockLight) {
              lightData[nIdx].blockLight = newLight;
              q.push({(float)nx, (float)ny, (float)nz});
            }
          }
        }
      }
    }
  };

  propagate(sunQueue, true);
  propagate(blockQueue, false);
}