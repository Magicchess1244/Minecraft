#ifndef	__CHUNKMANAGER_HPP__
#define __CHUNKMANAGER_HPP__

#include <SDL3_ttf/SDL_ttf.h>
#include <map>
#include <SDL3/SDL.h>
#include "PerlinNoise.h"
#include <vector>
#define BlockNum 6 

typedef struct
{
	SDL_Vertex Vertices[512 * 4];
	int Indices[512 * 6];
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

extern Block BlockDef[BlockNum];
extern int BlockSize;
namespace ChunckManager {
	void ChunkGenerator(int Chunk);
	void DrawChunk(int i, int xPlayerPos, int yPlayerPos, int xRange, int yRange, int FullRange, Mesh& mesh, bool FirstChunck);
	void PrintChunk(int i, int xPlayerPos, int yPlayerPos, int xRange, int yRange, int FullRange);
	SDL_FPoint getUV(int tileIndex, int cornerX, int cornerY);
	bool Collition(Vector3& PlayerPos, Vector3 Direction, int FullRange, int yRange, bool Swim, bool Block);
	bool PlaceBlock(int BlockType, Vector3 Position, int yRange, Vector3 PlayerPosition, short& Type);
	void Size(int PixelSizeX, int PixelSizeY, int yRange, int FullRange);
	int GetHeight(int xPos);
	void ShowInventor(SDL_Renderer* Renderer, int width, int height, std::vector<Slot>& Inventory, int InventorySlot, TTF_Font* font);
	void SimulateWater(int chunkIndex);
};
#endif