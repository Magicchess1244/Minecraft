#include "GameClient.h"
#include "GameServer.h"

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <iomanip>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

//Networking functions
SOCKET MakeServer()
{
	WSADATA wsaData;
	if (WSAStartup(WINSOCK_VERSION, &wsaData) != 0) {
		std::cerr << "Failed to initialize WinSock\n";
		return INVALID_SOCKET;
	}

	SOCKET hostSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (hostSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed\n";
		WSACleanup();
		return INVALID_SOCKET;
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
		WSACleanup();
		return INVALID_SOCKET;
	}

	if (listen(hostSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "Listen failed\n";
		closesocket(hostSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}

	std::cout << "Server listening on port " << PORT << "...\n";
	return hostSocket;
}
SOCKET MakeClient() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed.\n";
		return INVALID_SOCKET;
	}

	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "Failed to create socket.\n";
		WSACleanup();
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
		WSACleanup();
		return INVALID_SOCKET;
	}

	std::cout << "Connected to the server.\n";
	return serverSocket;
}

int main(int Argc, char* Argv[])
{
	srand(static_cast<unsigned int>(time(0)));

	int choice = 0;
	do {
		std::cout << "Choose an option:\n1. Start Server\n2. Start Client\n";
		std::cin >> choice;

		if (choice == 1) {
			GameServer server;
			server.set_seed(rand());
			server.set_listener(MakeServer());
		}
		else if (choice == 2) {
			GameClient client;
			client.set_socket(MakeClient());
			client.set_seed();
			client.add_player({ 800.0, 64.0 , { 255, 0, 0} });
		}
		else {
			std::cout << "Invalid choice.\n";
		}

		choice = 0;
	} while (choice > 2 || choice < 1);


	/*
	SOCKET host_socket = INVALID_SOCKET;
	Vector2 Range = { 16, 10 };
	int seed = 0;
	bool is_running = true;
	bool is_server;

	if (is_server)
	{
		std::thread(AcceptClients,
			host_socket,
			std::ref(sockets),
			std::ref(Running),
			seed,
			std::ref(players),
			Range).detach();

		std::thread(handleClientMessage,
			sockets[0],
			std::ref(sockets),
			0,
			false,
			seed,
			std::ref(Running),
			std::ref(players),
			Range).detach();
	}

	players[0].Color = GetColor(sockets[0]);

	std::thread(Ui,
		sockets[0],
		Range, 
		std::ref(Running),
		std::ref(players)).detach();

	while (Running) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	std::cout << "Exiting game..." << std::endl;
	*/

	return 0;
}