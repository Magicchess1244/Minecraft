#include "../../include/common/Chunck.hpp"
#include "../../include/common/PerlinNoise.hpp"
#include <SDL3/SDL_stdinc.h>

constexpr float CaveThreshold = -0.18f;
constexpr int CaveMinY = 5;
constexpr int CaveMaxY = 50;

const Vector3 Direction[6] = {
    {0, 0, 1},  // Front
    {0, 0, -1}, // Back
    {1, 0, 0},  // Right
    {-1, 0, 0}, // Left
    {0, 1, 0},  // Top
    {0, -1, 0}  // Bottom
};

constexpr HeightsDif ContinentelnessHeight[7] = {
    {0.7f, 120},  // Extreme Mountains
    {0.4f, 100},  // Huge Mountains
    {0.15f, 50},   // Hills
    {-0.15f, 38}, // Plains (near sea level)
    {-0.25f, 25}, // Shallow Water / Beach
    {-0.35f, 38}, // Plains (near sea level)
    {-1.0f, 15}}; // Deep Ocean

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
  // Continentalness defines the basic land/ocean height
  float base = SampleSpline(Continentalness, ContinentelnessHeight, 5);

  // Higher multipliers for mountains and valleys
  float erosionFactor = (1.0f - Erosion) * 15.0f;
  float peakFactor = Peaks * 12.0f;

  return (int)(base + (erosionFactor * Peaks) + peakFactor);
}

int ChunkPrefab::GetHeight(Vector2 Pos) {
  float cont = PerlinNoise2D(Pos, 3, 0.005f); // Low frequency
  float eros = PerlinNoise2D(Pos, 3, 0.01f);  // Mid frequency
  float peak = PerlinNoise2D(Pos, 4, 0.02f);  // High frequency

  return GetBaseHeight(cont, eros, peak);
}

bool ChunkPrefab::isSolidBlock(int worldX, int worldY, int worldZ,
                               int terrainHeight, ChunkManager &manager) {
  int localX = worldX - xPos;
  int localY = worldY;
  int localZ = worldZ - zPos;

  if (isValidPos(localX, localY, localZ) && !blocks.empty()) {
    Uint8 blockID = blocks[localX + localY * xSize + localZ * xSize * ySize];
    return blockID != 0 && blockID != 5;
  }

  Uint8 blockID = GetBlockID(worldX, worldY, worldZ, terrainHeight, manager);
  return blockID != 0 &&
         blockID != 5; // Air and Water are not solid (for collision)
}

Uint8 ChunkPrefab::GetBlockID(int worldX, int worldY, int worldZ,
                              int terrainHeight, ChunkManager &manager) {
  int localX = worldX - xPos;
  int localY = worldY;
  int localZ = worldZ - zPos;

  if (isValidPos(localX, localY, localZ) && !blocks.empty()) {
    return blocks[localX + localY * xSize + localZ * xSize * ySize];
  }

  // Priority 1: User modifications (placing/breaking)
  Uint8 Modifications =
      manager.GetMod({(float)worldX, (float)worldY, (float)worldZ});
  if (Modifications != 255) {
    return Modifications;
  }

  // Priority 2: World boundaries
  if (worldY < 0 || worldY >= ySize)
    return 0; // Air
  if (worldY == 0)
    return 4; // Bedrock floor

  // Priority 3: Natural terrain
  if (worldY > terrainHeight)
    return 0; // Air

  // Priority 4: Improved Caves (Ridge Noise for Tunnels)
  if (worldY >= CaveMinY && worldY <= CaveMaxY) {
    // We use two noise maps to create winding tunnels
    // Tunnels exist where both noise values are close to 0
    float n1 = PerlinNoise(
        {(float)worldX * 0.08f, (float)worldY * 0.08f, (float)worldZ * 0.08f},
        2, 0.5f);
    float n2 = PerlinNoise({(float)worldX * 0.1f + 100.0f, (float)worldY * 0.1f,
                            (float)worldZ * 0.1f},
                           2, 0.5f);

    // Ridge noise pattern: 1.0 - abs(noise) creates sharp ridges
    // Here we find the intersection of two ridges to create "tubes"
    float ridge1 = n1 * n1;
    float ridge2 = n2 * n2;
    float density = ridge1 + ridge2;

    if (density < 0.05f) // Narrow threshold for snake-like tunnels
      return 0;          // Air
  }

  // Determine block type by height
  if (terrainHeight - worldY > 3)
    return 3; // Stone
  else if (terrainHeight - worldY > 0)
    return 2; // Dirt

  return 1; // Grass
}

