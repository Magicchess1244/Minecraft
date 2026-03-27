#include "../../include/client/GameClient.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
    return 1;
  }

  



  SDL_Quit();
  return 0;
}