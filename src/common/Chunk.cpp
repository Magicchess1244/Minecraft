#include "../../include/common/Chunck.hpp"
#include "../../include/common/PerlinNoise.hpp"
#include <SDL3/SDL_stdinc.h>

constexpr float CaveThreshold = -0.18f;
constexpr int CaveMinY = 2;
constexpr int CaveMaxY = 38;

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
  if (localX >= 0 && localX < xSize && localY >= 0 && localY < ySize &&
      localZ >= 0 && localZ < zSize) {
    int Index = localX + localY * xSize + localZ * xSize * ySize;
    Uint8 Modifications =
        manager.GetMod({(float)worldX, (float)worldY, (float)worldZ});
    if (Modifications != 255) {
      return Modifications != 0; // 0 is Air (not solid), others are solid
    }
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
  this->allFaces.reserve(xSize * zSize * 10);

  static const int offsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
  static const Uint8 faceIndices[4] = {3, 2, 1, 0}; // Left, Right, Back, Front
  Vector3 ChunkWorldPos = {(float)this->xPos, 0, (float)this->zPos};

  for (Uint8 x = 0; x < xSize; x++) {
    for (Uint8 z = 0; z < zSize; z++) {
      int height = GetHeight({(float)(xPos + x), (float)(zPos + z)});

      for (int y = 0; y < ySize; y++) {
        int worldX = xPos + x;
        int worldY = y;
        int worldZ = zPos + z;

        if (!isSolidBlock(worldX, worldY, worldZ, height, manager))
          continue;

        // Determine Block ID
        int Index = x + y * xSize + z * xSize * ySize;
        Uint8 BlockID = 1;
        Uint8 Modifications =
            manager.GetMod({(float)worldX, (float)worldY, (float)worldZ});
        if (Modifications != 255) {
          BlockID = Modifications;
        } else {
          BlockID = 1; // Grass
          if (height - y > 0)
            BlockID = 2; // Dirt
          if (height - y > 3)
            BlockID = 3; // Stone
          if (y == 0)
            BlockID = 4; // Bedrock
        }

        // Top Face
        if (y + 1 < ySize) {
          if (!isSolidBlock(worldX, y + 1, worldZ, height, manager))
            this->allFaces.push_back(
                {Vector3((float)x, (float)worldY, (float)z)
                     .ToIndex(ChunkPrefab::xSize, ChunkPrefab::ySize),
                 4, BlockID, BlockDef[BlockID].Water});
        } else {
          this->allFaces.push_back(
              {Vector3((float)x, (float)worldY, (float)z)
                   .ToIndex(ChunkPrefab::xSize, ChunkPrefab::ySize),
               4, BlockID, BlockDef[BlockID].Water});
        }

        // Bottom Face
        if (y > 0) {
          if (!isSolidBlock(worldX, y - 1, worldZ, height, manager))
            this->allFaces.push_back(
                {Vector3((float)x, (float)worldY, (float)z)
                     .ToIndex(ChunkPrefab::xSize, ChunkPrefab::ySize),
                 5, BlockID, BlockDef[BlockID].Water});
        }

        // Side Faces
        for (int dir = 0; dir < 4; dir++) {
          int nx = worldX + offsets[dir][0];
          int nz = worldZ + offsets[dir][1];

          // For simplicity in culling, we calculate neighbor terrain height
          int nHeight = GetHeight({(float)nx, (float)nz});

          bool isSolid = isSolid = isSolidBlock(nx, y, nz, nHeight, manager);

          if (!isSolid) {
            this->allFaces.push_back(
                {Vector3((float)x, (float)worldY, (float)z)
                     .ToIndex(ChunkPrefab::xSize, ChunkPrefab::ySize),
                 faceIndices[dir], BlockID, BlockDef[BlockID].Water});
          }
        }
      }
    }
  }
  isDirty = false;
  needsMeshUpdate = true;
  allFaces.shrink_to_fit();
}