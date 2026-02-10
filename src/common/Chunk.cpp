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
  Uint8 Modifications = manager.GetMod({(float)worldX, (float)worldY, (float)worldZ});
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
  
  // ========== OPTIMIZATION 1: Cache all heights (including neighbors) ==========
  // We need heights for neighbors too, so cache (xSize+2) x (zSize+2)
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
            {(float)worldX * 0.1f, (float)y * 0.1f, (float)worldZ * 0.1f}, 3, 0.5f);
      }
    }
  }
  
  // ========== OPTIMIZATION 3: Cache modifications for the entire chunk ==========
  std::vector<Uint8> modCache(xSize * ySize * zSize, 255);
  for (int y = 0; y < ySize; y++) {
    for (int x = 0; x < xSize; x++) {
      for (int z = 0; z < zSize; z++) {
        int worldX = xPos + x;
        int worldZ = zPos + z;
        int idx = x + y * xSize + z * xSize * ySize;
        modCache[idx] = manager.GetMod({(float)worldX, (float)y, (float)worldZ});
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
            isSolid = true; // Bedrock floor
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
  
  // ========== OPTIMIZATION 5: Cache neighbor solidity (including neighbor chunks) ==========
  // For neighbors outside the chunk, we need to check them separately
  auto isNeighborSolid = [&](int worldX, int worldY, int worldZ, int neighborHeight) -> bool {
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
  
  // ========== OPTIMIZATION 7: Generate faces using cached data ==========
  for (int x = 0; x < xSize; x++) {
    for (int z = 0; z < zSize; z++) {
      int heightIdx = (x + 1) + (z + 1) * heightCacheWidth;
      int height = heightCache[heightIdx];
      
      for (int y = 0; y < ySize; y++) {
        int idx = x + y * xSize + z * xSize * ySize;
        
        // Skip if not solid
        if (!solidCache[idx])
          continue;
        
        int worldX = xPos + x;
        int worldY = y;
        int worldZ = zPos + z;
        
        Uint8 BlockID = blockIDCache[idx];
        Uint16 posIndex = Vector3((float)x, (float)worldY, (float)z)
                              .ToIndex(ChunkPrefab::xSize, ChunkPrefab::ySize);
        
        // Top Face
        if (y + 1 < ySize) {
          int topIdx = x + (y + 1) * xSize + z * xSize * ySize;
          if (!solidCache[topIdx]) {
            this->allFaces.push_back(
                {posIndex, 4, BlockID, BlockDef[BlockID].Water});
          }
        } else {
          // Top of chunk, always add face
          this->allFaces.push_back(
              {posIndex, 4, BlockID, BlockDef[BlockID].Water});
        }
        
        // Bottom Face
        if (y > 0) {
          int bottomIdx = x + (y - 1) * xSize + z * xSize * ySize;
          if (!solidCache[bottomIdx]) {
            this->allFaces.push_back(
                {posIndex, 5, BlockID, BlockDef[BlockID].Water});
          }
        }
        
        // Side Faces
        for (int dir = 0; dir < 4; dir++) {
          int nx = worldX + offsets[dir][0];
          int nz = worldZ + offsets[dir][1];
          
          // Get neighbor terrain height from cache
          int nHeightIdx = (x + 1 + offsets[dir][0]) + 
                          (z + 1 + offsets[dir][1]) * heightCacheWidth;
          int nHeight = heightCache[nHeightIdx];
          
          bool isSolid = isNeighborSolid(nx, y, nz, nHeight);
          
          if (!isSolid) {
            this->allFaces.push_back(
                {posIndex, faceIndices[dir], BlockID, BlockDef[BlockID].Water});
          }
        }
      }
    }
  }
  
  isDirty = false;
  needsMeshUpdate = true;
  allFaces.shrink_to_fit();
}