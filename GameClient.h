#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <WinSock2.h>
#include <SDL3/SDL.h>

static constexpr unsigned int MAX_PLAYERS = 8;

typedef struct {
	unsigned int r, g, b;
} Color;

typedef struct {
	double x, y;
	Color color;
} Player;

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

class GameClient
{
private:
	unsigned int seed;
	unsigned int player_count;
	std::vector<Player> players;
	SOCKET server;

public:
	GameClient() : seed(0), player_count(0), server(INVALID_SOCKET) {}
	~GameClient() {
		closesocket(server);
		WSACleanup();
	}

	void set_seed();
	unsigned int get_seed() const {
		return seed;
	}

	void set_socket(SOCKET connection) {
		server = connection;
	}

	void add_player(const Player& player) {
		if (players.size() < MAX_PLAYERS) {
			players.push_back(player);
			player_count++;
		}
	}
	const std::vector<Player>& get_players() const {
		return players;
	}
};

