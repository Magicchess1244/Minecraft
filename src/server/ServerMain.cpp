#include "../../include/server/GameServer.hpp"

int main(int argc, char *argv[]) {
  GameServer server;
  server.AcceptClients();

  return 0;
}