#include "Chunck.h"
#include "PerlinNoise.h"
#include "BiomeBuilder.h"
#include "ChunkManager.h"

void ChunckPrefab::ShowChunk()
{
	for (int y = ySize - 1; y > 0; y--) {
		for (int x = 0; x < xSize; x++) {
			if (Blocks[x][y] == 1) {
				std::cout << "O";
			}
			else {
				std::cout << " ";
			}
		}
		std::cout << std::endl;
	}
}
void ChunckPrefab::GenerateChunk()
{
	GenerateChunkSurface();
	GenerateChunkCaves();
}
void ChunckPrefab::GenerateChunkSurface()
{
	for (int x = 0; x < this->xSize; x++) {
		int Height = (int)(35 + (PerlinNoise(xPos + x, 0, 4, 0, 0.1f) * 25));
		int ActualHeight = Height;

		if (Height < 33) {
			Height = 33;
		}
		for (int y = Height; y > -1; y--) {
			if (y <= ActualHeight)
			{
				for (int i = 0; i < BlockNum; i++)
				{
					if (BlockDef[i].Top && Height - y >= BlockDef[i].SpawningLayer[0] && Height - y <= BlockDef[i].SpawningLayer[1]) {
						Blocks[x][y] = BlockDef[i].BlockId;
						break;
					}
					else if (!BlockDef[i].Top && y >= BlockDef[i].SpawningLayer[0] && y <= BlockDef[i].SpawningLayer[1])
					{
						Blocks[x][y] = BlockDef[i].BlockId;
						break;
					}
				}
			} else {
				Blocks[x][y] = 5;
			}
		}
	}
}
void ChunckPrefab::GenerateChunkCaves()
{
	for (int x = 0; x < this->xSize; x++) {
		for (int y = 2; y < this->ySize; y++)
		{
			float Hole = PerlinNoise(xPos + x, y, 3, 3, 0.1f);

			bool CheeseCave = Hole <= -0.9f || Hole >= 0.9f;
			bool NodleCave = (0.04f > Hole && Hole > -0.04f);

			if ((CheeseCave || NodleCave) && Blocks[x][y] != 4) {
				Blocks[x][y] = 0;
			}
		}
	}
}