#include "../../include/common/Chunck.hpp"
#include "../../include/common/PerlinNoise.hpp"
#include <SDL3/SDL_stdinc.h>

constexpr float CaveThreshold = -0.18f;
constexpr int CaveMinY = 2;
constexpr int CaveMaxY = 38;

const Vector3 Direction[6] = {
    {0, 0, 1},  // Front
    {0, 0, -1}, // Back
    {1, 0, 0},  // Right
    {-1, 0, 0}, // Left
    {0, 1, 0},  // Top
    {0, -1, 0}  // Bottom
};

constexpr HeightsDif ContinentelnessHeight[5] = {
    {0.15f, ChunkPrefab::ySize * 0.8f},
    {-0.15f, ChunkPrefab::ySize * 0.45f},
    {-0.35f, ChunkPrefab::ySize * 0.45f},
    {-0.65f, 30},
    {-0.9f, 15}};

int GetBaseHeight(float ValueNoise) {
  for (int i = 0; i < 5; i++) {
    if (ValueNoise >= ContinentelnessHeight[i].x) {
      return Lerp(ContinentelnessHeight[i].y, ContinentelnessHeight[i + 1].y,
                  (ValueNoise - ContinentelnessHeight[i + 1].x) /
                      (ContinentelnessHeight[i].x -
                       ContinentelnessHeight[i + 1].x)) +
             5;
    }
  }
  return ContinentelnessHeight[4].y;
}

int ChunkPrefab::GetHeight(Vector2 Pos) {
  return BaseHeight + (int)(PerlinNoise2D(Pos, 4, Frecuence) * HeightVar);
}

bool ChunkPrefab::isSolidBlock(int worldX, int worldY, int worldZ,
                               int terrainHeight, ChunkManager &manager) {
  int localX = worldX - xPos;
  int localY = worldY;
  int localZ = worldZ - zPos;

  // Priority 1: User modifications (placing/breaking)
  Uint8 Modifications =
      manager.GetMod({(float)worldX, (float)worldY, (float)worldZ});
  if (Modifications != 255) {
    return Modifications != 0; // 0 is Air (not solid), others are solid
  }

  // Priority 2: World boundaries
  if (worldY < 0 || worldY >= ySize)
    return false;
  if (worldY == 0)
    return true; // Bedrock floor

  // Priority 3: Natural terrain
  if (worldY > terrainHeight)
    return false;
  if (worldY < CaveMinY)
    return true;

  // Priority 4: Caves
  if (worldY <= CaveMaxY) {
    float caveNoise = PerlinNoise(
        {(float)worldX * 0.1f, (float)worldY * 0.1f, (float)worldZ * 0.1f}, 3,
        0.5f);
    if (caveNoise < CaveThreshold)
      return false;
  }

  return true;
}

