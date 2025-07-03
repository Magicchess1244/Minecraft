#ifndef __Chunck__
#define __Chunck__
#include "BiomeBuilder.h"
#include <iostream>

class ChunckPrefab {
public:
	const int xSize = 32;
	const int ySize = 64;
	int xPos = -1;
	int zPos = -1;
	int Height;
	Biome biome;

	int Blocks[32][64][32] = {0};
	void ShowChunk();
	void GenerateChunk();
private:
	void GenerateChunkSurface();
	void GenerateChunkCaves();
};

#endif