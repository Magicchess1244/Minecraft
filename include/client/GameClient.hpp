#ifndef __GAME_CLIENT_H
#define __GAME_CLIENT_H

#include "../common/Common.hpp"
#include "Renderer.hpp"
#include <unistd.h>

typedef enum { GetSeed, GetColor } Commands;

class GameClient {
private:
  unsigned int seed;
  unsigned int player_count = 0;
  std::vector<Player> players;
  // SOCKET server;
  int server = 0;
  bool running = true;

public:
  GameClient() : seed(0), player_count(0) {}
  ~GameClient() { std::cout << "closing conn" << std::endl; }

  void set_seed();
  unsigned int get_seed() const { return seed; }

  void get_socket() const {
    //	return this->server;
  }

  void add_player(Player player) {
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

  void MakeClient();
  void RequestChunk(int x, int z);
  void ReceiveChunks(class ChunkManager &chunkManager);

  bool GetRunning() const { return running; }
  void Quit() { running = false; }
};

namespace BitMiner {
int FindSlot(std::vector<Slot> &Inventory, short Type);
void PlayerInput(Vector3 &PlayerDirection, bool OnGround, int &InventorySlots,
                 Vector3 &PlayerRot);
void PlayerMovement(Player &player, int &inventorySlot);
void GameLoop(GameClient &game);
} // namespace BitMiner

#endif