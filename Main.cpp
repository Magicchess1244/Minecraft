#include "GameClient.h"
#include "GameServer.h"

#include <iostream>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

//Networking functions
SOCKET MakeClient() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed.\n";
		return INVALID_SOCKET;
	}

	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "Failed to create socket.\n";
		return INVALID_SOCKET;
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
		return INVALID_SOCKET;
	}

	std::cout << "Connected to the server.\n";
	return serverSocket;
}

int main(int Argc, char* Argv[])
{
	srand(static_cast<unsigned int>(time(0)));

	int choice = 0;
	bool is_running = true;
	Vector2 Range = { 16, 10 };

	do {
		std::cout << "Choose an option:\n1. Start Server\n2. Start Client\n";
		std::cin >> choice;

		if (choice == 1) {
			GameServer server;
			server.set_seed(rand());
			std::thread(&GameServer::AcceptClients, &server, std::ref(is_running), Range).detach();
		} else if (choice == 2) {
			
			GameClient client;
			client.set_socket(MakeClient());
			client.set_seed();
			std::thread(BitMiner::GameLoop, std::ref(is_running), std::ref(client)).detach();
			while (is_running) {}
		}
		else {
			std::cout << "Invalid choice.\n";
		}
	} while (choice > 2 || choice < 1);

	while (is_running) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	std::cout << "Exiting game..." << std::endl;

	return 0;
}