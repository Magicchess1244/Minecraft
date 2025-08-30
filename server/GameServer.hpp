#pragma once

#include "../core/common.hpp"

class GameServer {
   private:
    unsigned int seed;
    unsigned int player_count = 0;
    sock_t listener;
    std::vector<sock_t> sockets;
    std::vector<Player> players;

    void MakeServer() {
        if (initSockets() != 0) {
            std::cerr << "Failed to initialize sockets: " << GET_LAST_ERROR << "\n";
            return;
        }

        sock_t hostSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (hostSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket: " << GET_LAST_ERROR << "\n";
            cleanupSockets();
            return;
        }

        int opt = 1;
        if (setsockopt(hostSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
            std::cerr << "Failed to set socket options: " << GET_LAST_ERROR << "\n";
            CLOSESOCKET(hostSocket);
            cleanupSockets();
            return;
        }

        constexpr int PORT = 8080;
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(PORT);

        if (bind(hostSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed: " << GET_LAST_ERROR << "\n";
            CLOSESOCKET(hostSocket);
            cleanupSockets();
            return;
        }

        if (listen(hostSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed: " << GET_LAST_ERROR << "\n";
            CLOSESOCKET(hostSocket);
            cleanupSockets();
            return;
        }

        std::cout << "Server listening on port " << PORT << "...\n";
        this->listener = hostSocket;
    }

   public:
    GameServer() : seed(0), player_count(0), listener(INVALID_SOCKET) {}
    void set_seed(unsigned int new_seed) {
        std::cout << "seed: " << new_seed << std::endl;
        seed = new_seed;
    }

    unsigned int get_seed() const { return seed; }

    void add_socket(sock_t socket, Player player) {
        if (sockets.size() < MAX_PLAYERS) {
            sockets.push_back(socket);
            players.push_back(player);
            player_count++;
        }
    }
    const std::vector<sock_t>& get_sockets() const { return sockets; }

    void handlePlayers(sock_t player, int Id);

    void AcceptClients();

    ~GameServer() {
        for (sock_t socket : sockets) {
            CLOSESOCKET(socket);
        }
        cleanupSockets();
        std::cout << "2. Server closed.\n";
    }
};
