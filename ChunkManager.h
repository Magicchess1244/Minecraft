#ifndef	__CHUNKMANAGER_HPP__
#define __CHUNKMANAGER_HPP__

<<<<<<< HEAD
#include "common.hpp"
#include "Chunck.h"
=======
#include <map>
#include <SDL3/SDL.h>
#include "PerlinNoise.h"
#define BlockNum 6 
>>>>>>> parent of a48f4a5 (Fic¡x tcp but creating one extra client)

typedef struct {
	char Name[20];
	short BlockId;
	Color Color;
	short SpawningLayer[2];
	bool Top;
	bool Water;
} Block;
typedef struct {
	int MaxHumidity;
	int MaxTemperature;
	int MinHumidity;
	int MinTemperature;
	int BaseHeight;
	int ChangeAmount;
} Biome;
typedef struct {
	double x;
	double y;
} HeightsDif;

constexpr int BlockNum = 6;
const Block BlockDef[BlockNum] = {
		{"Air",      0, {255, 178, 255},       NULL,             false, false},
		{"Grass",    1, {  0, 166,   0},       {0, 0},              true,  false},
		{"Dirt",     2, {153,  76,  25},       {1, 3},              true,  false},
		{"Stone",    3, {128, 128, 128},       {4, 64},             false, false},
		{"Bedrock",  4, { 51,  51,  51},       {0, 3},              false, false},
		{"Water",    5, {  0, 102, 204},       NULL,             false, true }
};

<<<<<<< HEAD
class ChunkManager {
private:
	std::unordered_map<Vector3, ChunkPrefab> Chunks;
public:
	ChunkManager() = default;
	~ChunkManager() = default;

	ChunkPrefab& get_chunk(Vector3 key);
	int BaseHeight(double ValueNoise, int Length, const HeightsDif* Heights);
	Biome GetBiome(double Humudity, double Temperature);
	int GetHeight(double Continentalness, double Errotion, double PeakAndVallies);
	Block GetBlock(int BlockId) {
		if (BlockId < 0 || BlockId >= BlockNum) {
			std::cerr << "Invalid Block ID: " << BlockId << std::endl;
			return BlockDef[0];
		}
		return BlockDef[BlockId];
	}
};
/*
namespace ChunckManager {
	bool Collition(Vector3& PlayerPos, int FullRange, int yRange, bool Swim, bool Block);
	bool PlaceBlock(int BlockType, Vector3 Position, int yRange, Vector3 PlayerPosition, short& Type);
	void Size(int PixelSizeX, int PixelSizeY, int yRange, int FullRange);
	void ShowInventor(SDL_Renderer* Renderer, int width, int height, std::vector<Slot>& Inventory, int InventorySlot, TTF_Font* font);
	void SimulateWater(int chunkIndex);
};
*/
=======
void ChunkGenerator(int Chunk);
void DrawChunk(int i, int xPlayerPos, int yPlayerPos, int xRange, int yRange, int FullRange, Mesh* mesh, bool FirstChunck);
void PrintChunk(int i, int xPlayerPos, int yPlayerPos, int xRange, int yRange, int FullRange);
bool Collition(Vector2* PlayerPos, Vector2 Direction, int FullRange, int yRange, bool Swim, bool Block);
bool PlaceBlock(int BlockType, Vector2 Position, int yRange, Vector2 PlayerPosition, short* Type);
void Size(int PixelSizeX, int PixelSizeY, int yRange, int FullRange);
int GetHeight(int xPos);
void ShowInventor(SDL_Renderer* Renderer, int width, int height, Slot* Inventory, int InventorySlot);
void SimulateWater(int chunkIndex);
>>>>>>> parent of a48f4a5 (Fic¡x tcp but creating one extra client)
#endif