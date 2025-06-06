#ifndef __Chunck__
#define __Chunck__
#include "BiomeBuilder.h"
#include <iostream>

class ChunckPrefab {
public:
	const int xSize = 16;
	const int ySize = 64;
	int xPos = -1;
	int Height;
	Biome biome;

	int Blocks[16][64] = { 0 };
	void ShowChunk();
	void GenerateChunk();
private:
	void GenerateChunkSurface();
	void GenerateChunkCaves();
};

#endif