#ifndef	__CHUNKMANAGER_HPP__
#define __CHUNKMANAGER_HPP__

#include "common.hpp"

typedef struct {
	char Name[20];
	short BlockId;
	SDL_FColor Color;
	short SpawningLayer[2];
	bool Top;
	bool Water;
} Block;

constexpr int BlockNum = 6;
extern Block BlockDef[BlockNum];
extern int BlockSize;

namespace ChunckManager {
	void Normalize(Vector3& vector);
	void ChunkGenerator(Vector3 Chunk);
	bool Collition(Vector3& PlayerPos, int FullRange, int yRange, bool Swim, bool Block);
	bool PlaceBlock(int BlockType, Vector3 Position, int yRange, Vector3 PlayerPosition, short& Type);
	void Size(int PixelSizeX, int PixelSizeY, int yRange, int FullRange);
	void ShowInventor(SDL_Renderer* Renderer, int width, int height, std::vector<Slot>& Inventory, int InventorySlot, TTF_Font* font);
	void SimulateWater(int chunkIndex);
};
#endif