#include "../../include/common/Chunck.hpp"
#include "../../include/common/PerlinNoise.hpp"
#include <SDL3/SDL_stdinc.h>
#include <queue>

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
    {0.15f, 50},  // Hills
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

  // 5. Water Spread Logic - DISABLED: Water is now placed directly in
  // SimulateWaterSpread(solidCache);

  // 6. Vegetation (Trees)
  GenerateVegetation(heightCache, modCache, solidCache);

  isDirty = false;
  GenerateLighting();
  PropagateSunlight();
  // 7. Generate Mesh
  GenerateMesh(manager);

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

            if (y > height) {
              // Above terrain - fill with water if below sea level
              if (y < seaLevel) {
                isSolid = false;
                blockID = 5; // Water
              } else {
                isSolid = false;
                blockID = 0; // Air
              }
            } else if (isCave) {
              // Inside a cave - always air, even below sea level
              isSolid = false;
              blockID = 0; // Air
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

void ChunkPrefab::GenerateMesh(ChunkManager &manager) {
  this->allFaces.clear();
  int estimatedFaces = xSize * zSize * 6;
  this->allFaces.reserve(estimatedFaces);

  for (int side = 0; side < 6; side++) {
    int d = (side < 2) ? 2 : (side < 4 ? 0 : 1); // normal axis
    int u = (d == 0) ? 2 : 0;                    // u axis
    int v = (d == 1) ? 2 : 1;                    // v axis
    if (d == 2) {
      u = 0;
      v = 1;
    }

    int dims[3] = {xSize, ySize, zSize};

    for (int slice = 0; slice < dims[d]; slice++) {
      std::vector<Uint8> mask(dims[u] * dims[v], 0);
      std::vector<Uint8> lightMask(dims[u] * dims[v], 0);

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

            if (nx[0] >= 0 && nx[0] < xSize && nx[1] >= 0 && nx[1] < ySize &&
                nx[2] >= 0 && nx[2] < zSize) {
              int nIdx = nx[0] + nx[1] * xSize + nx[2] * xSize * ySize;
              nBid = this->blocks[nIdx];
            } else {
              nBid = manager.GetBlockID(
                  {(float)worldX, (float)worldY, (float)worldZ});
            }

            if (bid == 5) {
              visible = (nBid == 0);
            } else {
              visible = (nBid == 0 || nBid == 5);
            }

            if (visible) {
              mask[i + j * dims[u]] = bid;

              // FIXED: Get light level from neighbor, works for chunk
              // boundaries now
              Uint8 light = GetCombinedLight(nx[0], nx[1], nx[2], manager);

              lightMask[i + j * dims[u]] = light;
            }
          }
        }
      }

      // Greedy Meshing
      std::vector<bool> visited(dims[u] * dims[v], false);
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
                                               BlockDef[bid].Water});

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
Uint8 ChunkPrefab::GetCombinedLight(int x, int y, int z,
                                    ChunkManager &manager) {
  // If within bounds, get from this chunk directly
  if (x >= 0 && x < xSize && y >= 0 && y < ySize && z >= 0 && z < zSize) {
    if (!lightData.empty()) {
      int idx = x + y * xSize + z * xSize * ySize;
      return std::max(lightData[idx].sunlight, lightData[idx].blockLight);
    }
    return 15;
  }

  // Out of bounds - query manager safely (no recursion)
  return manager.GetLightLevel(
      {(float)(x + xPos), (float)y, (float)(z + zPos)});
}
void ChunkPrefab::GenerateLighting() {
  if (lightData.size() != blocks.size()) {
    lightData.assign(blocks.size(), {0, 0});
  }

  // Phase 1: Sunlight from top
  for (int x = 0; x < xSize; x++) {
    for (int z = 0; z < zSize; z++) {
      Uint8 sunLevel = 15;

      for (int y = ySize - 1; y >= 0; y--) {
        int idx = x + y * xSize + z * xSize * ySize;
        Uint8 blockID = blocks[idx];

        if (blockID == 0) { // Air
          lightData[idx].sunlight = sunLevel;
        } else if (blockID == 5) { // Water (transparent)
          sunLevel = (sunLevel > 2) ? sunLevel - 2 : 0;
          lightData[idx].sunlight = sunLevel;
        } else { // Solid block
          sunLevel = 0;
          lightData[idx].sunlight = 0;
        }
      }
    }
  }
}
void ChunkPrefab::PropagateSunlight() {
  std::queue<Vector3> lightQueue;

  // Add all lit blocks to queue
  for (int x = 0; x < xSize; x++) {
    for (int y = 0; y < ySize; y++) {
      for (int z = 0; z < zSize; z++) {
        int idx = x + y * xSize + z * xSize * ySize;
        if (lightData[idx].sunlight > 0) {
          lightQueue.push({(float)x, (float)y, (float)z});
        }
      }
    }
  }

  // BFS propagation
  while (!lightQueue.empty()) {
    Vector3 pos = lightQueue.front();
    lightQueue.pop();

    int idx = pos.x + pos.y * xSize + pos.z * xSize * ySize;
    Uint8 currentLight = lightData[idx].sunlight;

    if (currentLight <= 1)
      continue;

    // Check all 6 neighbors
    for (int i = 0; i < 6; i++) {
      int nx = pos.x + (int)Direction[i].x;
      int ny = pos.y + (int)Direction[i].y;
      int nz = pos.z + (int)Direction[i].z;

      if (nx < 0 || nx >= xSize || ny < 0 || ny >= ySize || nz < 0 ||
          nz >= zSize)
        continue;

      int nIdx = nx + ny * xSize + nz * xSize * ySize;
      Uint8 neighborBlock = blocks[nIdx];

      // Skip solid blocks
      if (neighborBlock != 0 && neighborBlock != 5)
        continue;

      Uint8 newLight = currentLight - 1;
      if (neighborBlock == 5)
        newLight = (newLight > 2) ? newLight - 1 : 0;

      if (newLight > lightData[nIdx].sunlight) {
        lightData[nIdx].sunlight = newLight;
        lightQueue.push({(float)nx, (float)ny, (float)nz});
      }
    }
  }
}