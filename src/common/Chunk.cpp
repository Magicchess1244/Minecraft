#include "../../include/common/Chunck.hpp"
#include "../../include/common/ChunkManager.hpp"
#include "../../include/common/PerlinNoise.hpp"

constexpr float CaveThreshold = -0.15f;
constexpr int CaveMinY = 2;
constexpr int CaveMaxY = 40;

bool ChunkPrefab::isSolidBlock(int worldX, int worldY, int worldZ,
                               int terrainHeight) {
  int localX = worldX - xPos;
  int localY = worldY;
  int localZ = worldZ - zPos;

  // Priority 1: User modifications (placing/breaking)
  if (localX >= 0 && localX < xSize && localY >= 0 && localY < ySize &&
      localZ >= 0 && localZ < zSize) {
    int Index = localX + localY * xSize + localZ * xSize * ySize;
    auto it = Modifications.find(Index);
    if (it != Modifications.end()) {
      return it->second != 0; // 0 is Air (not solid), others are solid
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

void ChunkPrefab::GenerateChunk(ChunkPrefab *negX, ChunkPrefab *posX,
                                ChunkPrefab *negZ, ChunkPrefab *posZ) {
  std::vector<int> heights(xSize * zSize);
  for (Uint8 x = 0; x < xSize; x++) {
    for (Uint8 z = 0; z < zSize; z++) {
      heights[x * zSize + z] =
          BaseHeight +
          (int)(PerlinNoise({(float)(xPos + x), 0, (float)(zPos + z)}, 4,
                            Frecuence) *
                HeightVar);
    }
  }

  this->allFaces.clear();
  this->allFaces.reserve(xSize * zSize * 10);

  static const int offsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
  static const int faceIndices[4] = {3, 2, 1, 0}; // Left, Right, Back, Front
  ChunkPrefab *neighbors[4] = {negX, posX, negZ, posZ};
  Vector3 ChunkWorldPos = {(float)this->xPos, 0, (float)this->zPos};

  for (Uint8 x = 0; x < xSize; x++) {
    for (Uint8 z = 0; z < zSize; z++) {
      int height = heights[x * zSize + z];

      for (int y = 0; y < ySize; y++) {
        int worldX = xPos + x;
        int worldY = y;
        int worldZ = zPos + z;

        if (!isSolidBlock(worldX, worldY, worldZ, height))
          continue;

        // Determine Block ID
        int Index = x + y * xSize + z * xSize * ySize;
        int BlockID = 1;
        auto it = Modifications.find(Index);
        if (it != Modifications.end()) {
          BlockID = it->second;
        } else {
          BlockID = 1; // Grass
          if (height - y > 0)
            BlockID = 2; // Dirt
          if (height - y > 3)
            BlockID = 3; // Stone
          if (y == 0)
            BlockID = 4;
        }

        // Top Face
        if (y + 1 < ySize) {
          if (!isSolidBlock(worldX, y + 1, worldZ, height))
            this->allFaces.push_back(
                {Vector3((float)worldX, (float)worldY, (float)worldZ), 4,
                 BlockID, BlockDef[BlockID].Water});
        } else {
          this->allFaces.push_back(
              {Vector3((float)worldX, (float)worldY, (float)worldZ), 4, BlockID,
               BlockDef[BlockID].Water});
        }

        // Bottom Face
        if (y > 0) {
          if (!isSolidBlock(worldX, y - 1, worldZ, height))
            this->allFaces.push_back(
                {Vector3((float)worldX, (float)worldY, (float)worldZ), 5,
                 BlockID, BlockDef[BlockID].Water});
        }

        // Side Faces
        for (int dir = 0; dir < 4; dir++) {
          int nx = worldX + offsets[dir][0];
          int nz = worldZ + offsets[dir][1];

          // For simplicity in culling, we calculate neighbor terrain height
          int nHeight =
              BaseHeight +
              (int)(PerlinNoise({(float)nx, 0, (float)nz}, 4, Frecuence) *
                    HeightVar);

          bool isSolid = false;
          if (neighbors[dir]) {
            // Check neighbor chunk
            // Only use neighbor chunk if the coordinate is ACTUALLY outside
            // this chunk's bounds in that direction.
            // Wait, offsets[dir] moves us 1 unit.
            // If x=0, dir=0 (left, -1), nx=-1 (relative to worldX).
            // worldX = xPos + x.
            // If x=0, worldX=xPos. nx = xPos - 1.
            // This is definitely outside current chunk.
            // But if x=1, worldX=xPos+1. nx=xPos. efficient logic:
            // Is nx inside [xPos, xPos+16)?
            if (nx >= xPos && nx < xPos + xSize && nz >= zPos &&
                nz < zPos + zSize) {
              isSolid = isSolidBlock(nx, y, nz, nHeight);
            } else {
              isSolid = neighbors[dir]->isSolidBlock(nx, y, nz, nHeight);
            }
          } else {
            isSolid = isSolidBlock(nx, y, nz, nHeight);
          }

          if (!isSolid) {
            this->allFaces.push_back(
                {Vector3((float)worldX, (float)worldY, (float)worldZ),
                 faceIndices[dir], BlockID, BlockDef[BlockID].Water});
          }
        }
      }
    }
  }
  isDirty = false;
}