void ChunkPrefab::GenerateChunk(ChunkManager &manager) {
  this->allFaces.clear();

  static const int offsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
  static const Uint8 faceIndices[4] = {3, 2, 1, 0}; // Left, Right, Back, Front

  // ========== OPTIMIZATION 1: Cache all heights (including neighbors)
  // ========== We need heights for neighbors too, so cache (xSize+2) x
  // (zSize+2)
  const int heightCacheWidth = xSize + 2;
  const int heightCacheDepth = zSize + 2;
  std::vector<int> heightCache(heightCacheWidth * heightCacheDepth);

  for (int x = -1; x <= xSize; x++) {
    for (int z = -1; z <= zSize; z++) {
      int cacheIdx = (x + 1) + (z + 1) * heightCacheWidth;
      heightCache[cacheIdx] = GetHeight({(float)(xPos + x), (float)(zPos + z)});
    }
  }

  // ========== OPTIMIZATION 2: Cache cave noise for the entire chunk ==========
  std::vector<float> caveCache(xSize * ySize * zSize);
  for (int y = CaveMinY; y <= CaveMaxY; y++) {
    for (int x = 0; x < xSize; x++) {
      for (int z = 0; z < zSize; z++) {
        int worldX = xPos + x;
        int worldZ = zPos + z;
        int idx = x + y * xSize + z * xSize * ySize;
        caveCache[idx] = PerlinNoise(
            {(float)worldX * 0.1f, (float)y * 0.1f, (float)worldZ * 0.1f}, 3,
            0.5f);
      }
    }
  }

  // ========== OPTIMIZATION 3: Cache modifications for the entire chunk
  // ==========
  std::vector<Uint8> modCache(xSize * ySize * zSize, 255);
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

  // ========== OPTIMIZATION 4: Cache solidity and block IDs ==========
  std::vector<bool> solidCache(xSize * ySize * zSize, false);
  std::vector<Uint8> blockIDCache(xSize * ySize * zSize, 0);

  for (int x = 0; x < xSize; x++) {
    for (int z = 0; z < zSize; z++) {
      int heightIdx = (x + 1) + (z + 1) * heightCacheWidth;
      int height = heightCache[heightIdx];

      for (int y = 0; y < ySize; y++) {
        int worldX = xPos + x;
        int worldZ = zPos + z;
        int idx = x + y * xSize + z * xSize * ySize;

        Uint8 mod = modCache[idx];

        // Check if solid
        bool isSolid = false;

        // Priority 1: User modifications
        if (mod != 255) {
          isSolid = (mod != 0);
          blockIDCache[idx] = mod;
        } else {
          // Priority 2: World boundaries
          if (y < 0 || y >= ySize) {
            isSolid = false;
          } else if (y == 0) {
            isSolid = true;        // Bedrock floor
            blockIDCache[idx] = 4; // Bedrock
          } else if (y > height) {
            // Priority 3: Natural terrain
            isSolid = false;
          } else if (y < CaveMinY) {
            isSolid = true;
            // Determine block type
            if (height - y > 3)
              blockIDCache[idx] = 3; // Stone
            else if (height - y > 0)
              blockIDCache[idx] = 2; // Dirt
            else
              blockIDCache[idx] = 1; // Grass
          } else {
            // Priority 4: Caves
            if (y <= CaveMaxY) {
              float caveNoise = caveCache[idx];
              if (caveNoise < CaveThreshold) {
                isSolid = false;
              } else {
                isSolid = true;
                // Determine block type
                if (height - y > 3)
                  blockIDCache[idx] = 3; // Stone
                else if (height - y > 0)
                  blockIDCache[idx] = 2; // Dirt
                else
                  blockIDCache[idx] = 1; // Grass
              }
            } else {
              isSolid = true;
              // Determine block type
              if (height - y > 3)
                blockIDCache[idx] = 3; // Stone
              else if (height - y > 0)
                blockIDCache[idx] = 2; // Dirt
              else
                blockIDCache[idx] = 1; // Grass
            }
          }
        }

        solidCache[idx] = isSolid;
      }
    }
  }

  // ========== OPTIMIZATION 5: Cache neighbor solidity (including neighbor
  // chunks) ========== For neighbors outside the chunk, we need to check them
  // separately
  auto isNeighborSolid = [&](int worldX, int worldY, int worldZ,
                             int neighborHeight) -> bool {
    int localX = worldX - xPos;
    int localZ = worldZ - zPos;

    // Check if neighbor is within current chunk
    if (localX >= 0 && localX < xSize && localZ >= 0 && localZ < zSize &&
        worldY >= 0 && worldY < ySize) {
      int idx = localX + worldY * xSize + localZ * xSize * ySize;
      return solidCache[idx];
    }

    // Neighbor is outside chunk, use original function
    return isSolidBlock(worldX, worldY, worldZ, neighborHeight, manager);
  };

  // ========== OPTIMIZATION 6: Estimate face count and reserve ==========
  // Rough estimate: assume ~6 faces per surface block
  int estimatedFaces = xSize * zSize * 8;
  this->allFaces.reserve(estimatedFaces);

  // ========== OPTIMIZATION 7: Generate faces using greedy meshing ==========
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

          if (solidCache[idx]) {
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

            if (nx[d] >= 0 && nx[d] < dims[d]) {
              int nIdx = nx[0] + nx[1] * xSize + nx[2] * xSize * ySize;
              visible = !solidCache[nIdx];
            } else {
              // Neighbor chunk check
              int hIdx = (nx[0] + 1) + (nx[2] + 1) * heightCacheWidth;
              // isNeighborSolid expects world coords for X and Z
              visible = !isNeighborSolid(nx[0] + xPos, nx[1], nx[2] + zPos,
                                         heightCache[hIdx]);
            }

            if (visible) {
              mask[i + j * dims[u]] = blockIDCache[idx];
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

  isDirty = false;
  needsMeshUpdate = true;
  allFaces.shrink_to_fit();
}