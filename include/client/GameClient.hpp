#pragma once

#include "Renderer.hpp"
#include "../common/common.hpp"
#include "../common/NetworkProtocol.hpp"

typedef enum { GetSeed, GetColor } Commands;

class GameClient {
   private:
    unsigned int seed;
    unsigned int player_count = 0;
    std::vector<Player> players;
    std::unique_ptr<NetworkProtocol> network;
    bool running = true;
    bool networked_mode = false;

   public:
    GameClient() : seed(0), player_count(0), networked_mode(false) {}
    ~GameClient() = default;

    void set_seed();
    void set_seed(unsigned int new_seed) { seed = new_seed; }
    unsigned int get_seed() const { return seed; }

    // Standalone mode methods
    void enable_standalone_mode() { networked_mode = false; network.reset(); }
    void enable_networked_mode() { networked_mode = true; }
    bool is_networked() const { return networked_mode; }

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

    bool connect_to_server(const std::string& host = "127.0.0.1", int port = 8080) {
        network = std::make_unique<NetworkProtocol>();
        if (network->connect_to_server(host, port)) {
            networked_mode = true;
            return true;
        }
        network.reset();
        return false;
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

