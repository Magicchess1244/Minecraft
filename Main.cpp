#include "common.hpp"
#include "GameServer.h"
#include "GameClient.h"

int main(int argc, char* argv[])
{
	srand(static_cast<unsigned int>(time(0)));

	int choice = 0;
	GameServer server;
	GameClient client;

	do {
		std::cout << "Choose an option:\n1. Start Server\n2. Start Client\n";
		std::cin >> choice;

		switch (choice) {
		case 1:
			server.set_seed(rand());
			server.AcceptClients();
			break;
		case 2:
			BitMiner::GameLoop(client);
			break;
		default:
			std::cout << "Invalid choice.\n";
			break;
		}
	} while (choice > 2 || choice < 1);

	std::cout << "Exiting game..." << std::endl;

	return 0;
}