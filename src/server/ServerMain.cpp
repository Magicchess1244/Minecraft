#include "../../include/server/GameServer.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  try {
    GameServer server;
    server.set_seed(12345); // Set a default seed
    std::cout << "Server starting with seed: " << server.get_seed()
              << std::endl;
    server.AcceptClients();
  } catch (const std::exception &e) {
    std::cerr << "Server crashed: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}