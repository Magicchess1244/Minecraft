#pragma once

#include "common.hpp"

class GameServer
{
private:
	unsigned int seed;
	unsigned int player_count = 0;
	SOCKET listener;
	std::vector<SOCKET> sockets;
	std::vector<Player> players;
	void MakeServer()
	{
		WSADATA wsaData;
		if (WSAStartup(WINSOCK_VERSION, &wsaData) != 0) {
			std::cerr << "Failed to initialize WinSock\n";
			return;
		}

		SOCKET hostSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (hostSocket == INVALID_SOCKET) {
			std::cerr << "Socket creation failed\n";
			return;
		}

		int opt = 1;
		setsockopt(hostSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

		constexpr int PORT = 8080;

		sockaddr_in serverAddr{};
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = INADDR_ANY;
		serverAddr.sin_port = htons(PORT);

		if (bind(hostSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
			std::cerr << "Bind failed\n";
			closesocket(hostSocket);
			return;
		}
		if (listen(hostSocket, SOMAXCONN) == SOCKET_ERROR) {
			std::cerr << "Listen failed\n";
			closesocket(hostSocket);
			return;
		}
		else
		{
			std::cerr << "working\n";
		}

		std::cout << "Server listening on port " << PORT << "...\n";
		this->listener = hostSocket;
	}

public:
	GameServer() : seed(0), player_count(0), listener(INVALID_SOCKET) {}
	void set_seed(unsigned int new_seed) {
		std::cout << "seed: " << new_seed << std::endl;
		seed = new_seed;
	}

	unsigned int get_seed() const {
		return seed;
	}

	void add_socket(SOCKET socket, Player player) {
		if (sockets.size() < MAX_PLAYERS) {
			sockets.push_back(socket);
			players.push_back(player);
			player_count++;
		}
	}
	const std::vector<SOCKET>& get_sockets() const {
		return sockets;
	}

	void handlePlayers(SOCKET player, bool Running, int Id);

	void AcceptClients(bool& Running, Vector3 Range);

	~GameServer()
	{
		for (SOCKET socket : sockets)
		{
			closesocket(socket);
		}
		WSACleanup();
		std::cout << "2. Server closed.\n";
	}

};

