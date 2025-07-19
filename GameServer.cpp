#include "GameServer.h"

void GameServer::handlePlayers(SOCKET player, bool Running, int Id)
{
	char buf[16] = {};
	int res = 0;
	//std::cout << player << std::endl;

	while (Running) {

		res = recv(player, buf, sizeof(buf), 0);

		//std::cout << res << std::endl;

		if (res <= 0) {
			std::cerr << "Client disconnected or error occurred\n";
			std::cerr << "recv() failed with error: " << WSAGetLastError() << std::endl;
			return;
		}

		//std::cout << "Result: " << res << "\n";
		std::cout << "Buf:" << buf << "\n";

		if (strcmp(buf, "seed") == 0) {
			std::string seed_str = std::to_string(this->seed);
			res = send(player, seed_str.c_str(), seed_str.size(), 0);
			std::cout << res << std::endl;

		}
		else if (strcmp(buf, "getColor") == 0) {
			std::string color_str = std::to_string(this->players[Id].color.r) + "," +  
								   std::to_string(this->players[Id].color.g) + "," +  
								   std::to_string(this->players[Id].color.b);
			std::cout << "sending..." << std::endl;
			res = send(player, color_str.c_str(), color_str.size(), 0);

			std::cout << "Color: " << color_str[0] <<  std::endl;
		}
	}
}

void GameServer::AcceptClients(bool& Running, Vector3 Range)
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

		this->add_socket(clientSocket, Player{ Vector3{ 0, 24, 0 }, Vector3{ 0, 0, 0 }, PlayerColors[player_count] });

		//std::cout << player_count << " clients connected." << std::endl;
		//std::cout << "Client connected! Socket: " << clientSocket << std::endl;

		// TODO: no et preocupis nigga

		handlePlayers(clientSocket, Running, player_count - 1);
		//std::thread(&GameServer::handlePlayers, this, clientSocket, true, this->player_count - 1).detach();
	}
}