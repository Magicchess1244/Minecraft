#include "ChunkManager.h"

constexpr int ySize = 64;
constexpr Biome Biomes[11] = {
	{ 20, 20 , 0, 0, 20, 6}, // Ice
	{ 40, 20 , 20, 0, 20, 6}, // Tundra
	{ 100, 20 , 40, 0, 20, 7}, // Taiga
	{ 100, 40 , 60, 20, 20, 4}, // Big Taiga
	{ 60, 60 , 20, 20, 20, 3 }, // Plains
	{ 80, 60 , 20, 40, 20, 6 }, // Forest
	{ 80, 60 , 20, 60, 20, 5 }, // Birch
	{ 100, 60 , 20, 80, 20, 5 }, // Dark Forest
	{ 100, 100 , 60, 60, 20, 7 }, // Jungle
	{ 60, 100 , 0, 80, 20, 4 }, // Desert
	{ 40, 80 , 20, 0, 20, 5 }, // Savanna
};
constexpr HeightsDif ContinentelnessHeight[8] = {
	{1, ySize},
	{0.4f, ySize * 0.9f},
	{0.15f, ySize * 0.8f},
	{-0.15f, ySize * 0.45f},
	{-0.35f, ySize * 0.45f},
	{-0.65f, ySize * 0.1f},
	{-0.9f, ySize * 0.09f},
	{-1.0f, ySize}
};
constexpr HeightsDif ErrotionHeight[10] = {
	{1, ySize * 0.05f},
	{0.8f, ySize * 0.1f},
	{0.7f, ySize * 0.35f},
	{0.4f, ySize * 0.35f},
	{0.3f, ySize * 0.11f},
	{-0.2f, ySize * 0.2f},
	{-0.4f, ySize * 0.6f},
	{-0.5f, ySize * 0.5f},
	{0.8f, ySize * 0.9f},
	{0, ySize},
};
constexpr HeightsDif PeaksAndValiesHeight[6] = {
	{1, ySize},
	{0.8f, ySize * 0.9f},
	{0.5f, ySize * 0.95f},
	{0.05f, ySize * 0.35f},
	{-0.4f, ySize * 0.3f},
	{-0.9f, ySize * 0.1f},
};



int BlockSize = 50;

