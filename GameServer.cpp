#include "GameServer.h"
#include <iostream>

void GameServer::handlePlayers(SOCKET player)
{
	char buf[8];
	int res;
	
	res = recv(player, buf, sizeof(buf), 0);
	if (res <= 0) std::cerr << "Client disconnected or error occurred\n";
	if (res != sizeof(buf)) return;

	std::cout << "Result: " << res << "\n";
	std::cout << "Buf:" << buf << "\n";



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

void AcceptClients(SOCKET hostSocket, std::vector<SOCKET>& sockets, bool& Running, int seed, std::vector<Players>& PlayerPos, Vector2 Range)
{
	int Index = 0;
	while (sockets.size() < MAX_PLAYERS) {
		sockaddr_in client;
		int clientSize = sizeof(sockaddr_in);

		SOCKET clientSocket = accept(hostSocket, (sockaddr*)&client, &clientSize);
		if (clientSocket == INVALID_SOCKET) {
			std::cerr << "accept() failed with error: " << WSAGetLastError() << std::endl;
			continue;
		}

		if (Index != 0)
		{
			sockets.push_back(clientSocket);
			Players newPlayer = { { 800, 64 }, PlayerColors[Index] };
			PlayerPos.push_back(newPlayer);
		}


		std::cout << sockets.size() << " clients connected." << std::endl;
		std::cout << "Client connected! Socket: " << clientSocket << std::endl;

		std::thread(handleClientMessage, clientSocket, std::ref(sockets), Index, true, seed, std::ref(Running), std::ref(PlayerPos), Range).detach();
		Index++;
	}
}