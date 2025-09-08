#pragma once

#include "common.hpp"
#include <unordered_map>

enum class BlockType : uint8_t {
    AIR = 0,
    GRASS = 1,
    DIRT = 2,
    STONE = 3,
    BEDROCK = 4,
    WATER = 5,
    SAND = 6,
    WOOD = 7,
    LEAVES = 8,
    COBBLESTONE = 9,
    GLASS = 10,
    IRON_ORE = 11,
    COAL_ORE = 12,
    GOLD_ORE = 13,
    DIAMOND_ORE = 14
};

struct BlockDefinition {
    BlockType type;
    std::string name;
    Color color;
    bool isTransparent;
    bool isLiquid;
    bool isSolid;
    float hardness;
    
    // Texture coordinates (for future texture atlas support)
    struct TextureCoords {
        float top[4][2];    // Top face UV coordinates
        float bottom[4][2]; // Bottom face UV coordinates
        float side[4][2];   // Side face UV coordinates
    } textures;
    
    // Default constructor
    BlockDefinition() : type(BlockType::AIR), name("Air"), color({0, 0, 0}), 
                       isTransparent(true), isLiquid(false), isSolid(false), hardness(0.0f) {
        // Initialize default texture coordinates
        for (int i = 0; i < 4; i++) {
            textures.top[i][0] = textures.bottom[i][0] = textures.side[i][0] = 0.0f;
            textures.top[i][1] = textures.bottom[i][1] = textures.side[i][1] = 0.0f;
        }
    }
    
    BlockDefinition(BlockType t, const std::string& n, const Color& c, 
                   bool transparent = false, bool liquid = false, bool solid = true, 
                   float h = 1.0f)
        : type(t), name(n), color(c), isTransparent(transparent), 
          isLiquid(liquid), isSolid(solid), hardness(h) {
        // Initialize default texture coordinates (will be replaced with actual texture atlas)
        for (int i = 0; i < 4; i++) {
            textures.top[i][0] = textures.bottom[i][0] = textures.side[i][0] = 0.0f;
            textures.top[i][1] = textures.bottom[i][1] = textures.side[i][1] = 0.0f;
        }
    }
};

class BlockRegistry {
private:
    std::unordered_map<BlockType, BlockDefinition> blocks;
    
public:
    BlockRegistry() {
        initializeBlocks();
    }
    
    void initializeBlocks() {
        // Air
        blocks[BlockType::AIR] = BlockDefinition(
            BlockType::AIR, "Air", Color{0, 0, 0}, true, false, false, 0.0f);
        
        // Grass
        blocks[BlockType::GRASS] = BlockDefinition(
            BlockType::GRASS, "Grass", Color{124, 252, 0}, false, false, true, 0.6f);
        
        // Dirt
        blocks[BlockType::DIRT] = BlockDefinition(
            BlockType::DIRT, "Dirt", Color{139, 69, 19}, false, false, true, 0.5f);
        
        // Stone
        blocks[BlockType::STONE] = BlockDefinition(
            BlockType::STONE, "Stone", Color{128, 128, 128}, false, false, true, 1.5f);
        
        // Bedrock
        blocks[BlockType::BEDROCK] = BlockDefinition(
            BlockType::BEDROCK, "Bedrock", Color{64, 64, 64}, false, false, true, -1.0f);
        
        // Water
        blocks[BlockType::WATER] = BlockDefinition(
            BlockType::WATER, "Water", Color{0, 100, 200}, true, true, false, 0.0f);
        
        // Sand
        blocks[BlockType::SAND] = BlockDefinition(
            BlockType::SAND, "Sand", Color{238, 203, 173}, false, false, true, 0.5f);
        
        // Wood
        blocks[BlockType::WOOD] = BlockDefinition(
            BlockType::WOOD, "Wood", Color{139, 90, 43}, false, false, true, 2.0f);
        
        // Leaves
        blocks[BlockType::LEAVES] = BlockDefinition(
            BlockType::LEAVES, "Leaves", Color{34, 139, 34}, true, false, true, 0.2f);
        
        // Cobblestone
        blocks[BlockType::COBBLESTONE] = BlockDefinition(
            BlockType::COBBLESTONE, "Cobblestone", Color{105, 105, 105}, false, false, true, 2.0f);
        
        // Glass
        blocks[BlockType::GLASS] = BlockDefinition(
            BlockType::GLASS, "Glass", Color{200, 200, 255}, true, false, true, 0.3f);
        
        // Iron Ore
        blocks[BlockType::IRON_ORE] = BlockDefinition(
            BlockType::IRON_ORE, "Iron Ore", Color{192, 192, 192}, false, false, true, 3.0f);
        
        // Coal Ore
        blocks[BlockType::COAL_ORE] = BlockDefinition(
            BlockType::COAL_ORE, "Coal Ore", Color{64, 64, 64}, false, false, true, 3.0f);
        
        // Gold Ore
        blocks[BlockType::GOLD_ORE] = BlockDefinition(
            BlockType::GOLD_ORE, "Gold Ore", Color{255, 215, 0}, false, false, true, 3.0f);
        
        // Diamond Ore
        blocks[BlockType::DIAMOND_ORE] = BlockDefinition(
            BlockType::DIAMOND_ORE, "Diamond Ore", Color{185, 242, 255}, false, false, true, 3.0f);
    }
    
    const BlockDefinition& getBlock(BlockType type) const {
        auto it = blocks.find(type);
        if (it != blocks.end()) {
            return it->second;
        }
        return blocks.at(BlockType::AIR); // Return air as default
    }
    
    const BlockDefinition& getBlock(int blockId) const {
        return getBlock(static_cast<BlockType>(blockId));
    }
    
    bool isTransparent(int blockId) const {
        return getBlock(blockId).isTransparent;
    }
    
    bool isLiquid(int blockId) const {
        return getBlock(blockId).isLiquid;
    }
    
    bool isSolid(int blockId) const {
        return getBlock(blockId).isSolid;
    }
};

// Global block registry instance
extern BlockRegistry g_BlockRegistry;
