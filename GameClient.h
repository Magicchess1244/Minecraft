#ifndef __GAME_CLIENT_H
#define __GAME_CLIENT_H

#include "common.hpp"
#include "Renderer.h"

typedef enum
{
	GetSeed,
	GetColor
} Commands;

class GameClient
{
private:
	unsigned int seed;
	unsigned int player_count = 0;
	std::vector<Player> players;
	SOCKET server;
	bool running = true;

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

	SOCKET get_socket() const {
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
		if (players.size() > 0) {
			return players[0].color;
		}

		return {0,0,0}; // NIGGAS
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

		sockaddr_in server = { 0 };
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

	bool GetRunning() {
		return running;
	}
	void Quit() {
		running = false;
	}
};

namespace BitMiner {
	int FindSlot(std::vector<Slot>& Inventory, short Type);
	void PlayerInput(Vector3& PlayerDirection, bool OnGround, int& InventorySlots, Vector3& PlayerRot);
	void PlayerMovement(Player& player, int& inventorySlot);
	void GameLoop(GameClient& game);
}

#endif