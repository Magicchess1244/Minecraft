#include "../../include/common/Chunck.hpp"
#include "../../include/common/PerlinNoise.hpp"

constexpr float CaveThreshold = -0.15f; // Lower threshold = more caves
constexpr int CaveMinY = 2;
constexpr int CaveMaxY = 40;

const Vector3 Direction[6] = {
    {0, 0, 1},  // Front
    {0, 0, -1}, // Back
    {1, 0, 0},  // Right
    {-1, 0, 0}, // Left
    {0, 1, 0},  // Top
    {0, -1, 0}  // Bottom
};

// Helper function to check if a block should exist (considering caves)
bool ChunkPrefab::isSolidBlock(int worldX, int worldY, int worldZ, int terrainHeight) {
  // Above terrain height = air
  if (worldY > terrainHeight)
    return false;

  // Below cave range = always solid
  if (worldY < CaveMinY)
    return true;

  // Within cave range - use 3D Perlin noise for caves
  if (worldY <= CaveMaxY) {
    // Use larger scale for bigger caves, more octaves for detail
    float caveNoise = PerlinNoise(
        {(float)worldX * 0.1f, (float)worldY * 0.1f, (float)worldZ * 0.1f}, 3,
        0.5f);
    if (caveNoise < CaveThreshold) { // Note: less than, not greater than
      return false;                  // This is a cave
    }
  }

  return true; // Solid block
}

void ChunkPrefab::GenerateChunk() {
  // Pre-calculate all heights in the chunk
  std::vector<int> heights(xSize * zSize);
  for (Uint8 x = 0; x < xSize; x++) {
    for (Uint8 z = 0; z < zSize; z++) {
      heights[x * zSize + z] =
          BaseHeight + (int)(PerlinNoise({(float)(xPos + x), 0, (float)(zPos + z)}, 4,
                                 Frecuence) *
                     HeightVar);
    }
  }

  this->allFaces.clear();
  this->allFaces.reserve(xSize * zSize * 10);

  // Check all 4 horizontal directions for side faces
  static const int offsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
  static const int faceIndices[4] = {3, 2, 1, 0}; // Left, Right, Back, Front

  // Build a 3D grid of solid blocks
  for (Uint8 x = 0; x < xSize; x++) {
    for (Uint8 z = 0; z < zSize; z++) {
      int height = heights[x * zSize + z];
      Vector3 ChunkWorldPos = {(float)this->xPos, 0, (float)this->zPos};

      // Iterate through all Y levels from 0 to height
      for (int y = 0; y <= height; y++) {
        int worldX = xPos + x;
        int worldY = y;
        int worldZ = zPos + z;

        // Check if this block is solid
        if (!isSolidBlock(worldX, worldY, worldZ, height))
          continue;

        // Determine block type based on depth
        int BlockID = 1; // Grass
        if (height - y > 0) BlockID = 2; // Dirt
        if (height - y > 3) BlockID = 3; // Stone
        if (y == 0) BlockID = 4;

        // Check top face (index 4)
        int topY = y + 1;
        int topNeighborHeight;
        topNeighborHeight = heights[x * zSize + z]; // Same column

        if (!isSolidBlock(worldX, topY, worldZ, topNeighborHeight)) {
          Vector3 blockPos = {(float)x, (float)y, (float)z};
          this->allFaces.push_back(
              {blockPos + ChunkWorldPos, 4, BlockID, false});
        }

        // Check bottom face (index 5)
        if (y > 0) {
          int bottomY = y - 1;
          if (!isSolidBlock(worldX, bottomY, worldZ, height)) {
            Vector3 blockPos = {(float)x, (float)y, (float)z};
            this->allFaces.push_back(
                {blockPos + ChunkWorldPos, 5, BlockID, false});
          }
        }

        // Check all 4 horizontal directions for side faces
        for (int dir = 0; dir < 4; dir++) {
          int nx = x + offsets[dir][0];
          int nz = z + offsets[dir][1];

          int nWorldX = xPos + nx;
          int nWorldZ = zPos + nz;

          // Get neighbor height
          int neighborHeight;
          if (nx < 0 || nx >= xSize || nz < 0 || nz >= zSize) {
            // Outside chunk - calculate height
            neighborHeight =
                35 + (int)(PerlinNoise({(float)nWorldX, 0, (float)nWorldZ}, 4,
                                       0.1f) *
                           HeightVar);
          } else {
            neighborHeight = heights[nx * zSize + nz];
          }

          // Check if neighbor is solid
          bool neighborSolid =
              isSolidBlock(nWorldX, y, nWorldZ, neighborHeight);

          // Draw face if neighbor is air/cave
          if (!neighborSolid) {
            Vector3 blockPos = {(float)x, (float)y, (float)z};
            this->allFaces.push_back(
                {blockPos + ChunkWorldPos, faceIndices[dir], BlockID, true});
          }
        }
      }
    }
  }
}