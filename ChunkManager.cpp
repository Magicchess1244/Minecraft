#include "ChunkManager.h"
#include "Chunck.h"
#include <SDL3/SDL_render.h>
#include <iostream>
#include <sstream>
#include <map>
#include <queue>
#include <utility>
#include <vector>

#pragma comment(lib, "SDL3_ttf.lib")

ChunckPrefab Chunks[10][10]{};
Block BlockDef[BlockNum] = {
	{"Air", 0, { 0, 0.7f, 1, 1 }, NULL, NULL},
	{"Grass", 1, { 0, 0.65f, 0, 1 }, {0,0}, true, false},
	{"Dirt", 2, { 0.6f, 0.3f, 0.1f, 255 }, {1,3}, true, false},
	{"Stone", 3, { 0.5f, 0.5f, 0.5f, 1 }, {4, 64}, false, false},
	{"Bedrock", 4, { 0.2f, 0.2f, 0.2f, 1 }, {0, 3}, false, false},
	{"Water", 5, { 0, 0.4f, 0.8f, 1 }, NULL, false, true}
};

int BlockSize = 50;

namespace ChunckManager {
	bool isTransparent(int blockID)
	{
		return blockID == 0 || blockID == 5;
	}
	bool canSwim(int blockID)
	{
		return blockID == 5;
	}
	void PrintChunk(int i, int xPlayerPos, int yPlayerPos, int xRange, int yRange, int FullRange)
	{
		/*
		int xSize = Chunks[i].xSize;
		int ySize = Chunks[i].ySize;
		int xPos = Chunks[i].xPos;

		for (int y = 0; y < yRange; y++) {
			for (int x = 0; x < xRange; x++) {
				std::cout << Chunks[i].Blocks[xPlayerPos + x][yPlayerPos + y];
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
		std::cout << std::endl;
		std::cout << std::endl;
		*/
	}
	SDL_FPoint getUV(int tileIndex, int cornerX, int cornerY) {
		const int tileSize = 16;
		const int atlasSize = 64;
		const float pixelNudge = 0.25f; // or 0.25f if needed

		int tilesPerRow = atlasSize / tileSize;
		int tileX = tileIndex % tilesPerRow;
		int tileY = tileIndex / tilesPerRow;

		float u = (tileX * tileSize + cornerX * (tileSize - pixelNudge * 2) + pixelNudge) / (float)atlasSize;
		float v = (tileY * tileSize + cornerY * (tileSize - pixelNudge * 2) + pixelNudge) / (float)atlasSize;

		return { u, v };
	}
	void RenderChunk(Vector3 cameraPos, Mesh& mesh, int& faces) {
		int chunkX = (int)cameraPos.x;
		int chunkZ = (int)cameraPos.z;

		ChunckPrefab& chunk = Chunks[chunkX][chunkZ];
		const float fov = 1.0f; //tanf((45.0f / 180.0f) * 3.14159f / 2.0f);
		int quadIndex = 0;

		for (int y = 0; y < Chunks[chunkX][chunkZ].ySize; y++) {
			for (int x = 0; x < Chunks[chunkX][chunkZ].xSize; x++) {
				for (int z = 1; z < 2; z++) {
					int blockID = Chunks[chunkX][chunkZ].Blocks[x][y][z];
					if (blockID == 0) continue;

					SDL_FColor color = { 1, 1, 1, 1 }; // Use BlockDef[blockID].Color if available

					float FlipX = (x + chunk.xPos - cameraPos.x) * BlockSize;
					float FlipY = (chunk.ySize - y - 1 - cameraPos.y) * BlockSize;
					float FlipZ = (z + chunk.zPos - cameraPos.z) * BlockSize;

					if (FlipZ == 0) FlipZ = 0.0001f;

					int vIndex = quadIndex * 4;
					int iIndex = quadIndex * 6;

					mesh.Vertices[vIndex + 0] = { {FlipX / (FlipZ * fov), FlipY / (FlipZ * fov)}, color };//, getUV(blockID, 0, 0) };
					mesh.Vertices[vIndex + 1] = { {(FlipX + BlockSize) / (FlipZ * fov), FlipY / (FlipZ * fov)}, color };//, getUV(blockID, 1, 0) };
					mesh.Vertices[vIndex + 2] = { {FlipX / (FlipZ * fov), (FlipY + BlockSize) / (FlipZ * fov)}, color };//, getUV(blockID, 0, 1) };
					mesh.Vertices[vIndex + 3] = { {(FlipX + BlockSize) / (FlipZ * fov), (FlipY + BlockSize) / (FlipZ * fov)}, color }; //, getUV(blockID, 1, 1) };

					mesh.Indices[iIndex + 0] = vIndex + 0;
					mesh.Indices[iIndex + 1] = vIndex + 1;
					mesh.Indices[iIndex + 2] = vIndex + 2;
					mesh.Indices[iIndex + 3] = vIndex + 2;
					mesh.Indices[iIndex + 4] = vIndex + 1;
					mesh.Indices[iIndex + 5] = vIndex + 3;

					quadIndex++;
					faces++;
				}
			}
		}
	}

