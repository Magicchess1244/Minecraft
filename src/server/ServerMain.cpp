#include "../../include/server/GameServer.hpp"

int main(int argc, char *argv[]) {
  GameServer server;
  if (argc > 1) {
    server.set_seed(std::stoul(argv[1]));
  }
  server.AcceptClients();

  return 0;
}