#include "Chunck.h"

const Vector3 Direction[6] = {
	{ 0, 0, -1 }, // Front
	{ 0, 0, 1 },  // Back
	{ 1, 0, 0 },  // Right
	{ -1, 0, 0 }, // Left
	{ 0, 1, 0 },  // Top
	{ 0, -1, 0 }   // Bottom
};

void ChunkPrefab::GenerateChunk()
{
	this->GenerateChunkSurface();
	//this->GenerateChunkCaves();
	this->VisableFaces();
}

void ChunkPrefab::GenerateChunkSurface()
{
	std::cout << "GenerateChunkSurface()" << std::endl;
	/*
	for (int x = 0;x < this->xSize; x++) {
		for (int y = 0; y < this->ySize; y++) {
			for (int z = 0; z < this->zSize; z++) {
				Blocks[{x, y, z}] = 3;
			}
		}
	}
	*/
	for (int x = 0; x < this->xSize; x++) {
		for (int z = 0; z < this->zSize; z++) {
			int Height = (int)(35 + (
				PerlinNoise({ (double)xPos + x, 0, (double)zPos + z }, 4, 0.1f)
				* 25));
			int ActualHeight = Height;
	
			if (Height < 35) {
				Height = 35;
			}

			for (int y = Height; y > -1; y--) {
				Vector3 BlockPos = { (double)x, (double)y, (double)z };
				if (y <= ActualHeight) {
					Blocks[BlockPos] = 3;
					/*
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
					*/
				}
				else {
					Blocks[BlockPos] = 5;
				}
			}
		}
	}

	return;
}

void ChunkPrefab::GenerateChunkCaves()
{

	for (int x = 0; x < this->xSize; x++) {
		for (int z = 0; z < this->xSize; z++) {
			for (int y = 2; y < this->ySize; y++)
			{
				Vector3 BlockPos = { (double)x, (double)y, (double)z };
				auto it = Blocks.find(BlockPos);
				if (it == Blocks.end()) continue;
				int blockID = it->second;

				double Hole = PerlinNoise({ (double)xPos + x, (double)y , (double) z}, 3, 0.1f);

				bool CheeseCave = Hole <= -0.9f || Hole >= 0.9f;
				bool NodleCave = (0.04f > Hole && Hole > -0.04f);

				if ((CheeseCave || NodleCave) && (blockID != 4 && blockID != 5)) {
					this->Blocks.erase(BlockPos);
				}
			}
		}
	}
}
void ChunkPrefab::VisableFaces() {

	for (int y = 0; y < ySize; y++) {
		for (int x = 0; x < xSize; x++) {
			for (int z = 0; z < zSize; z++) {
				Vector3 blockPos = { (double)x, (double)y, (double)z };

				auto it = Blocks.find(blockPos);
				if (it == Blocks.end()) continue;
				int blockID = it->second;


				for (int i = 0; i < 6; i++)
				{
					Vector3 NextBlockPos = blockPos + Direction[i];
					auto blockIt = Blocks.find({NextBlockPos.x, NextBlockPos.y, NextBlockPos.z });
					if ((blockIt == Blocks.end() || (blockID == 5 && blockIt->second != 5))) {
						Vector3 ChunkPos = { (double)xPos, 0, (double)zPos };
						Vector3 world = blockPos + ChunkPos;
						allFaces.push_back({ world, i, blockID, 0 });
					}
				}

			}
		}
	}
}