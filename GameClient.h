#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <WinSock2.h>
#include <SDL3/SDL.h>
#include "PerlinNoise.h"
#include "ChunkManager.h"

static constexpr unsigned int MAX_PLAYERS = 8;

typedef struct {
	unsigned int r, g, b;
} Color;
enum Commands
{
	GetSeed,
	GetColor
};

typedef struct {
	Vector2 Position;
	Color color;
} Player;

class GameClient
{
private:
	unsigned int seed;
	unsigned int player_count;
	std::vector<Player> players;
	SOCKET server;

public:
	GameClient() : seed(0), player_count(0), server(INVALID_SOCKET) {}
	~GameClient() {
		std::cout << "closing conn" << std::endl;
		closesocket(server);
		WSACleanup();
	}

	void set_seed();
	unsigned int get_seed() const {
		return seed;
	}

	void set_socket(SOCKET connection) {
		server = connection;
	}
	SOCKET get_socket(){
		return server;
	}

	void add_player(Player player) {
		if (players.size() < MAX_PLAYERS) {
			this->players.push_back(player);
			this->player_count++;		
		}
		std::cout << "players size: " << players.size() << std::endl;
	}
	const std::vector<Player> get_players() {
		return this->players;
	}

	void set_color();
	Color get_color() const {
		return {255,0,0};
	}
};

namespace BitMiner {
	int FindSlot(std::vector<Slot>& Inventory, short Type);
	void Input(Vector2& PlayerDirection, bool OnGround, int& InventorySlots, Vector2& PlayerPos, Vector2 Range);
	void DrawBG(SDL_Renderer* Renderer, Vector2& PlayerPos, Vector2 Range);
	void DrawPlayer(SDL_Renderer* Renderer, Vector2 Range, std::vector<Player>& PlayerPos);
	int SetUpRender(SDL_Window** Window, SDL_Renderer** Renderer);
	void Render(SDL_Event event, SDL_Renderer** renderer, SDL_Window** window, Vector2 Range, int width, int height, std::vector<Slot>& inventory, int inventorySlot, std::vector<Player>& players, bool& Running, bool& FullScreen, TTF_Font* font);
	void PlayerMovement(Vector2& playerDirection, Vector2& range, Player& player, int& inventorySlot);
	int GameLoop(bool& running, GameClient& game);
}