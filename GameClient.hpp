#ifndef __GAME_CLIENT_H
#define __GAME_CLIENT_H

#include "Renderer.hpp"
#include "common.hpp"

typedef enum { GetSeed, GetColor } Commands;

class GameClient {
   private:
    unsigned int seed;
    unsigned int player_count = 0;
    std::vector<Player> players;
    sock_t server;
    bool running = true;

   public:
    GameClient() : seed(0), player_count(0), server(INVALID_SOCKET) {}
    ~GameClient() {
        std::cout << "closing conn" << std::endl;
        CLOSESOCKET(server);
        cleanupSockets();
    }

    void set_seed();
    unsigned int get_seed() const { return seed; }

    sock_t get_socket() const { return server; }

    void add_player(const Player& player) {
        if (players.size() < MAX_PLAYERS) {
            this->players.push_back(player);
            this->player_count++;
        }
        std::cout << "players size: " << players.size() << std::endl;
    }
    const std::vector<Player> get_players() { return this->players; }

    void set_color();
    Color get_color() const {
        if (players.size() > 0) {
            return players[0].color;
        }

        return {0, 0, 0};
    }

    void MakeClient() {
        if (initSockets() != 0) {
            std::cerr << "Failed to initialize sockets: " << GET_LAST_ERROR << "\n";
            return;
        }

        sock_t serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket: " << GET_LAST_ERROR << "\n";
            cleanupSockets();
            return;
        }

        int PORT = 8080;
        const char* SERVER_IP = "127.0.0.1";

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

        if (connect(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Failed to connect to the server.:" << GET_LAST_ERROR << "\n";
            CLOSESOCKET(serverSocket);
            return;
        }

        std::cout << "Connected to the server.\n";
        this->server = serverSocket;
    }

    bool GetRunning() const { return running; }
    void Quit() { running = false; }
};

namespace BitMiner {
int FindSlot(std::vector<Slot>& Inventory, short Type);
void PlayerInput(Vector3& PlayerDirection, bool OnGround, int& InventorySlots, Vector3& PlayerRot);
void PlayerMovement(Player& player, int& inventorySlot);
void GameLoop(GameClient& game);
}  // namespace BitMiner

#endif