	bool Collition(Vector3& PlayerPos, int FullRange, int yRange, bool Swim, bool Block)
	{

		int newX = (int)(PlayerPos.x + (int)(FullRange / 2.0f - 1));
		int newY = (int)(PlayerPos.y + (int)(yRange / 2.0f));
		int newZ = (int)(PlayerPos.z + (int)(FullRange / 2.0f - 1));

		Vector3 chunk = { (int)(newX) / 32, (int)(newZ) / 32 };
		int localX = (int)(newX % 32);
		int localZ = (int)(newZ % 32);

		//std::cout << "Chunk: " << chunk << ", LocalX: " << localX << ", FootY: " << footY << ", HeadY: " << headY << ", Block Info: " << Chunks[chunk].Blocks[localX][footY] << std::endl;

		bool blockHead = !isTransparent(Chunks[(int)chunk.x][(int)chunk.z].Blocks[localX][newY][localZ]);

		if (Swim && !blockHead) {
			blockHead = canSwim(Chunks[(int)chunk.x][(int)chunk.z].Blocks[localX][newY][newZ]);
		}

		if (Block) {
			blockHead = false;
		}

		return blockHead;
	}
	bool PlaceBlock(int BlockType, Vector3 Position, int yRange, Vector3 PlayerPosition, short& Type)
	{
		Position.x = (int)(Position.x / BlockSize) + PlayerPosition.x;
		Position.y = (yRange - (int)(Position.y / BlockSize) - 1) + PlayerPosition.y;


		Vector3 CurrrentChunk = { floorf((Position.x) / 32), floorf((Position.z) / 32) };
		int RelativeX = (int)(Position.x) % 32;
		int RelativeZ = (int)(Position.z) % 32;


		bool notBedRock = (Chunks[(int)CurrrentChunk.x][(int)CurrrentChunk.z].Blocks[RelativeX][(int)Position.y][RelativeZ] != 4);
		bool water = (Chunks[(int)CurrrentChunk.x][(int)CurrrentChunk.z].Blocks[RelativeX][(int)Position.y][RelativeZ] == 5);
		bool canPlace = (BlockType != 0 && (Chunks[(int)CurrrentChunk.x][(int)CurrrentChunk.z].Blocks[RelativeX][(int)Position.y] == 0 || water));
		bool canBreak = (BlockType == 0 && (Chunks[(int)CurrrentChunk.x][(int)CurrrentChunk.z].Blocks[RelativeX][(int)Position.y] != 0 && !water));

		//std::cout << canBreak << " " << canPlace << std::endl;

		if (notBedRock && (canPlace || canBreak)) {
			std::cout << "Placing Block: " << BlockType << ", Position: " << Position.x << ", " << Position.y << "Chunk: " << CurrrentChunk.x << "; " << CurrrentChunk.z << std::endl;
			Type = (short)Chunks[(int)CurrrentChunk.x][(int)CurrrentChunk.z].Blocks[RelativeX][(int)Position.y];
			//Chunks[CurrrentChunk].Blocks[RelativeX][(int)Position.y] = BlockType;
			return true;
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
	void ShowInventor(SDL_Renderer* Renderer, int width, int height, std::vector<Slot>& Inventory, int InventorySlot, TTF_Font* font)
	{
		SDL_SetRenderDrawColor(Renderer, 157, 76, 0, 255);
		SDL_FRect InventoryRect = { (float)(width / 2) - (BlockSize * 4), (height - (BlockSize * 1.3f)), (BlockSize * 1.1f) * 8 + (BlockSize * .1f), (BlockSize * 1.2f) };
		SDL_RenderFillRect(Renderer, &InventoryRect);

		for (int i = 0; i < 8; i++)
		{
			if (InventorySlot == i)
			{
				SDL_SetRenderDrawColor(Renderer, 255, 153, 56, 255);
			}
			else
			{
				SDL_SetRenderDrawColor(Renderer, 204, 102, 0, 255);
			}
			SDL_FRect InventoryRect = { (float)(((width / 2) - (BlockSize * 4)) + (BlockSize * 1.1f * i) + (BlockSize * 0.1f)), (float)((height - (BlockSize * 1.3f)) + (BlockSize * 0.1f)), (float)BlockSize, (float)BlockSize };
			SDL_RenderFillRect(Renderer, &InventoryRect);

			if (Inventory[i].Type != 0)
			{
				SDL_SetRenderDrawColor(Renderer, (Uint8)(BlockDef[Inventory[i].Type].Color.r * 255), (Uint8)(BlockDef[Inventory[i].Type].Color.g * 255), (Uint8)(BlockDef[Inventory[i].Type].Color.b * 255), (Uint8)(BlockDef[Inventory[i].Type].Color.a * 255));
				SDL_FRect BlockRect = { ((width / 2) - (BlockSize * 4)) + (BlockSize * 1.1f * i) + (BlockSize * 0.3f), (height - (BlockSize * 1.3f)) + (BlockSize * 0.3f), BlockSize * 0.6f, BlockSize * 0.6f };
				SDL_RenderFillRect(Renderer, &BlockRect);
				//Add be a number
				if (Inventory[i].Amount > 1)
				{
					std::string text = std::to_string(Inventory[i].Amount);
					SDL_Color White = { 200, 200, 200 };
					SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), sizeof(text.c_str()), White);
					SDL_Texture* texture = SDL_CreateTextureFromSurface(Renderer, surface);
					SDL_DestroySurface(surface);
					SDL_FRect dstRect{ (float)(((width / 2) - (BlockSize * 4)) + (BlockSize * 1.1f * i) + (BlockSize * 0.8f)), (float)((height - (BlockSize * 1.1f)) + (BlockSize * 0.4f)), BlockSize * 0.5f , BlockSize * 0.5f};
					SDL_RenderTexture(Renderer, texture, NULL, &dstRect);
					SDL_DestroyTexture(texture);
				}
			}

		}
	}
	void SimulateWater(int chunkIndex) {
		/*
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
			if (y - 1 >= 0 && (Chunks[chunk].Blocks[localX][y - 1] == 0 || Chunks[chunk].Blocks[localX][y - 1] == 5)) {
				Chunks[chunk].Blocks[localX][y - 1] = 5;
				q.push(std::make_pair(x, y - 1));
			}
			else {
				// Try left
				if (x - 1 >= 0) {
					int leftChunk = (x - 1) / 16;
					int leftX = (x - 1) % 16;
					if (Chunks[leftChunk].Blocks[leftX][y] == 0) {
						Chunks[leftChunk].Blocks[leftX][y] = 5;
						q.push(std::make_pair(x - 1, y));
					}
				}
				// Try right
				if (x + 1 < 1600) {
					int rightChunk = (x + 1) / 16;
					int rightX = (x + 1) % 16;
					if (Chunks[rightChunk].Blocks[rightX][y] == 0) {
						Chunks[rightChunk].Blocks[rightX][y] = 5;
						q.push(std::make_pair(x + 1, y));
					}
				}
			}
		}
		*/
	}

	void ChunkGenerator(Vector3 Chunk)
	{
		if (Chunks[(int)Chunk.x][(int)Chunk.z].xPos == -1 ) {
			Chunks[(int)Chunk.x][(int)Chunk.z].xPos = Chunk.x * Chunks[(int)Chunk.x][(int)Chunk.z].xSize;
			Chunks[(int)Chunk.x][(int)Chunk.z].zPos = Chunk.z * Chunks[(int)Chunk.x][(int)Chunk.z].xSize;
			Chunks[(int)Chunk.x][(int)Chunk.z].GenerateChunk();
			//SimulateWater(Chunk);
			return;
		}
	}
}