#include "../../include/common/ChunkManager.hpp"
#include "../../include/common/Chunck.hpp"
#include "../../include/common/ChunkCache.hpp"

constexpr int ySize = 64;
constexpr Biome Biomes[11] = {
    {20, 20, 0, 0, 20, 6},     // Ice
    {40, 20, 20, 0, 20, 6},    // Tundra
    {100, 20, 40, 0, 20, 7},   // Taiga
    {100, 40, 60, 20, 20, 4},  // Big Taiga
    {60, 60, 20, 20, 20, 3},   // Plains
    {80, 60, 20, 40, 20, 6},   // Forest
    {80, 60, 20, 60, 20, 5},   // Birch
    {100, 60, 20, 80, 20, 5},  // Dark Forest
    {100, 100, 60, 60, 20, 7}, // Jungle
    {60, 100, 0, 80, 20, 4},   // Desert
    {40, 80, 20, 0, 20, 5},    // Savanna
};
constexpr HeightsDif ContinentelnessHeight[8] = {{1, ySize},
                                                 {0.4f, ySize * 0.9f},
                                                 {0.15f, ySize * 0.8f},
                                                 {-0.15f, ySize * 0.45f},
                                                 {-0.35f, ySize * 0.45f},
                                                 {-0.65f, ySize * 0.1f},
                                                 {-0.9f, ySize * 0.09f},
                                                 {-1.0f, ySize}};
constexpr HeightsDif ErrotionHeight[10] = {
    {1, ySize * 0.05f},    {0.8f, ySize * 0.1f},
    {0.7f, ySize * 0.35f}, {0.4f, ySize * 0.35f},
    {0.3f, ySize * 0.11f}, {-0.2f, ySize * 0.2f},
    {-0.4f, ySize * 0.6f}, {-0.5f, ySize * 0.5f},
    {0.8f, ySize * 0.9f},  {0, ySize},
};
constexpr HeightsDif PeaksAndValiesHeight[6] = {
    {1, ySize},
    {0.8f, ySize * 0.9f},
    {0.5f, ySize * 0.95f},
    {0.05f, ySize * 0.35f},
    {-0.4f, ySize * 0.3f},
    {-0.9f, ySize * 0.1f},
};

int BlockSize = 50;

ChunkManager::ChunkManager() {
  cache = new ChunkCache(); // Initialize cache system
}

ChunkManager::~ChunkManager() {
  // Save all dirty chunks before destroying
  for (auto &pair : Chunks) {
    if (pair.second.isDirty) {
      cache->saveChunk(pair.second, (int)pair.first.x, (int)pair.first.z);
    }
  }
  delete cache;
}

ChunkPrefab &ChunkManager::get_chunk(Vector3 key) {
  key.y = 0;

  // Check if chunk is already loaded
  if (this->Chunks.find(key) == this->Chunks.end()) {
    // REMOVED: std::cout - major performance killer!
    CHUNK_LOG("Generating chunk at: " << key.x << ", " << key.z);

    ChunkPrefab newChunk;
    newChunk.xPos = (int)key.x * newChunk.xSize;
    newChunk.zPos = (int)key.z * newChunk.zSize;

    // Try to load from cache first
    if (cache->loadChunk(newChunk, (int)key.x, (int)key.z)) {
      CHUNK_LOG("Loaded chunk from cache");
      // Need to regenerate faces after loading
      newChunk.VisableFaces();
    } else {
      // Generate new chunk if not in cache
      newChunk.GenerateChunk();
      // Save to cache for next time
      cache->saveChunk(newChunk, (int)key.x, (int)key.z);
    }

    this->Chunks[key] = newChunk;
  }
  return std::ref(Chunks[key]);
}

int ChunkManager::BaseHeight(float ValueNoise, int Length,
                             const HeightsDif *Heights) {
  for (int i = 0; i < Length - 1; i++) {
    if (ValueNoise > ContinentelnessHeight[i].x) {
      // REMOVED: std::cout - performance killer!
      return Lerp(Heights[i + 1].y, Heights[i].y,
                  (ValueNoise + 1 / (Heights[i + 1].x + 1)));
    }
  }
  return Heights[Length - 1].y;
}
Biome ChunkManager::GetBiome(float Humudity, float Temperature) {
  Biome TheBiome;

  for (int i = 0; i < 11; i++) {
    bool allowed_humidity =
        (Humudity < Biomes[i].MaxHumidity && Humudity > Biomes[i].MinHumidity);
    bool allowed_temperature = (Temperature < Biomes[i].MaxTemperature &&
                                Temperature > Biomes[i].MinTemperature);

    if (allowed_humidity && allowed_temperature) {
      TheBiome = Biomes[i];
    }
  }

  return TheBiome;
}
int ChunkManager::GetHeight(float Continentalness, float Errotion,
                            float PeakAndValleys) {
  return (int)(BaseHeight(Continentalness, 8, ContinentelnessHeight));
}

