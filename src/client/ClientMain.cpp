#include "../../include/client/GameClient.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  std::string ip;
  std::cout << "Enter server IP address (default 127.0.0.1): ";
  std::getline(std::cin, ip);

  if (ip.empty()) {
    ip = "127.0.0.1";
  }

  GameClient client(ip);

  if (client.GetRunning()) {
    BitMiner::GameLoop(client);
  } else {
    std::cout << "Could not connect to server. Exiting." << std::endl;
  }

  return 0;
}