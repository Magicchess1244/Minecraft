#pragma once

#include "../common/ChunkManager.hpp"
#include "../common/Common.hpp"
#include <unistd.h>

class GameServer {
private:
  ChunkManager chunkManager;
  unsigned int seed;
  unsigned int player_count = 0;
  int listener;
  std::vector<int> sockets;
  std::vector<Player> players;
  void MakeServer();

public:
  GameServer() : seed(0), player_count(0), listener(0) {}
  void set_seed(unsigned int new_seed) {
    std::cout << "seed: " << new_seed << std::endl;
    seed = new_seed;
  }

  unsigned int get_seed() const { return seed; }

  void add_socket(int socket, Player player) {
    if (sockets.size() < MAX_PLAYERS) {
      sockets.push_back(socket);
      players.push_back(player);
      player_count++;
    }
  }
  const std::vector<int> &get_sockets() const { return sockets; }

  void handlePlayers(int, int);
  void AcceptClients();

  ~GameServer() {
    for (int socket : sockets) {
      close(socket);
    }
    if (listener > 0)
      close(listener);
    std::cout << "Server closed.\n";
  }
};
