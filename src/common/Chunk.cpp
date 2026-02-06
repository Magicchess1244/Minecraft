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
  // Pre-calculate all heights in the chunk
  std::vector<int> heights(xSize * zSize);
  for(Uint8 x = 0; x < xSize; x++) {
    for(Uint8 z = 0; z < zSize; z++) {
      heights[x * zSize + z] = 35 + (int)(PerlinNoise({(float)(xPos + x), 0, (float)(zPos + z)}, 4, 0.1f) * 25);
    }
  }
  
  this->allFaces.clear();
  this->allFaces.reserve(xSize * zSize * 5); // Estimate: 1 top + ~4 sides average
  
  for(Uint8 x = 0; x < xSize; x++) {
    for(Uint8 z = 0; z < zSize; z++) {
      int height = heights[x * zSize + z];
      Vector3 ChunkWorldPos = {(float)this->xPos, 0, (float)this->zPos};
      
      // Top face - always visible
      Vector3 topPos = {(float)x, (float)height, (float)z};
      this->allFaces.push_back({topPos + ChunkWorldPos, 4, 1, false});
      
      // Check all 4 horizontal directions for side faces
      static const int offsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
      static const int faceIndices[4] = {2, 3, 1, 0}; // Left, Right, Front, Back
      
      for(int dir = 0; dir < 4; dir++) {
        int nx = x + offsets[dir][0];
        int nz = z + offsets[dir][1];
        int neighborHeight;
        
        // Check if neighbor is outside chunk bounds
        if(nx < 0 || nx >= xSize || nz < 0 || nz >= zSize) {
          // At chunk edge - calculate height in neighboring chunk using world coordinates
          neighborHeight = 35 + (int)(PerlinNoise({(float)(xPos + nx), 0, (float)(zPos + nz)}, 4, 0.1f) * 25);
        } else {
          neighborHeight = heights[nx * zSize + nz];
        }
        
        // Draw side faces between this column and neighbor
        int diff = neighborHeight - height;
        
        if(diff < 0) {
          // Neighbor is lower or doesn't exist - draw faces going down
          int startY = height;
          int endY = neighborHeight;
          
          for(int y = startY; y > endY; y--) {
            // For left (dir 0) and right (dir 1) faces, use neighbor position
            // For front (dir 2) and back (dir 3) faces, use current position
            float faceX = (dir == 0 || dir == 1) ? (float)nx : (float)x;
            float faceZ = (float)z; // Keep Z at current position for all faces
            
            Vector3 facePos = {faceX, (float)y, faceZ};
            int BlockID = 1;
            if (height - facePos.y > 0) BlockID = 2; 
            if (height - facePos.y > 3) BlockID = 3; 
            this->allFaces.push_back({facePos + ChunkWorldPos, faceIndices[dir], BlockID, false});
          }
        }
      }
    }
  }
}