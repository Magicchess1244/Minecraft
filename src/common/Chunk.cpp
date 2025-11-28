#include "../../include/common/Chunck.hpp"
#include "../../include/common/PerlinNoise.hpp"

const Vector3 Direction[6] = {
    {0, 0, -1}, // Front
    {0, 0, 1},  // Back
    {1, 0, 0},  // Right
    {-1, 0, 0}, // Left
    {0, 1, 0},  // Top
    {0, -1, 0}  // Bottom
};

void ChunkPrefab::GenerateChunk() {
  this->GenerateChunkSurface();
  // this->GenerateChunkCaves();
  this->VisableFaces();
}

void ChunkPrefab::GenerateChunkSurface() {
  for (int x = 0; x < this->xSize; x++) {
    for (int z = 0; z < this->zSize; z++) {
      int Height = (int)(35); //+ ( PerlinNoise({ (float)xPos + x, 0,
                              //(float)zPos + z }, 4, 0.1f) * 25));
      int ActualHeight = Height;
      if (Height < 35) {
        Height = 35;
      }

      for (int y = Height; y > -1; y--) {
        if (y <= ActualHeight) {
          SetBlock(x, y, z, 3);
        } else {
          SetBlock(x, y, z, 5);
        }
      }
    }
  }
}

void ChunkPrefab::GenerateChunkCaves() {
  for (int x = 0; x < this->xSize; x++) {
    for (int z = 0; z < this->zSize; z++) { // Fixed typo: was xSize
      for (int y = 2; y < this->ySize; y++) {
        int blockID = GetBlock(x, y, z);
        if (blockID == 0)
          continue;

        float Hole =
            PerlinNoise({(float)xPos + x, (float)y, (float)z}, 3, 0.1f);

        bool CheeseCave = Hole <= -0.9f || Hole >= 0.9f;
        bool NodleCave = (0.04f > Hole && Hole > -0.04f);

        if ((CheeseCave || NodleCave) && (blockID != 4 && blockID != 5)) {
          SetBlock(x, y, z, 0);
        }
      }
    }
  }
}

void ChunkPrefab::VisableFaces() {
  this->allFaces
      .clear(); // Use clear instead of reserve on potentially non-empty
  this->allFaces.reserve(6000);

  for (int y = 0; y < this->ySize; y++) {
    for (int x = 0; x < this->xSize; x++) {
      for (int z = 0; z < this->zSize; z++) {
        int blockID = GetBlock(x, y, z);
        if (blockID == 0)
          continue;

        Vector3 blockPos = {(float)x, (float)y, (float)z};

        for (int i = 0; i < 6; i++) {
          Vector3 NextBlockPos = blockPos + Direction[i];
          int nx = (int)NextBlockPos.x;
          int ny = (int)NextBlockPos.y;
          int nz = (int)NextBlockPos.z;

          int neighborID = GetBlock(nx, ny, nz);

          // If neighbor is air (0) or if current is not water (5) and neighbor
          // is water (5) Wait, original logic: (blockIt == Blocks.end() ||
          // (blockID != 5 && blockIt->second == 5)) blockIt == end() means air
          // (0) in map (sparse). So if neighbor is 0 OR (current != 5 AND
          // neighbor == 5)

          if (neighborID == 0 || (blockID != 5 && neighborID == 5)) {
            Vector3 ChunkWorldPos = {(float)this->xPos, 0, (float)this->zPos};
            Vector3 world = blockPos + ChunkWorldPos;
            this->allFaces.push_back(
                {world, i, blockID, 0,
                 false}); // Added false for Transparent, though struct def
                          // might need check
          }
        }
      }
    }
  }
}