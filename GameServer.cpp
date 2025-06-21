#include "GameServer.h"
#include <iostream>
#include <thread>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

void GameServer::handlePlayers(SOCKET player, bool Running)
{
	char buf[16];
	int res;


	while (Running) {
		std::cout << player << std::endl;

		res = recv(player, buf, sizeof(buf), 0);

		std::cout << res << std::endl;

		if (res <= 0) {
			std::cerr << "Client disconnected or error occurred\n";
			std::cerr << "recv() failed with error: " << WSAGetLastError() << std::endl;
			return;
		}

		std::cout << "Result: " << res << "\n";
		std::cout << "Buf:" << buf << "\n";

		if (strcmp(buf, "seed") == 0) {
			std::string seed_str = std::to_string(this->seed);
			res = send(player, seed_str.c_str(), seed_str.size(), 0);
			std::cout << res << std::endl;

		}
	}

	/*

	if (cmd == UPDATE_PLAYER_POS) {
		std::cout << "Received command: " << cmd << "\n";
		for (int i = 0; i < clients.size(); i++) {
			if (client != clients[i]) {
				//UpdatePlayerPos(clients[i],
					//SDL_Color{ (Uint8)command[3], (Uint8)command[4], (Uint8)command[5], 255 },
					//Vector2{ (float)command[1], (float)command[2] });
			}
		}
	}
	else if (cmd == GET_SEED) {
		std::cout << "Received command: " << cmd << "\n";
		char seedPacket[6] = {
			seed,
			0,0,0,0,0
		};
		int sent = send(client, seedPacket, sizeof(seedPacket), 0);
		if (sent == SOCKET_ERROR) {
			std::cerr << "Failed to send seed\n";
		}
	}
	else if (cmd == GET_COLOR) {
		std::cout << "Received command: " << cmd << "\n";
		char colorPacket[6] = {
			PlayerColors[id].r,
			PlayerColors[id].g,
			PlayerColors[id].b,
			0,0,0
		};
		int sent = send(client, colorPacket, sizeof(colorPacket), 0);
		if (sent != sizeof(colorPacket)) {
			std::cerr << "Failed to send color packet\n";
		}
	}
	*/
}

void GameServer::AcceptClients(bool& Running, Vector2 Range)
{
	std::map<int, Color> PlayerColors = {
		{0, {255, 0, 0} },
		{1, {0, 255, 0} },
		{2, {0, 0, 255} },
		{3, {255, 255, 0} },
		{4, {255, 0, 255} },
		{5, {0, 255, 255} },
		{6, {128, 128, 128} },
		{7, {255, 165, 0} }
	};
	this->MakeServer();
	std::cout << "Listener: " << listener << "\n";

	while (player_count < MAX_PLAYERS && Running) {
		SOCKET clientSocket = accept(this->listener, NULL, NULL);

		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "accept() failed with error: " << WSAGetLastError() << std::endl;
			continue;
		}

		this->add_socket(clientSocket, Player{ Vector2{ 800, 64 }, PlayerColors[player_count] });

		std::cout << player_count << " clients connected." << std::endl;
		std::cout << "Client connected! Socket: " << clientSocket << std::endl;

		std::thread(&GameServer::handlePlayers, this, clientSocket, true).detach();
		std::cout << "a" << std::endl;
	}
}