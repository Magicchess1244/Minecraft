#include "Chunck.h"
#include "PerlinNoise.h"
#include "BiomeBuilder.h"
#include "ChunkManager.h"

void ChunkPrefab::ShowChunk()
{
	for (int y = ySize - 1; y > 0; y--) {
		for (int x = 0; x < xSize; x++) {
			if (this->Blocks[{x, y, 0}] == 1) {
				std::cout << "O";
			}
			else {
				std::cout << " ";
			}
		}
		std::cout << std::endl;
	}
}

void ChunkPrefab::GenerateChunk()
{
	this->GenerateChunkSurface();
	//this->GenerateChunkCaves();
}

void ChunkPrefab::GenerateChunkSurface()
{
	//std::cout << "here" << std::endl;

	for (int x = 0;x < this->xSize; x++) {
		for (int y = 0; y < this->ySize; y++) {
			for (int z = 0; z < this->zSize; z++) {
				Blocks[{x, y, z}] = 3;
			}
		}
	}
	/*
	for (int x = 0; x < this->xSize; x++) {
		for (int z = 0; z < this->zSize; z++) {
			int Height = (int)(35 + (PerlinNoise({ (float)xPos + x, 0, (float)zPos + z }, 4, 0.1f) * 25));
			int ActualHeight = Height;
			std::cout << "Height: " << Height << std::endl;

			if (Height < 35) {
				Height = 35;
			}

			for (int y = this->ySize; y > -1; y--) {
				if (y <= ActualHeight + 100)
				{
					Blocks[{x,y,z}] = 3;
					
					for (int i = 0; i < BlockNum; i++)
					{
						if (BlockDef[i].Top && Height - y >= BlockDef[i].SpawningLayer[0] && Height - y <= BlockDef[i].SpawningLayer[1]) {
							Blocks[{x, y, z}] = BlockDef[i].BlockId;
							break;
						}
						else if (!BlockDef[i].Top && y >= BlockDef[i].SpawningLayer[0] && y <= BlockDef[i].SpawningLayer[1])
						{
							Blocks[{x, y, z}] = BlockDef[i].BlockId;
							break;
						}
					}
					
				}
				else {
					Blocks[{x, y, z}] = 5;
				}
			}
		}
	}
	*/

	return;
}
void ChunkPrefab::GenerateChunkCaves()
{
	for (int x = 0; x < this->xSize; x++) {
		for (int z = 0; z < this->xSize; z++) {
			for (int y = 2; y < this->ySize; y++)
			{
				float Hole = PerlinNoise({ (float)xPos + x, (float)y , (float) z}, 3, 0.1f);

				bool CheeseCave = Hole <= -0.9f || Hole >= 0.9f;
				bool NodleCave = (0.04f > Hole && Hole > -0.04f);

				if ((CheeseCave || NodleCave) && (this->Blocks[{x,y,z}] != 4 && this->Blocks[{x, y, z}] != 5)) {
					this->Blocks[{x, y, z}] = 0;
				}
			}
		}
	}
}