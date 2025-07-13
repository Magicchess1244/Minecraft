#ifndef	__CHUNKMANAGER_HPP__
#define __CHUNKMANAGER_HPP__

#include <SDL3_ttf/SDL_ttf.h>
#include <map>
#include <SDL3/SDL.h>
#include "PerlinNoise.h"
#include <vector>

typedef struct
{
	std::vector<SDL_Vertex> Vertices;
	std::vector<int> Indices;
	int faces;

} Mesh;
typedef struct  {
	short Amount;
	short Type;
} Slot;
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
	void ChunkGenerator(Vector3 Chunk);
	void Face(Mesh& mesh, Vector3 position, Vector3 rotation, Vector3 blocks, int color, Vector3 ScreenSize, int Side);
	void RenderChunk(Vector3 cameraPos, Vector3 cameraRot, Vector3 screenSize, Mesh& mesh);
	void PrintChunk(int i, int xPlayerPos, int yPlayerPos, int xRange, int yRange, int FullRange);
	SDL_FPoint getUV(int tileIndex, int cornerX, int cornerY);
	bool Collition(Vector3& PlayerPos, int FullRange, int yRange, bool Swim, bool Block);
	bool PlaceBlock(int BlockType, Vector3 Position, int yRange, Vector3 PlayerPosition, short& Type);
	void Size(int PixelSizeX, int PixelSizeY, int yRange, int FullRange);
	void ShowInventor(SDL_Renderer* Renderer, int width, int height, std::vector<Slot>& Inventory, int InventorySlot, TTF_Font* font);
	void SimulateWater(int chunkIndex);
};
#endif