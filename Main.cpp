#include "common.hpp"
#include "GameServer.h"
#include "GameClient.h"

int main(int argc, char* argv[])
{
	srand(static_cast<unsigned int>(time(0)));

	int choice = 0;

	do {
		std::cout << "Choose an option:\n1. Start Server\n2. Start Client\n";
		std::cin >> choice;

		if (choice == 1) {
			GameServer server;
			server.set_seed(rand());
			server.AcceptClients();
			//std::thread(&GameServer::AcceptClients, &server, std::ref(is_running), Range).detach();
		} else if (choice == 2) {
			
			GameClient client;
			BitMiner::GameLoop(client);
			//std::thread(BitMiner::GameLoop, std::ref(is_running), std::ref(client)).detach();
		}
		else {
			std::cout << "Invalid choice.\n";
		}
	} while (choice > 2 || choice < 1);

	std::cout << "Exiting game..." << std::endl;

	return 0;
}