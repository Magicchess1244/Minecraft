#include "ChunkManager.h"
#include "Chunck.h"
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <sstream>
#include <map>
#include <queue>
#include <utility>
#include <vector>
#include <algorithm>
#include <tuple>
#include <unordered_map>

//#pragma comment(lib, "SDL3_ttf.lib")

constexpr float PI = 3.1415926535;
#define AngleToRadians(angle) ((angle) * (PI / 180.0f))
#define ADDVECTOR(V1, V2) {V1.x + V2.x, V1.y + V2.y, V1.z + V2.z} 
#define SUBSVECTOR(V1, V2) {V1.x - V2.x, V1.y - V2.y, V1.z - V2.z} 
#define BLOCKPOS(V1, V2) { \
    fmodf(fmodf(V1.x + V2.x, 32.0f) + 32.0f, 32.0f), \
    V1.y + V2.y, \
    fmodf(fmodf(V1.z + V2.z, 32.0f) + 32.0f, 32.0f) \
}
#define ADDCOLOR(C1, C2) {SDL_clamp(C1.r + C2.r, 0, 1), SDL_clamp(C1.g + C2.g, 0, 1), SDL_clamp(C1.b + C2.b, 0, 1), SDL_clamp(C1.a + C2.a, 0, 1)}

struct FaceToDraw {
	int index;
	float avgZ;
};
struct DrawnFace {
	Vector3 blockPos;
	int side;
	int blockID;
	float avgZ;
};

namespace std {
	template<>
	struct hash<std::tuple<int, int>> {
		size_t operator()(const std::tuple<int, int>& t) const noexcept {
			auto h1 = std::hash<int>{}(std::get<0>(t));
			auto h2 = std::hash<int>{}(std::get<1>(t));
			return h1 ^ (h2 << 1); // simple and safer
		}
	};
}

constexpr float FOV = 0.8f;//(float)tanf((45.0f / 2.0f) / (180.0f * 3.14159f));
constexpr Vector3 Verts[6][4] = {
	{//Front
		{ 0, 0, 0 },
		{1, 0, 0},
		{0, 1, 0},
		{1, 1, 0} },
	{//Back
		{ 1, 0, 1 },
		{0, 0, 1},
		{1, 1, 1},
		{0, 1, 1} },
	{//Right
		{1, 0, 0},
		{1, 0, 1},
		{1, 1, 0},
		{1, 1, 1} },
	{//Left
		{0, 0, 1},
		{0, 0, 0},
		{0, 1, 1},
		{0, 1, 0} },
	{//Top
		{0, 1, 0},
		{1, 1, 0},
		{0, 1, 1},
		{1, 1, 1} },
	{//Bottom
		{0, 0, 0},
		{1, 0, 0},
		{0, 0, 1},
		{1, 0, 1} 
	}
};
constexpr Vector3 Direction[6] = {
	{ 0, 0, -1 }, // Front
	{ 0, 0, 1 },  // Back
	{ 1, 0, 0 },  // Right
	{ -1, 0, 0 }, // Left
	{ 0, 1, 0 },  // Top
	{ 0, -1, 0 }   // Bottom
};
constexpr SDL_FColor Colors[3] = {
	{0,0,0,1}, // Front / Back
	{0.05f,0.05f,0.05f,1}, // Right / Left
	{0.1f,0.1f,0.1f,1}, // Top / Bottom
};