void ChunkPrefab::GenerateChunk(ChunkManager &manager) {
  this->allFaces.clear();

  // 1. Generate Height Map
  std::vector<int> heightCache;
  GenerateHeightMap(heightCache);

  // 2. Generate Cave Density Map
  std::vector<float> caveDensityCache;
  GenerateCaveMap(caveDensityCache);

  // 3. Generate Modification Map
  std::vector<Uint8> modCache;
  GenerateModMap(modCache, manager);

  // 4. Populate Blocks (Main Terrain Logic)
  std::vector<bool> solidCache(xSize * ySize * zSize, false);
  PopulateBlocks(heightCache, caveDensityCache, modCache, solidCache, manager);

  // 5. Water Spread Logic
  SimulateWaterSpread(solidCache);

  // 6. Vegetation (Trees)
  GenerateVegetation(heightCache, modCache, solidCache);

  // 7. Generate Mesh
  GenerateMesh(solidCache, heightCache, manager);

  isDirty = false;
  needsMeshUpdate = true;
  allFaces.shrink_to_fit();
}

void ChunkPrefab::GenerateHeightMap(std::vector<int> &heightCache) {
  const int heightCacheWidth = xSize + 2;
  const int heightCacheDepth = zSize + 2;
  heightCache.resize(heightCacheWidth * heightCacheDepth);

  for (int x = -1; x <= xSize; x++) {
    for (int z = -1; z <= zSize; z++) {
      int cacheIdx = (x + 1) + (z + 1) * heightCacheWidth;
      heightCache[cacheIdx] = GetHeight({(float)(xPos + x), (float)(zPos + z)});
    }
  }
}

void ChunkPrefab::GenerateCaveMap(std::vector<float> &caveDensityCache) {
  caveDensityCache.resize(xSize * (CaveMaxY - CaveMinY + 1) * zSize);
  for (int y = CaveMinY; y <= CaveMaxY; y++) {
    for (int x = 0; x < xSize; x++) {
      for (int z = 0; z < zSize; z++) {
        int worldX = xPos + x;
        int worldZ = zPos + z;
        int idx =
            x + (y - CaveMinY) * xSize + z * xSize * (CaveMaxY - CaveMinY + 1);

        float n1 = PerlinNoise(
            {(float)worldX * 0.08f, (float)y * 0.08f, (float)worldZ * 0.08f}, 2,
            0.5f);
        float n2 = PerlinNoise({(float)worldX * 0.1f + 100.0f, (float)y * 0.1f,
                                (float)worldZ * 0.1f},
                               2, 0.5f);

        caveDensityCache[idx] = (n1 * n1) + (n2 * n2);
      }
    }
  }
}

void ChunkPrefab::GenerateModMap(std::vector<Uint8> &modCache,
                                 ChunkManager &manager) {
  modCache.assign(xSize * ySize * zSize, 255);
  for (int y = 0; y < ySize; y++) {
    for (int x = 0; x < xSize; x++) {
      for (int z = 0; z < zSize; z++) {
        int worldX = xPos + x;
        int worldZ = zPos + z;
        int idx = x + y * xSize + z * xSize * ySize;
        modCache[idx] =
            manager.GetMod({(float)worldX, (float)y, (float)worldZ});
      }
    }
  }
}

