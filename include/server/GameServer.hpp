#pragma once

#include "../common/common.hpp"
#include "../common/NetworkProtocol.hpp"
#include <thread>
#include <atomic>

class GameServer {
   private:
    unsigned int seed;
    unsigned int player_count = 0;
    SocketRAII listener;
    std::vector<std::unique_ptr<NetworkProtocol>> client_connections;
    std::vector<Player> players;
    std::vector<std::thread> client_threads;
    std::atomic<bool> running{true};

    bool start_server(int port = 8080) {
        if (initSockets() != 0) {
            std::cerr << "Failed to initialize sockets: " << GET_LAST_ERROR << std::endl;
            return false;
        }

        sock_t hostSocket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (hostSocket == INVALID_SOCKET) {
            std::cerr << "Failed to create socket: " << GET_LAST_ERROR << std::endl;
            cleanupSockets();
            return false;
        }

        int opt = 1;
        if (setsockopt(hostSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
            std::cerr << "Failed to set socket options: " << GET_LAST_ERROR << std::endl;
            CLOSESOCKET(hostSocket);
            cleanupSockets();
            return false;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (bind(hostSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed: " << GET_LAST_ERROR << std::endl;
            CLOSESOCKET(hostSocket);
            cleanupSockets();
            return false;
        }

        if (listen(hostSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed: " << GET_LAST_ERROR << std::endl;
            CLOSESOCKET(hostSocket);
            cleanupSockets();
            return false;
        }

        listener.reset(hostSocket);
        std::cout << "Server listening on port " << port << "..." << std::endl;
        return true;
    }

   public:
    GameServer() : seed(0), player_count(0) {}
    
    void set_seed(unsigned int new_seed) {
        std::cout << "seed: " << new_seed << std::endl;
        seed = new_seed;
    }

    unsigned int get_seed() const { return seed; }

    void handle_client(std::unique_ptr<NetworkProtocol> client, int player_id);
    void accept_clients();
    void stop_server();

    ~GameServer() {
        stop_server();
        std::cout << "Server closed." << std::endl;
    }
};