std::unordered_map<std::tuple<int, int>, ChunkPrefab> Chunks;
Block BlockDef[BlockNum] = {
	{"Air", 0, { 1, 0.7f, 1, 1 }, NULL, NULL},
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
	Vector3 rotate(Vector3 p, Vector3 r) {
		float cx = cosf((float)AngleToRadians(r.x)), sx = sinf((float)AngleToRadians(r.x)); // pitch
		float cy = cosf((float)AngleToRadians(r.y)), sy = sinf((float)AngleToRadians(r.y)); // yaw
		float cz = cosf((float)AngleToRadians(r.z)), sz = sinf((float)AngleToRadians(r.z)); // roll
		/*
		// First rotate around Z (roll)
		Vector3 p1 = {
			p.x * cz - p.y * sz,
			p.x * sz + p.y * cz,
			p.z
		};

		// Then X (pitch)
		Vector3 p2 = {
			p1.x,
			p1.y * cx - p1.z * sx,
			p1.y * sx + p1.z * cx
		};

		// Then Y (yaw)
		Vector3 result = {
		  	p2.x * cy + p2.z * sy,
			p2.y,
			-p2.x * sy + p2.z * cy
		};
		*/
		Vector3 result = { 
			p.x * (cy * cx) + p.y * (sy * cx) - p.z * sx,
			p.x * (cy * sx * sz - sy * cz) + p.y * (sy * sx * sz + cy * cz) + p.z * (cx * cz),
			p.x * (cy * sx * cz + sy * sz) + p.y * (sy * sx * cz - cy * sz) + p.z * (cx * cz)
		};
		return result;
	}
	void Face(Mesh& mesh, Vector3 position, Vector3 rotation, Vector3 blocks, int color, Vector3 ScreenSize, int Side)
	{
		const Vector3* verts = Verts[Side];
		
		//std::cout << "\n \n";
		for (int i = 0; i < 4; i++) {
			Vector3 worldPos = {
				(verts[i].x + blocks.x) * BlockSize,
				(verts[i].y + blocks.y) * BlockSize,
				(verts[i].z + blocks.z) * BlockSize
			};

			// Convert to camera-relative space BEFORE rotating
			Vector3 relToCamera = {
				worldPos.x - position.x * BlockSize,
				worldPos.y - position.y * BlockSize,
				worldPos.z - position.z * BlockSize
			};

			// Rotate
			Vector3 localPos = rotate(relToCamera, rotation);

			//std::cout << " 3D:\t Px: " << Px << " Py: " << Py << " Pz: " << Pz << std::endl;

			float screenX = localPos.x;
			float screenY = localPos.y;

			if (localPos.z >= 0.01f) {
				screenX = (localPos.x / (localPos.z * FOV));
				screenY = (localPos.y / (localPos.z * FOV));
			}
			screenX += ScreenSize.x / 2.0f;
			screenY += ScreenSize.y / 2.0f;
			screenY = ScreenSize.y - screenY;

			//std::cout << " 2D:\t Px: " << screenX << " Py: " << screenY << std::endl;

			mesh.Vertices.push_back({ { screenX, screenY }, ADDCOLOR(BlockDef[color].Color, Colors[(int)(Side / 2)]) });
		}
		int vIndex = mesh.faces * 4;

		mesh.Indices.push_back(vIndex + 0);
		mesh.Indices.push_back(vIndex + 1);
		mesh.Indices.push_back(vIndex + 2);

		mesh.Indices.push_back(vIndex + 2);
		mesh.Indices.push_back(vIndex + 1);
		mesh.Indices.push_back(vIndex + 3);

		mesh.faces++;
	}
	void RenderChunk(Vector3 cameraPos, Vector3 cameraRot, Vector3 screenSize, Mesh& mesh) {
		int chunkX = (int)floorf((cameraPos.x) / 32);
		int chunkZ = (int)floorf((cameraPos.z) / 32);

		auto chunkIt = Chunks.find({ chunkX, chunkZ });
		if (chunkIt == Chunks.end()) return;
		ChunkPrefab& chunk = chunkIt->second;

		std::vector<DrawnFace> allFaces;

		for (int y = 0; y < chunk.ySize; y++) {
			for (int x = 0; x < chunk.xSize; x++) {
				for (int z = 0; z < chunk.zSize; z++) {
					auto it = chunk.Blocks.find({ x, y, z });
					if (it == chunk.Blocks.end()) continue;
					int blockID = it->second;

					if (blockID == 0) continue;

					Vector3 blockPos = { (float)x, (float)y, (float)z };
					int RelChunkX = (int)floorf(cameraPos.x - 1) / 32;
					int RelChunkX1 = (int)floorf(cameraPos.x + 1) / 32;

					int RelChunkZ = (int)floorf(cameraPos.z - 1) / 32;
					int RelChunkZ1 = (int)floorf(cameraPos.z + 1) / 32;
					
					Vector3 RelChunkPos = {0, 0, 0};

					for (int i = 0; i < 6; i++)
					{
						if (blockPos.x == 0) {
							RelChunkPos.x = (float)RelChunkX;
						} else if (blockPos.x == 31) {
							RelChunkPos.x = (float)RelChunkX1;
						} else {
							RelChunkPos.x = (float)chunkX;
						}

						if (blockPos.z == 0) {
							RelChunkPos.z = (float)RelChunkZ;
						} else if (blockPos.z == 31) {
							RelChunkPos.z = (float)RelChunkZ1;
						} else {
							RelChunkPos.z = (float)chunkZ;
						}

						Vector3 RelBlockPos = BLOCKPOS(blockPos, Direction[i]);
						auto RelChunkPtr = Chunks.find({ RelChunkPos.x, RelChunkPos.z });
						if (RelChunkPtr == Chunks.end()) continue;
						ChunkPrefab RelChunk = RelChunkPtr->second;

						int bx = (int)floorf(RelBlockPos.x);
						int by = (int)floorf(RelBlockPos.y);
						int bz = (int)floorf(RelBlockPos.z);

						auto blockIt = RelChunk.Blocks.find({ bx, by, bz });
						if (blockIt != RelChunk.Blocks.end() && isTransparent(blockIt->second)) {
							float avgZ = 0;
							for (int j = 0; j < 4; j++) {
								Vector3 world = ADDVECTOR(blockPos, Verts[i][j]);
								Vector3 local = rotate(SUBSVECTOR(world, cameraPos), cameraRot);
								avgZ += local.z;
							}
							avgZ /= 4.0f;
							allFaces.push_back({ blockPos, i, blockID, avgZ });
						}
					}

				}
			}
		}
		std::sort(allFaces.begin(), allFaces.end(), [](const DrawnFace& a, const DrawnFace& b) {
			return a.avgZ > b.avgZ;
			});
		for (const auto& face : allFaces) {
			Face(mesh, cameraPos, cameraRot, face.blockPos, face.blockID, screenSize, face.side);
		}
	}
	bool Collition(Vector3& PlayerPos, int FullRange, int yRange, bool Swim, bool Block)
	{

		int newX = (int)(PlayerPos.x + (int)(FullRange / 2.0f - 1));
		int newY = (int)(PlayerPos.y + (int)(yRange / 2.0f));
		int newZ = (int)(PlayerPos.z + (int)(FullRange / 2.0f - 1));

		Vector3 chunk = { floorf((float)(newX) / 32), floorf((float)(newZ) / 32) };
		int localX = (int)(newX % 32);
		int localZ = (int)(newZ % 32);

		//std::cout << "Chunk: " << chunk << ", LocalX: " << localX << ", FootY: " << footY << ", HeadY: " << headY << ", Block Info: " << Chunks[chunk].Blocks[localX][footY] << std::endl;

		bool blockHead = !isTransparent(Chunks[{(int)chunk.x, (int)chunk.z}].Blocks[{localX, newY, localZ}]);

		if (Swim && !blockHead) {
			blockHead = canSwim(Chunks[{(int)chunk.x, (int)chunk.z}].Blocks[{localX, newY, newZ}]);
		}

		if (Block) {
			blockHead = false;
		}

		return blockHead;
	}
	bool PlaceBlock(int BlockType, Vector3 Position, int yRange, Vector3 PlayerPosition, short& Type)
	{
		/*
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
		*/
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
		Vector3 ChunkPos = {
				floorf(Chunk.x / 32),
				floorf(Chunk.y / 32),
				floorf(Chunk.z / 32)
		};
		auto key = std::make_tuple((int)ChunkPos.x, (int)ChunkPos.z);
		if (Chunks.find(key) != Chunks.end()) return;

		ChunkPrefab newChunk;
		newChunk.xPos = (int)ChunkPos.x * newChunk.xSize;
		newChunk.zPos = (int)ChunkPos.z * newChunk.zSize;
		newChunk.GenerateChunk();
		Chunks[key] = newChunk;

	}
}