void ChunkPrefab::PopulateBlocks(const std::vector<int> &heightCache,
                                 const std::vector<float> &caveDensityCache,
                                 const std::vector<Uint8> &modCache,
                                 std::vector<bool> &solidCache,
                                 ChunkManager &manager) {
  if (this->blocks.size() != (size_t)xSize * ySize * zSize) {
    this->blocks.assign(xSize * ySize * zSize, 0);
  }

  const int heightCacheWidth = xSize + 2;

  for (int x = 0; x < xSize; x++) {
    for (int z = 0; z < zSize; z++) {
      int heightIdx = (x + 1) + (z + 1) * heightCacheWidth;
      int height = heightCache[heightIdx];

      for (int y = 0; y < ySize; y++) {
        int worldX = xPos + x;
        int worldZ = zPos + z;
        int idx = x + y * xSize + z * xSize * ySize;

        Uint8 mod = modCache[idx];
        bool isSolid = false;
        Uint8 blockID = 0;

        if (mod != 255) {
          isSolid = (mod != 0 && mod != 5);
          blockID = mod;
        } else {
          if (y == 0) {
            isSolid = true;
            blockID = 4; // Bedrock
          } else if (y < 0 || y >= ySize) {
            isSolid = false;
          } else {
            const int seaLevel = 35;
            float cont = PerlinNoise2D(
                {(float)worldX * 0.005f, (float)worldZ * 0.005f}, 3, 0.5f);

            bool isCave = false;
            if (y >= CaveMinY && y <= CaveMaxY) {
              int cIdx = x + (y - CaveMinY) * xSize +
                         z * xSize * (CaveMaxY - CaveMinY + 1);
              if (caveDensityCache[cIdx] < 0.015f) {
                isCave = true;
              }
            }

            if (y > height || isCave) {
              if (y < seaLevel && cont < -0.1f) {
                isSolid = false;
                blockID = 5; // Water
              } else {
                isSolid = false;
                blockID = 0; // Air
              }
            } else {
              isSolid = true;

              const int beachLevel = 37;
              bool isBeach = (height <= beachLevel &&
                              height >= beachLevel - 3 && cont < -0.1f);
              bool isUnderwaterFloor = (height < seaLevel);

              if (height - y > 3) {
                blockID = 3; // Stone
              } else if (isBeach || isUnderwaterFloor) {
                blockID = 8; // Sand
              } else if (height - y > 0) {
                blockID = 2; // Dirt
              } else {
                blockID = 1; // Grass
              }
            }
          }
        }
        solidCache[idx] = isSolid;
        this->blocks[idx] = blockID;
      }
    }
  }
}

void ChunkPrefab::SimulateWaterSpread(std::vector<bool> &solidCache) {
  const int seaLevel = 35;
  for (int x = 1; x < xSize - 1; x++) {
    for (int z = 1; z < zSize - 1; z++) {
      for (int y = 1; y < seaLevel - 1; y++) {
        int idx = x + y * xSize + z * xSize * ySize;
        if (this->blocks[idx] == 0) { // Air
          bool nearWater = false;
          if (this->blocks[idx - 1] == 5)
            nearWater = true;
          else if (this->blocks[idx + 1] == 5)
            nearWater = true;
          else if (this->blocks[idx - (xSize * ySize)] == 5)
            nearWater = true;
          else if (this->blocks[idx + (xSize * ySize)] == 5)
            nearWater = true;
          else if (this->blocks[idx + xSize] == 5)
            nearWater = true; // Above

          if (nearWater) {
            this->blocks[idx] = 5;
            solidCache[idx] = false;
          }
        }
      }
    }
  }
}

