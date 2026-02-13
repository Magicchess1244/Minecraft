//
#include "../../include/client/GameClient.hpp"

int main(int argc, char *argv[]) {

  GameClient client;

  BitMiner::GameLoop(client);

  return 0;
}