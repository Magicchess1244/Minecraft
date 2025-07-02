#include "GameClient.h"
#include "GameServer.h"

#include <iostream>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

int main(int Argc, char* Argv[])
{
	srand(static_cast<unsigned int>(time(0)));

	int choice = 0;
	bool is_running = true;
	Vector3 Range = { 16, 10 };

	do {
		std::cout << "Choose an option:\n1. Start Server\n2. Start Client\n";
		std::cin >> choice;

		if (choice == 1) {
			GameServer server;
			server.set_seed(rand());
			std::thread(&GameServer::AcceptClients, &server, std::ref(is_running), Range).detach();
			while (is_running) {}
		} else if (choice == 2) {
			
			GameClient client;
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