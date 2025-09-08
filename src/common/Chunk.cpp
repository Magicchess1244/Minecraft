#include "Chunk.hpp"
#include "PerlinNoise.hpp"
#include "BlockTypes.hpp"

const Vector3 Direction[6] = {
    {0, 0, -1},  // Front
    {0, 0, 1},   // Back
    {1, 0, 0},   // Right
    {-1, 0, 0},  // Left
    {0, 1, 0},   // Top
    {0, -1, 0}   // Bottom
};

void ChunkPrefab::GenerateChunk() {
    this->GenerateChunkSurface();
    // this->GenerateChunkCaves();
    this->VisableFaces();
}

void ChunkPrefab::GenerateChunkSurface() {
    for (int x = 0; x < this->xSize; x++) {
        for (int z = 0; z < this->zSize; z++) {
            // Calculate world position
            float worldX = xPos + x;
            float worldZ = zPos + z;
            
            // Generate height using multiple octaves of Perlin noise for more realistic terrain
            float heightNoise = 0.0f;
            heightNoise += PerlinNoise({worldX, 0, worldZ}, 1, 0.5f) * 32.0f;  // Large features
            heightNoise += PerlinNoise({worldX, 0, worldZ}, 2, 0.25f) * 16.0f; // Medium features
            heightNoise += PerlinNoise({worldX, 0, worldZ}, 4, 0.125f) * 8.0f; // Small features
            
            int surfaceHeight = static_cast<int>(heightNoise + 64.0f); // Base height at 64
            surfaceHeight = std::max(1, std::min(surfaceHeight, 128)); // Clamp between 1 and 128
            
            // Generate terrain layers
            for (int y = 0; y < this->ySize; y++) {
                Vector3 BlockPos = {(float)x, (float)y, (float)z};
                int worldY = y;
                
                if (worldY == 0) {
                    // Bedrock layer at bottom
                    Blocks[BlockPos] = static_cast<int>(BlockType::BEDROCK);
                } else if (worldY < surfaceHeight - 4) {
                    // Stone layer
                    Blocks[BlockPos] = static_cast<int>(BlockType::STONE);
                } else if (worldY < surfaceHeight - 1) {
                    // Dirt layer
                    Blocks[BlockPos] = static_cast<int>(BlockType::DIRT);
                } else if (worldY == surfaceHeight - 1) {
                    // Grass layer
                    Blocks[BlockPos] = static_cast<int>(BlockType::GRASS);
                } else if (worldY < 64) {
                    // Water level (sea level at 64)
                    Blocks[BlockPos] = static_cast<int>(BlockType::WATER);
                }
                // Air above surface and water
            }
        }
    }
}

void ChunkPrefab::GenerateChunkCaves() {
    for (int x = 0; x < this->xSize; x++) {
        for (int z = 0; z < this->xSize; z++) {
            for (int y = 2; y < this->ySize; y++) {
                Vector3 BlockPos = {(float)x, (float)y, (float)z};
                auto it = Blocks.find(BlockPos);
                if (it == Blocks.end()) continue;
                int blockID = it->second;

                float Hole = PerlinNoise({(float)xPos + x, (float)y, (float)z}, 3, 0.1f);

                bool CheeseCave = Hole <= -0.9f || Hole >= 0.9f;
                bool NodleCave = (0.04f > Hole && Hole > -0.04f);

                if ((CheeseCave || NodleCave) && (blockID != 4 && blockID != 5)) {
                    this->Blocks.erase(BlockPos);
                }
            }
        }
    }
}

void ChunkPrefab::VisableFaces() {
    // Rebuild the list of visible faces from current Blocks
    this->allFaces.clear();
    this->allFaces.reserve(7000);

    for (int y = 0; y < this->ySize; y++) {
        for (int x = 0; x < this->xSize; x++) {
            for (int z = 0; z < this->zSize; z++) {
                Vector3 blockPos = {(float)x, (float)y, (float)z};

                auto it = Blocks.find(blockPos);
                if (it == Blocks.end()) continue;

                int blockID = it->second;
                
                // Skip air blocks
                if (blockID == static_cast<int>(BlockType::AIR)) continue;

                for (int i = 0; i < 6; i++) {
                    Vector3 NextBlockPos = blockPos + Direction[i];
                    auto blockIt = Blocks.find(NextBlockPos);
                    
                    bool shouldDrawFace = false;
                    
                    if (blockIt == Blocks.end()) {
                        // Block is outside chunk bounds, draw face
                        shouldDrawFace = true;
                    } else {
                        int neighborBlockID = blockIt->second;
                        
                        // Draw face if neighbor is air or transparent
                        if (neighborBlockID == static_cast<int>(BlockType::AIR) || 
                            g_BlockRegistry.isTransparent(neighborBlockID)) {
                            shouldDrawFace = true;
                        }
                    }
                    
                    if (shouldDrawFace) {
                        Vector3 ChunkWorldPos = {(float)this->xPos, 0, (float)this->zPos};
                        Vector3 world = blockPos + ChunkWorldPos;
                        DrawnFace face = {world, i, blockID, false};
                        this->allFaces.push_back(face);
                    }
                }
            }
        }
    }
}
