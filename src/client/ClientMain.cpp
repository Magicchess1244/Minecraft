#include "../../include/client/GameClient.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
    return 1;
  }

  std::string ip;
  std::cout << "Enter server IP address (default 127.0.0.1): ";
  std::getline(std::cin, ip);

  if (ip.empty()) {
    ip = "127.0.0.1";
  }

  std::string name;
  std::cout << "Enter player name: ";
  std::getline(std::cin, name);
  if (name.empty()) {
    name = "Player" + std::to_string(rand() % 1000);
  }

  {
    GameClient client(ip, name);

    if (client.GetRunning()) {
      BitMiner::GameLoop(client);
    } else {
      std::cout << "Could not connect to server. Exiting." << std::endl;
    }
  }

  SDL_Quit();
  return 0;
}