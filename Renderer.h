#pragma once

#include "common.hpp"

#include "Chunck.h"
#include "ChunkManager.h"
#include "GameClient.h"

typedef struct
{
	std::vector<SDL_Vertex> Vertices;
	std::vector<int> Indices;
	int faces;
} Mesh;

class Renderer 
{
private:
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Event event;
	TTF_Font* font = nullptr;
	SDL_Texture* texture = nullptr;
	Vector3 ScreenSize = { 0,0,0 };
	ChunkManager chunkManager;
	GameClient& gameClient;
	int BlockPixelSize = 50;
	Mesh terrainMesh;
	bool fullScreen = false;

	SDL_FPoint getUV(int tileIndex, int cornerX, int cornerY);
	Vector3 rotate(const Vector3 pos, Vector3 Angle);
	void DrawFace(Mesh& mesh, Player& player, Vector3 blocks, int color, int Side);
public:
	Renderer(GameClient& client);
	~Renderer() {
		if (renderer) {
			SDL_DestroyRenderer(renderer);
			renderer = nullptr;
		}
		if (window) {
			SDL_DestroyWindow(window);
			window = nullptr;
		}
		if (font) {
			TTF_CloseFont(font);
			font = nullptr;
		}
		if (texture) {
			SDL_DestroyTexture(texture);
			texture = nullptr;
		}
		event = {};
		terrainMesh = {};
	};

	void RenderChunk(ChunkPrefab& chunk, Player& player, Mesh& mesh);
	void DrawTerrain(Player& player);
	void DrawPlayer(SDL_Renderer* Renderer, Vector3 Range, std::vector<Player>& PlayerPos);
	void MainRenderLoop(std::vector<Slot>& inventory, int inventorySlot, std::vector<Player>& players);
};