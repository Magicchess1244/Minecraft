#pragma once
#include <vector>
#include <winsock2.h>

static constexpr unsigned int MAX_PLAYERS = 8;

class GameServer
{
private:
	unsigned int seed;
	unsigned int player_count;
	SOCKET listener;
	std::vector<SOCKET> players;
public:
	GameServer() : seed(0), player_count(0), listener(INVALID_SOCKET) {}
	void set_seed(unsigned int new_seed) {
		seed = new_seed;
	}

	unsigned int get_seed() const {
		return seed;
	}

	void set_listener(SOCKET socket) {
		listener = socket;
	}

	void add_socket(SOCKET socket) {
		if (players.size() < MAX_PLAYERS) {
			players.push_back(socket);
			player_count++;
		}
	}
	const std::vector<SOCKET>& get_sockets() const {
		return players;
	}

	void handlePlayers(SOCKET player);

	~GameServer()
	{
		for (SOCKET socket : players)
		{
			closesocket(socket);
		}
		WSACleanup();
		std::cout << "Server closed.\n";
	}

};

