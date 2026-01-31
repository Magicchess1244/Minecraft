#include "../../include/common/Chunck.hpp"
#include "../../include/common/PerlinNoise.hpp"
#include <fstream>
#include <future>

const Vector3 Direction[6] = {
    {0, 0, -1}, // Front
    {0, 0, 1},  // Back
    {1, 0, 0},  // Right
    {-1, 0, 0}, // Left
    {0, 1, 0},  // Top
    {0, -1, 0}  // Bottom
};
void ChunkPrefab::GenerateChunk() {
  for(Uint8 x = 0; x < xSize; x++){
    for(Uint8 z = 0; z < zSize; z++){
      int Height = (int)(35); // + ( PerlinNoise({ (float)xPos + x, 0, (float)zPos + z }, 4, 0.1f) * 25));

      DrawnFace Face = {{(float)x, (float)Height, (float)z}, 4, 3, false};
      allFaces.push_back(Face);
    }
  }
}
/*
void ChunkPrefab::GenerateChunkCaves() {
  for (int x = 0; x < this->xSize; x++) {
    for (int z = 0; z < this->zSize; z++) {
      for (int y = 2; y < this->ySize; y++) {
        int blockID = getBlock(x, y, z);
        if (blockID == 0)
          continue; // Skip air

        float Hole =
            PerlinNoise({(float)xPos + x, (float)y, (float)zPos + z}, 3, 0.1f);

        bool CheeseCave = Hole <= -0.9f || Hole >= 0.9f;
        bool NodleCave = (0.04f > Hole && Hole > -0.04f);

        if ((CheeseCave || NodleCave) && (blockID != 4 && blockID != 5)) {
          getBlock(x, y, z) = 0; // Remove block (make it air)
        }
      }
    }
  }
}
  */