ChunkPrefab& ChunkManager::get_chunk(Vector3 key)
{
<<<<<<< HEAD
	if (this->Chunks.find(key) != this->Chunks.end()) {
		return std::ref(Chunks[key]);
=======
	return blockID == 0 || blockID == 5;
}
bool canSwim(int blockID)
{
	return blockID == 5;
}
void PrintChunk(int i, int xPlayerPos, int yPlayerPos, int xRange, int yRange, int FullRange)
{
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
}
void DrawChunk(int i, int xPlayerPos, int yPlayerPos, int xRange, int yRange, int FullRange, Mesh* mesh, bool FirstChunck)
{
	int xSize = Chunks[i].xSize;
	int ySize = Chunks[i].ySize;
	int xPos = Chunks[i].xPos;
	int quadIndex = 0;
	int OffSet = ((int)xPlayerPos - xPlayerPos);

	if (!FirstChunck) {
		OffSet += FullRange - xRange;
	}

	for (int y = 0; y < yRange; y++) {
		for (int x = 0; x < xRange; x++) {
			
			SDL_FColor Color;

			Color = BlockDef[Chunks[i].Blocks[xPlayerPos + x][yPlayerPos + y]].Color;
			
			int vIndex = quadIndex * 4;
			int iIndex = quadIndex * 6;
			float FlipX = (x + OffSet) * BlockSize;
			float FlipY = (yRange - y -1) * BlockSize;

			mesh->Vertices[vIndex + 0] = { {FlipX, FlipY}, Color };
			mesh->Vertices[vIndex + 1] = { {FlipX + BlockSize, FlipY}, Color};
			mesh->Vertices[vIndex + 2] = { {FlipX, FlipY + BlockSize}, Color};
			mesh->Vertices[vIndex + 3] = { {FlipX + BlockSize, FlipY + BlockSize}, Color};

			mesh->Indices[iIndex + 0] = vIndex + 0;
			mesh->Indices[iIndex + 1] = vIndex + 1;
			mesh->Indices[iIndex + 2] = vIndex + 2;
			mesh->Indices[iIndex + 3] = vIndex + 2;
			mesh->Indices[iIndex + 4] = vIndex + 1;
			mesh->Indices[iIndex + 5] = vIndex + 3;

			quadIndex++;
		}
	}
}
bool Collition(Vector2* PlayerPos, Vector2 Direction, int FullRange, int yRange, bool Swim, bool Block)
{

		int newX = PlayerPos->x + (int)(FullRange / 2.0f - 1) + Direction.x;
		int newY = PlayerPos->y + (int)(yRange / 2.0f) + Direction.y;

		int chunk = (int)(newX) / 16;
		int localX = (int)(newX % 16);
		int footY = (int)(newY);
		int headY = (int)(newY + 1);

		//std::cout << "Chunk: " << chunk << ", LocalX: " << localX << ", FootY: " << footY << ", HeadY: " << headY << ", Block Info: " << Chunks[chunk].Blocks[localX][footY] << std::endl;

		bool blockFoot = !isTransparent(Chunks[chunk].Blocks[localX][footY]);
		bool blockHead = !isTransparent(Chunks[chunk].Blocks[localX][headY]);

		if (Swim && !(blockFoot || blockHead)) {
			blockFoot = canSwim(Chunks[chunk].Blocks[localX][footY]);
			blockHead = canSwim(Chunks[chunk].Blocks[localX][headY]);
		}

		if (Block) {
			blockHead = false;
		}

		return (blockFoot || blockHead);
}
bool PlaceBlock(int BlockType, Vector2 Position, int yRange, Vector2 PlayerPosition, short* Type) 
{
	Position.x = (int)(Position.x / BlockSize ) + PlayerPosition.x;
	Position.y = (yRange - (int)(Position.y / BlockSize) - 1) + PlayerPosition.y;


	int CurrrentChunk = (int)floorf((Position.x) / 16);
	int RelativeX = (int)(Position.x) % 16;

	
	bool notBedRock = (Chunks[CurrrentChunk].Blocks[RelativeX][(int)Position.y] != 4);
	bool water = (Chunks[CurrrentChunk].Blocks[RelativeX][(int)Position.y] == 5);
	bool canPlace = (BlockType != 0 && (Chunks[CurrrentChunk].Blocks[RelativeX][(int)Position.y] == 0 || water));
	bool canBreak = (BlockType == 0 && (Chunks[CurrrentChunk].Blocks[RelativeX][(int)Position.y] != 0 && !water));

	//std::cout << canBreak << " " << canPlace << std::endl;

	if (notBedRock && (canPlace || canBreak)) {
		std::cout << "Placing Block: " << BlockType << ", Position: " << Position.x << ", " << Position.y << "Chunk: " << CurrrentChunk << std::endl;
		*Type = Chunks[CurrrentChunk].Blocks[RelativeX][(int)Position.y];
		Chunks[CurrrentChunk].Blocks[RelativeX][(int)Position.y] = BlockType;
		return true;
	}
	return false;
}
void Size(int PixelSizeX, int PixelSizeY, int yRange, int FullRange)
{
	if (PixelSizeX > PixelSizeY)
	{
		BlockSize = (int)(PixelSizeY / yRange);
>>>>>>> parent of a48f4a5 (Fic¡x tcp but creating one extra client)
	}
	else
	{
		std::cout << "Generating chunk at: " << key.x << ", " << key.z << std::endl;

		ChunkPrefab newChunk;
		newChunk.xPos = (int)key.x * newChunk.xSize;
		newChunk.zPos = (int)key.z * newChunk.zSize;
		newChunk.GenerateChunk();
		this->Chunks[key] = newChunk;
		return std::ref(Chunks[key]);
	}
}
<<<<<<< HEAD
=======
int GetHeight(int xPos)
{
	return Chunks[xPos].Height;
}
void ShowInventor(SDL_Renderer* Renderer, int width, int height, Slot* Inventory, int InventorySlot)
{
	SDL_SetRenderDrawColor(Renderer, 157, 76, 0, 255);
	SDL_FRect InventoryRect = { (width / 2) - (BlockSize * 4), (height - (BlockSize * 1.3f)), (BlockSize * 1.1f) * 8 + (BlockSize * .1f), (BlockSize * 1.2f) };
	SDL_RenderFillRect(Renderer, &InventoryRect);
>>>>>>> parent of a48f4a5 (Fic¡x tcp but creating one extra client)

int ChunkManager::BaseHeight(double ValueNoise, int Length, const HeightsDif* Heights)
{
	for (int i = 0; i < Length - 1; i++) {
		if (ValueNoise > ContinentelnessHeight[i].x) {
			std::cout << "Continentalness: " << Lerp(Heights[i].y, Heights[i + 1].y, (ValueNoise + 1 / (Heights[i + 1].x + 1))) << std::endl;
			return Lerp(Heights[i + 1].y, Heights[i].y, (ValueNoise + 1 / (Heights[i + 1].x + 1)));
		}
	}
	return Heights[Length - 1].y;
}
Biome ChunkManager::GetBiome(double Humudity, double Temperature)
{
	Biome TheBiome;

	for (int i = 0; i < 11; i++)
	{
		bool allowed_humidity = (Humudity < Biomes[i].MaxHumidity && Humudity > Biomes[i].MinHumidity);
		bool allowed_temperature = (Temperature < Biomes[i].MaxTemperature && Temperature > Biomes[i].MinTemperature);

		if (allowed_humidity && allowed_temperature) {
			TheBiome = Biomes[i];
		}
	}

	return TheBiome;
}
int ChunkManager::GetHeight(double Continentalness, double Errotion, double PeakAndValleys) {
	return (int)(BaseHeight(Continentalness, 8, ContinentelnessHeight));
}

/*
namespace ChunckManager {
	static bool isTransparent(int blockID)
	{
		return blockID == 0 || blockID == 5;
	}
	static bool canSwim(int blockID)
	{
		return blockID == 5;
	}
	bool Collition(Vector3& PlayerPos, int FullRange, int yRange, bool Swim, bool Block)
	{
		int newX = (int)(PlayerPos.x + (int)(FullRange / 2.0f - 1));
		int newY = (int)(PlayerPos.y + (int)(yRange / 2.0f));
		int newZ = (int)(PlayerPos.z + (int)(FullRange / 2.0f - 1));

		Vector3 chunk = { floorf((double)(newX) / 32), floorf((double)(newZ) / 32) };
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
<<<<<<< HEAD
	}
	void ShowInventor(SDL_Renderer* Renderer, int width, int height, std::vector<Slot>& Inventory, int InventorySlot, TTF_Font* font)
	{
		SDL_SetRenderDrawColor(Renderer, 157, 76, 0, 255);
		SDL_FRect InventoryRect = { (double)(width / 2) - (BlockSize * 4), (height - (BlockSize * 1.3f)), (BlockSize * 1.1f) * 8 + (BlockSize * .1f), (BlockSize * 1.2f) };
=======
		SDL_FRect InventoryRect = { ((width / 2) - (BlockSize * 4)) + (BlockSize * 1.1f * i) + (BlockSize * 0.1f), (height - (BlockSize * 1.3f)) + (BlockSize * 0.1f), BlockSize, BlockSize };
>>>>>>> parent of a48f4a5 (Fic¡x tcp but creating one extra client)
		SDL_RenderFillRect(Renderer, &InventoryRect);

		for (int i = 0; i < 8; i++)
		{
<<<<<<< HEAD
			if (InventorySlot == i)
			{
				SDL_SetRenderDrawColor(Renderer, 255, 153, 56, 255);
=======
			SDL_SetRenderDrawColor(Renderer, BlockDef[Inventory[i].Type].Color.r * 255, BlockDef[Inventory[i].Type].Color.g * 255, BlockDef[Inventory[i].Type].Color.b * 255, BlockDef[Inventory[i].Type].Color.a * 255);
			SDL_FRect BlockRect = { ((width / 2) - (BlockSize * 4)) + (BlockSize * 1.1f * i) + (BlockSize * 0.3f), (height - (BlockSize * 1.3f)) + (BlockSize * 0.3f), BlockSize * 0.6f, BlockSize * 0.6f };
			SDL_RenderFillRect(Renderer, &BlockRect);
			//Add be a number
		}

	}
}
void SimulateWater(int chunkIndex) {
	std::queue<std::pair<int, int> > q; 

	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 64; y++) {
			if (Chunks[chunkIndex].Blocks[x][y] == 5) {
				int globalX = chunkIndex * 16 + x;
				q.push(std::make_pair(globalX, y));
>>>>>>> parent of a48f4a5 (Fic¡x tcp but creating one extra client)
			}
			else
			{
				SDL_SetRenderDrawColor(Renderer, 204, 102, 0, 255);
			}
			SDL_FRect InventoryRect = { (double)(((width / 2) - (BlockSize * 4)) + (BlockSize * 1.1f * i) + (BlockSize * 0.1f)), (double)((height - (BlockSize * 1.3f)) + (BlockSize * 0.1f)), (double)BlockSize, (double)BlockSize };
			SDL_RenderFillRect(Renderer, &InventoryRect);

			if (Inventory[i].Type != 0)
			{
				SDL_SetRenderDrawColor(Renderer, BlockDef[Inventory[i].Type].Color.r, BlockDef[Inventory[i].Type].Color.g, BlockDef[Inventory[i].Type].Color.b, 1);
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
					SDL_FRect dstRect{ (double)(((width / 2) - (BlockSize * 4)) + (BlockSize * 1.1f * i) + (BlockSize * 0.8f)), (double)((height - (BlockSize * 1.1f)) + (BlockSize * 0.4f)), BlockSize * 0.5f , BlockSize * 0.5f};
					SDL_RenderTexture(Renderer, texture, NULL, &dstRect);
					SDL_DestroyTexture(texture);
				}
			}

		}
	}
	void SimulateWater(int chunkIndex) {
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
	}
}
*/