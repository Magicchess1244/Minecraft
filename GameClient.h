#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <WinSock2.h>
#include <SDL3/SDL.h>
#include "PerlinNoise.h"
#include "ChunkManager.h"
#include <WS2tcpip.h>

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
	Vector3 Position;
	Color color;
} Player;

class GameClient
{
private:
	unsigned int seed;
	unsigned int player_count = 0;
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

	void MakeClient() {
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			std::cerr << "WSAStartup failed.\n";
			return;
		}

		SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (serverSocket == INVALID_SOCKET) {
			std::cerr << "Failed to create socket.\n";
			return;
		}

		int PORT = 8080;
		const char* SERVER_IP = "127.0.0.1";

		sockaddr_in server;
		server.sin_family = AF_INET;
		server.sin_port = htons(PORT);
		inet_pton(AF_INET, SERVER_IP, &server.sin_addr);

		if (connect(serverSocket, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
			std::cerr << "Failed to connect to the server.\n";
			closesocket(serverSocket);
			return;
		}

		std::cout << "Connected to the server.\n";
		this->server = serverSocket;
	}
};

namespace BitMiner {
	int FindSlot(std::vector<Slot>& Inventory, short Type);
	void Input(Vector3& PlayerDirection, bool OnGround, int& InventorySlots, Vector3& PlayerPos, Vector3 Range);
	void DrawBG(SDL_Renderer* Renderer, Vector3& PlayerPos, Vector3 Range, SDL_Texture* texture);
	void DrawPlayer(SDL_Renderer* Renderer, Vector3 Range, std::vector<Player>& PlayerPos);
	int SetUpRender(SDL_Window** Window, SDL_Renderer** Renderer);
	void Render(SDL_Event event, SDL_Renderer* renderer, SDL_Window* window, Vector3 Range, int& width, int& height, std::vector<Slot>& inventory, int inventorySlot, std::vector<Player>& players, bool& Running, bool& FullScreen, TTF_Font* font, SDL_Texture* texture);
	void PlayerMovement(Vector3& playerDirection, Vector3& range, Player& player, int& inventorySlot);
	void GameLoop(bool& running, GameClient& game);
}