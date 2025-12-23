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

// Serialization implementation
bool ChunkPrefab::saveToFile(const std::string &path) {
  std::ofstream file(path, std::ios::binary);
  if (!file)
    return false;

  file.write(reinterpret_cast<const char *>(&xPos), sizeof(xPos));
  file.write(reinterpret_cast<const char *>(&zPos), sizeof(zPos));
  file.write(reinterpret_cast<const char *>(blocks.data()),
             blocks.size() * sizeof(int));

  return file.good();
}

bool ChunkPrefab::loadFromFile(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  if (!file)
    return false;

  file.read(reinterpret_cast<char *>(&xPos), sizeof(xPos));
  file.read(reinterpret_cast<char *>(&zPos), sizeof(zPos));
  file.read(reinterpret_cast<char *>(blocks.data()),
            blocks.size() * sizeof(int));

  isDirty = false;
  return file.good();
}

void ChunkPrefab::GenerateChunk() {
  this->GenerateChunkSurface();
  // this->GenerateChunkCaves();
  this->VisableFaces();
  this->isDirty = true; // Mark for saving
}

void ChunkPrefab::GenerateCollum(int x, int z) {
  int Height = (int)(35); // + ( PerlinNoise({ (float)xPos + x, 0, (float)zPos +
                          // z }, 4, 0.1f) * 25));
  int ActualHeight = Height;
  if (Height < 35) {
    Height = 35;
  }

  for (int y = Height; y > -1; y--) {
    if (y <= ActualHeight) {
      getBlock(x, y, z) = 3; // Stone
    } else {
      getBlock(x, y, z) = 5; // Water
    }
  }
}

void ChunkPrefab::GenerateChunkSurface() {
  // Multi-threaded column generation for massive speedup
  std::vector<std::future<void>> tasks;
  tasks.reserve(xSize * zSize);

  for (int x = 0; x < xSize; x++) {
    for (int z = 0; z < zSize; z++) {
      // Launch async tasks for parallel generation
      tasks.push_back(std::async(std::launch::async,
                                 &ChunkPrefab::GenerateCollum, this, x, z));
    }
  }

  // Wait for all columns to finish
  for (auto &task : tasks) {
    task.get();
  }
}

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

void ChunkPrefab::VisableFaces() {
  this->allFaces.reserve(6000);

  for (int y = 0; y < this->ySize; y++) {
    for (int x = 0; x < this->xSize; x++) {
      for (int z = 0; z < this->zSize; z++) {
        int blockID = getBlock(x, y, z);

        // Skip air blocks - major optimization
        if (blockID == 0) continue;

        // Check all 6 faces
        for (int i = 0; i < 6; i++) {
          int nx = x + (int)Direction[i].x;
          int ny = y + (int)Direction[i].y;
          int nz = z + (int)Direction[i].z;

          // Check if neighbor is outside chunk or is air/transparent
          bool shouldDraw = false;
          if (!isValidPos(nx, ny, nz)) {
            shouldDraw = true; // Edge of chunk
          } else {
            int neighborID = getBlock(nx, ny, nz);
            if (neighborID == 0 || (blockID != 5 && neighborID == 5)) {
              shouldDraw = true; // Neighbor is air or water
            }
          }

          if (shouldDraw) {
            Vector3 ChunkWorldPos = {(float)this->xPos, 0, (float)this->zPos};
            Vector3 blockPos = {(float)x, (float)y, (float)z};
            Vector3 world = blockPos + ChunkWorldPos;
            this->allFaces.push_back({world, i, blockID, 0});
          }
        }
      }
    }
  }
}