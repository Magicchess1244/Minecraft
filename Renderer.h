#pragma once

#include "common.hpp"

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
	TTF_Font* font = nullptr;
	SDL_Texture* texture = nullptr;
	Vector3 ScreenSize = { 0,0,0 };
	

	SDL_FPoint getUV(int tileIndex, int cornerX, int cornerY);
	Vector3 rotate(const Vector3 pos, Vector3 Angle);
	void DrawFace(Mesh& mesh, Player& player, Vector3 blocks, int color, int Side);
public:
	Renderer();
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
	};

	void RenderChunk(ChunkPrefab& chunk, Player& player, Mesh& mesh);
	void DrawTerrain(Player& player);
};