/*
namespace ChunckManager {
        static bool isTransparent(int blockID)
        {
                return blockID == 0 || blockID == 5;
        }
        static bool canSwim(int blockID)
        {
                return blockID == 5;
        }
        bool Collition(Vector3& PlayerPos, int FullRange, int yRange, bool Swim,
bool Block)
        {
                int newX = (int)(PlayerPos.x + (int)(FullRange / 2.0f - 1));
                int newY = (int)(PlayerPos.y + (int)(yRange / 2.0f));
                int newZ = (int)(PlayerPos.z + (int)(FullRange / 2.0f - 1));

                Vector3 chunk = { floorf((float)(newX) / 32),
floorf((float)(newZ) / 32) }; int localX = (int)(newX % 32); int localZ =
(int)(newZ % 32);

                //std::cout << "Chunk: " << chunk << ", LocalX: " << localX <<
", FootY: " << footY << ", HeadY: " << headY << ", Block Info: " <<
Chunks[chunk].Blocks[localX][footY] << std::endl;

                bool blockHead = !isTransparent(Chunks[{(int)chunk.x,
(int)chunk.z}].Blocks[{localX, newY, localZ}]);

                if (Swim && !blockHead) {
                        blockHead = canSwim(Chunks[{(int)chunk.x,
(int)chunk.z}].Blocks[{localX, newY, newZ}]);
                }

                if (Block) {
                        blockHead = false;
                }

                return blockHead;
        }
        bool PlaceBlock(int BlockType, Vector3 Position, int yRange, Vector3
PlayerPosition, short& Type)
        {

                Position.x = (int)(Position.x / BlockSize) + PlayerPosition.x;
                Position.y = (yRange - (int)(Position.y / BlockSize) - 1) +
PlayerPosition.y;


                Vector3 CurrrentChunk = { floorf((Position.x) / 32),
floorf((Position.z) / 32) }; int RelativeX = (int)(Position.x) % 32; int
RelativeZ = (int)(Position.z) % 32;


                bool notBedRock =
(Chunks[(int)CurrrentChunk.x][(int)CurrrentChunk.z].Blocks[RelativeX][(int)Position.y][RelativeZ]
!= 4); bool water =
(Chunks[(int)CurrrentChunk.x][(int)CurrrentChunk.z].Blocks[RelativeX][(int)Position.y][RelativeZ]
== 5); bool canPlace = (BlockType != 0 &&
(Chunks[(int)CurrrentChunk.x][(int)CurrrentChunk.z].Blocks[RelativeX][(int)Position.y]
== 0 || water)); bool canBreak = (BlockType == 0 &&
(Chunks[(int)CurrrentChunk.x][(int)CurrrentChunk.z].Blocks[RelativeX][(int)Position.y]
!= 0 && !water));

                //std::cout << canBreak << " " << canPlace << std::endl;

                if (notBedRock && (canPlace || canBreak)) {
                        std::cout << "Placing Block: " << BlockType << ",
Position: " << Position.x << ", " << Position.y << "Chunk: " << CurrrentChunk.x
<< "; " << CurrrentChunk.z << std::endl; Type =
(short)Chunks[(int)CurrrentChunk.x][(int)CurrrentChunk.z].Blocks[RelativeX][(int)Position.y];
                        //Chunks[CurrrentChunk].Blocks[RelativeX][(int)Position.y]
= BlockType; return true;
                }
                return false;
        }
        void Size(int PixelSizeX, int PixelSizeY, int yRange, int FullRange)
        {
                if (PixelSizeX > PixelSizeY)
                {
                        BlockSize = (int)(PixelSizeY / yRange);
                }
                else
                {
                        BlockSize = (int)(PixelSizeX / FullRange);
                }
        }
        void ShowInventor(SDL_Renderer* Renderer, int width, int height,
std::vector<Slot>& Inventory, int InventorySlot, TTF_Font* font)
        {
                SDL_SetRenderDrawColor(Renderer, 157, 76, 0, 255);
                SDL_FRect InventoryRect = { (float)(width / 2) - (BlockSize *
4), (height - (BlockSize * 1.3f)), (BlockSize * 1.1f) * 8 + (BlockSize * .1f),
(BlockSize * 1.2f) }; SDL_RenderFillRect(Renderer, &InventoryRect);

                for (int i = 0; i < 8; i++)
                {
                        if (InventorySlot == i)
                        {
                                SDL_SetRenderDrawColor(Renderer, 255, 153, 56,
255);
                        }
                        else
                        {
                                SDL_SetRenderDrawColor(Renderer, 204, 102, 0,
255);
                        }
                        SDL_FRect InventoryRect = { (float)(((width / 2) -
(BlockSize * 4)) + (BlockSize * 1.1f * i) + (BlockSize * 0.1f)), (float)((height
- (BlockSize * 1.3f)) + (BlockSize * 0.1f)), (float)BlockSize, (float)BlockSize
}; SDL_RenderFillRect(Renderer, &InventoryRect);

                        if (Inventory[i].Type != 0)
                        {
                                SDL_SetRenderDrawColor(Renderer,
BlockDef[Inventory[i].Type].Color.r, BlockDef[Inventory[i].Type].Color.g,
BlockDef[Inventory[i].Type].Color.b, 1); SDL_FRect BlockRect = { ((width / 2) -
(BlockSize * 4)) + (BlockSize * 1.1f * i) + (BlockSize * 0.3f), (height -
(BlockSize * 1.3f)) + (BlockSize * 0.3f), BlockSize * 0.6f, BlockSize * 0.6f };
                                SDL_RenderFillRect(Renderer, &BlockRect);
                                //Add be a number
                                if (Inventory[i].Amount > 1)
                                {
                                        std::string text =
std::to_string(Inventory[i].Amount); SDL_Color White = { 200, 200, 200 };
                                        SDL_Surface* surface =
TTF_RenderText_Blended(font, text.c_str(), sizeof(text.c_str()), White);
                                        SDL_Texture* texture =
SDL_CreateTextureFromSurface(Renderer, surface); SDL_DestroySurface(surface);
                                        SDL_FRect dstRect{ (float)(((width / 2)
- (BlockSize * 4)) + (BlockSize * 1.1f * i) + (BlockSize * 0.8f)),
(float)((height - (BlockSize * 1.1f)) + (BlockSize * 0.4f)), BlockSize * 0.5f ,
BlockSize * 0.5f}; SDL_RenderTexture(Renderer, texture, NULL, &dstRect);
                                        SDL_DestroyTexture(texture);
                                }
                        }

                }
        }
        void SimulateWater(int chunkIndex) {
                std::queue<std::pair<int, int> > q;

                for (int x = 0; x < 16; x++) {
                        for (int y = 0; y < 64; y++) {
                                if (Chunks[chunkIndex].Blocks[x][y] == 5) {
                                        int globalX = chunkIndex * 16 + x;
                                        q.push(std::make_pair(globalX, y));
                                }
                        }
                }

                while (!q.empty()) {
                        std::pair<int, int> pos = q.front();
                        q.pop();

                        int x = pos.first;
                        int y = pos.second;

                        int chunk = x / 16;
                        int localX = x % 16;

                        // Move down
                        if (y - 1 >= 0 && (Chunks[chunk].Blocks[localX][y - 1]
== 0 || Chunks[chunk].Blocks[localX][y - 1] == 5)) {
                                Chunks[chunk].Blocks[localX][y - 1] = 5;
                                q.push(std::make_pair(x, y - 1));
                        }
                        else {
                                // Try left
                                if (x - 1 >= 0) {
                                        int leftChunk = (x - 1) / 16;
                                        int leftX = (x - 1) % 16;
                                        if (Chunks[leftChunk].Blocks[leftX][y]
== 0) { Chunks[leftChunk].Blocks[leftX][y] = 5; q.push(std::make_pair(x - 1,
y));
                                        }
                                }
                                // Try right
                                if (x + 1 < 1600) {
                                        int rightChunk = (x + 1) / 16;
                                        int rightX = (x + 1) % 16;
                                        if (Chunks[rightChunk].Blocks[rightX][y]
== 0) { Chunks[rightChunk].Blocks[rightX][y] = 5; q.push(std::make_pair(x + 1,
y));
                                        }
                                }
                        }
                }
        }
}
*/