void ChunkPrefab::GenerateVegetation(const std::vector<int> &heightCache,
                                     const std::vector<Uint8> &modCache,
                                     std::vector<bool> &solidCache) {
  const int heightCacheWidth = xSize + 2;
  for (int x = 2; x < xSize - 2; x++) {
    for (int z = 2; z < zSize - 2; z++) {
      int heightIdx = (x + 1) + (z + 1) * heightCacheWidth;
      int height = heightCache[heightIdx];

      if (height < CaveMinY || height > ySize - 10)
        continue;

      float treeNoise = PerlinNoise2D(
          {(float)(xPos + x) * 0.5f, (float)(zPos + z) * 0.5f}, 1, 0.5f);
      if (treeNoise > 0.65f && treeNoise < 0.7f || treeNoise > 0.9f) {
        int idx = x + height * xSize + z * xSize * ySize;
        if (this->blocks[idx] == 1) { // On Grass
          int trunkHeight = 4 + (int)(treeNoise * 4.0f);
          for (int ty = 1; ty <= trunkHeight; ty++) {
            if (height + ty < ySize) {
              int tidx = x + (height + ty) * xSize + z * xSize * ySize;
              if (modCache[tidx] == 255) {
                solidCache[tidx] = true;
                this->blocks[tidx] = 6; // Wood
              }
            }
          }
          for (int ly = trunkHeight - 1; ly <= trunkHeight + 1; ly++) {
            for (int lx = -2; lx <= 2; lx++) {
              for (int lz = -2; lz <= 2; lz++) {
                int nx = x + lx, ny = height + ly, nz = z + lz;
                if (nx >= 0 && nx < xSize && ny >= 0 && ny < ySize && nz >= 0 &&
                    nz < zSize) {
                  int lidx = nx + ny * xSize + nz * xSize * ySize;
                  if (this->blocks[lidx] == 0 && modCache[lidx] == 255) {
                    if (lx * lx + lz * lz +
                            (ly - trunkHeight) * (ly - trunkHeight) <=
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
      }
    }
  }
}

void ChunkPrefab::GenerateMesh(const std::vector<bool> &solidCache,
                               const std::vector<int> &heightCache,
                               ChunkManager &manager) {
  int estimatedFaces =
      xSize * zSize * 8; // Adjust estimation based on new complexity
  this->allFaces.reserve(estimatedFaces);

  for (int side = 0; side < 6; side++) {
    int d = (side < 2) ? 2 : (side < 4 ? 0 : 1); // normal axis
    int u, v;
    if (d == 0) {
      u = 2;
      v = 1;
    } // X normal: Width=Z, Height=Y
    else if (d == 1) {
      u = 0;
      v = 2;
    } // Y normal: Width=X, Height=Z
    else {
      u = 0;
      v = 1;
    } // Z normal: Width=X, Height=Y

    int dims[3] = {xSize, ySize, zSize};

    for (int slice = 0; slice < dims[d]; slice++) {
      std::vector<Uint8> mask(dims[u] * dims[v], 0);

      for (int i = 0; i < dims[u]; i++) {
        for (int j = 0; j < dims[v]; j++) {
          int x[3];
          x[d] = slice;
          x[u] = i;
          x[v] = j;
          int idx = x[0] + x[1] * xSize + x[2] * xSize * ySize;
          Uint8 bid = this->blocks[idx];

          if (bid != 0) {
            bool visible = false;
            int nx[3] = {x[0], x[1], x[2]};
            if (side == 0)
              nx[2]++; // Front (+Z)
            else if (side == 1)
              nx[2]--; // Back (-Z)
            else if (side == 2)
              nx[0]++; // Right (+X)
            else if (side == 3)
              nx[0]--; // Left (-X)
            else if (side == 4)
              nx[1]++; // Top (+Y)
            else if (side == 5)
              nx[1]--; // Bottom (-Y)

            // Convert to world coordinates
            int worldX = nx[0] + xPos;
            int worldY = nx[1];
            int worldZ = nx[2] + zPos;

            bool neighborInBounds =
                (nx[0] >= 0 && nx[0] < xSize && nx[1] >= 0 && nx[1] < ySize &&
                 nx[2] >= 0 && nx[2] < zSize);

            Uint8 nBid = 0;

            if (neighborInBounds) {
              // Neighbor is within current chunk - use local cache
              int nIdx = nx[0] + nx[1] * xSize + nx[2] * xSize * ySize;
              nBid = this->blocks[nIdx];
            } else if (worldY >= 0 && worldY < ySize) {
              // Neighbor is outside chunk - calculate using same logic as
              // PopulateBlocks

              // Calculate terrain height for neighbor position
              int height = GetHeight({(float)worldX, (float)worldZ});

              // Check for bedrock
              if (worldY == 0) {
                nBid = 4; // Bedrock
              } else {
                const int seaLevel = 35;
                float cont = PerlinNoise2D(
                    {(float)worldX * 0.005f, (float)worldZ * 0.005f}, 3, 0.5f);

                // Check for caves using same logic
                bool isCave = false;
                if (worldY >= CaveMinY && worldY <= CaveMaxY) {
                  float n1 =
                      PerlinNoise({(float)worldX * 0.08f, (float)worldY * 0.08f,
                                   (float)worldZ * 0.08f},
                                  2, 0.5f);
                  float n2 =
                      PerlinNoise({(float)worldX * 0.1f + 100.0f,
                                   (float)worldY * 0.1f, (float)worldZ * 0.1f},
                                  2, 0.5f);
                  float density = (n1 * n1) + (n2 * n2);
                  if (density < 0.015f) {
                    isCave = true;
                  }
                }

                // Determine block type using same logic as PopulateBlocks
                if (worldY > height || isCave) {
                  if (worldY < seaLevel && cont < -0.1f) {
                    nBid = 5; // Water
                  } else {
                    nBid = 0; // Air
                  }
                } else {
                  // Solid terrain
                  const int beachLevel = 37;
                  bool isBeach = (height <= beachLevel &&
                                  height >= beachLevel - 3 && cont < -0.1f);
                  bool isUnderwaterFloor = (height < seaLevel);

                  if (height - worldY > 3) {
                    nBid = 3; // Stone
                  } else if (isBeach || isUnderwaterFloor) {
                    nBid = 8; // Sand
                  } else if (height - worldY > 0) {
                    nBid = 2; // Dirt
                  } else {
                    nBid = 1; // Grass
                  }
                }
              }
            } else {
              // Outside Y bounds
              nBid = 0; // Air
            }

            // Determine visibility based on block types
            if (bid == 5) {
              // Water: only show faces against air
              visible = (nBid == 0);
            } else {
              // Opaque blocks: show against air or water (transparent blocks)
              visible = (nBid == 0 || nBid == 5);
            }

            if (visible) {
              mask[i + j * dims[u]] = bid;
            }
          }
        }
      }

      // Greedily merge mask
      std::vector<bool> visited(dims[u] * dims[v], false);
      for (int j = 0; j < dims[v]; j++) {
        for (int i = 0; i < dims[u]; i++) {
          int mIdx = i + j * dims[u];
          if (mask[mIdx] != 0 && !visited[mIdx]) {
            Uint8 bid = mask[mIdx];
            int w = 1, h = 1;

            // Expand width
            for (int i2 = i + 1;
                 i2 < dims[u] && mask[i2 + j * dims[u]] == bid &&
                 !visited[i2 + j * dims[u]];
                 i2++) {
              w++;
            }

            // Expand height
            for (int j2 = j + 1; j2 < dims[v]; j2++) {
              bool rowMatch = true;
              for (int i2 = i; i2 < i + w; i2++) {
                if (mask[i2 + j2 * dims[u]] != bid ||
                    visited[i2 + j2 * dims[u]]) {
                  rowMatch = false;
                  break;
                }
              }
              if (rowMatch)
                h++;
              else
                break;
            }

            // Add face
            int x[3];
            x[d] = slice;
            x[u] = i;
            x[v] = j;
            Uint16 posIndex = Vector3((float)x[0], (float)x[1], (float)x[2])
                                  .ToIndex(xSize, ySize);
            this->allFaces.push_back({posIndex, (Uint8)side, bid, (Uint8)w,
                                      (Uint8)h, BlockDef[bid].Water});

            // Mark visited
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