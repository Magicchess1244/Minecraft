#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include "../core/common.hpp"
#include "../net/common.hpp"

class GameServer {
   private:
    std::atomic<unsigned int> seed{0};
    std::atomic<int> player_count{0};
    Socket listener;
    std::vector<std::unique_ptr<Socket>> clients;
    std::vector<Player> players;
    std::vector<std::thread> client_threads;
    std::mutex clients_mutex;
    std::atomic<bool> running{true};

    static const std::unordered_map<int, Color> PLAYER_COLORS;

    void makeServer();
    void handleClient(int client_id);
    void acceptClients();
    bool sendMessage(const Socket& client_socket, const std::string& message);
    std::string receiveMessage(const Socket& client_socket);
    void cleanup();

   public:
    GameServer();
    ~GameServer();

    void setSeed(unsigned int new_seed);
    unsigned int getSeed() const;
    void stop();
    void waitForCompletion();
};
