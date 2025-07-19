#ifndef __GAME_CLIENT_H
#define __GAME_CLIENT_H

#include "common.hpp"

static constexpr unsigned int MAX_PLAYERS = 8;

typedef enum
{
	GetSeed,
	GetColor
} Commands;

typedef struct {
	SDL_Event event;
	SDL_Window* window;
	SDL_Renderer* renderer;
	TTF_Font* font;
	SDL_Texture* texture;
	Renderer rendererObj;
	Vector3& screenSize;
	bool& Running, FullScreen;
} RendererParameters;

class GameClient
{
private:
	unsigned int seed;
	unsigned int player_count = 0;
	std::vector<Player> players;
	SOCKET server;

public:
	GameClient() : seed(0), player_count(0), server(INVALID_SOCKET) {}
	~GameClient() {
		std::cout << "closing conn" << std::endl;
		closesocket(server);
		WSACleanup();
	}

	void set_seed();
	unsigned int get_seed() const {
		return seed;
	}

	SOCKET get_socket() const {
		return server;
	}

	void add_player(Player player) {
		if (players.size() < MAX_PLAYERS) {
			this->players.push_back(player);
			this->player_count++;		
		}
		std::cout << "players size: " << players.size() << std::endl;
	}
	const std::vector<Player> get_players() {
		return this->players;
	}

	void set_color();
	Color get_color() const {
		if (players.size() > 0) {
			return players[0].color;
		}

		return {0,0,0}; // NIGGAS
	}

	void MakeClient() {
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			std::cerr << "WSAStartup failed.\n";
			return;
		}

		SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (serverSocket == INVALID_SOCKET) {
			std::cerr << "Failed to create socket.\n";
			return;
		}

		int PORT = 8080;
		const char* SERVER_IP = "127.0.0.1";

		sockaddr_in server = { 0 };
		server.sin_family = AF_INET;
		server.sin_port = htons(PORT);
		inet_pton(AF_INET, SERVER_IP, &server.sin_addr);

		if (connect(serverSocket, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
			std::cerr << "Failed to connect to the server.\n";
			closesocket(serverSocket);
			return;
		}

		std::cout << "Connected to the server.\n";
		this->server = serverSocket;
	}
};
namespace std {
	template<>
	struct hash<std::tuple<int, int>> {
		size_t operator()(const std::tuple<int, int>& t) const noexcept {
			auto h1 = std::hash<int>{}(std::get<0>(t));
			auto h2 = std::hash<int>{}(std::get<1>(t));
			return h1 ^ (h2 << 1); // simple and safer
		}
	};
}

namespace BitMiner {
	std::unordered_map<std::tuple<int, int>, ChunkPrefab> Chunks;

	ChunkPrefab& get_chunk(int x, int z) {
		auto key = std::make_tuple(x, z);
		if (Chunks.find(key) != Chunks.end()) {
			return Chunks[key];
		}
		else
		{
			std::cout << "Generating chunk at: " << x << ", " << z << std::endl;

			ChunkPrefab newChunk;
			newChunk.xPos = (int)x * newChunk.xSize;
			newChunk.zPos = (int)z * newChunk.zSize;
			newChunk.GenerateChunk();
			Chunks[key] = newChunk;
		}
	}

	int FindSlot(std::vector<Slot>& Inventory, short Type);
	void Input(Vector3& PlayerDirection, bool OnGround, int& InventorySlots, Vector3& PlayerPos);
	void DrawPlayer(SDL_Renderer* Renderer, Vector3 Range, std::vector<Player>& PlayerPos);
	int SetUpRender(SDL_Window** Window, SDL_Renderer** Renderer);
	void Render( RendererParameters Params, std::vector<Slot>& inventory, int inventorySlot, std::vector<Player>& players);
	void PlayerMovement(Player& player, int& inventorySlot);
	void GameLoop(bool& running, GameClient& game);
}

#endif