#ifndef __GAME_CLIENT_H
#define __GAME_CLIENT_H

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "../core/common.hpp"
#include "../net/common.hpp"
#include "Renderer.hpp"

typedef enum { GetSeed, GetColor } Commands;

class GameClient {
   private:
    std::atomic<unsigned int> seed{0};
    std::atomic<unsigned int> player_count{0};
    std::vector<Player> players;
    std::unique_ptr<Socket> server_socket;
    std::atomic<bool> running{true};
    mutable std::mutex players_mutex;

    static constexpr int DEFAULT_PORT = 8080;
    static constexpr const char* DEFAULT_SERVER_IP = "127.0.0.1";

    bool sendCommand(const std::string& command);
    std::string receiveResponse();
    void GameLoop();

   public:
    GameClient();
    ~GameClient();

    bool makeClient(const std::string& server_ip = DEFAULT_SERVER_IP, int port = DEFAULT_PORT);
    void disconnect();
    bool isConnected() const;

    void set_seed();
    unsigned int get_seed() const { return seed.load(); }

    sock_t get_socket() const { return server_socket ? server_socket->get() : INVALID_SOCKET; }

    void add_player(const Player& player);
    const std::vector<Player> get_players() const;

    void set_color();
    Color get_color() const;

    bool GetRunning() const { return running.load(); }
    void Quit() { running = false; }
};

std::vector<std::string> split(const std::string& s, const std::string& delimiter);

namespace BitMiner {
int FindSlot(const std::vector<Slot>& Inventory, short Type);
void PlayerInput(Vector3& PlayerDirection, bool OnGround, int& InventorySlots, Vector3& PlayerRot);
void PlayerMovement(Player& player, int& inventorySlot);
}  // namespace BitMiner

#endif  // __GAME_CLIENT_H
