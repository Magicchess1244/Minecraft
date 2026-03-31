#include "../../include/client/GameManager.hpp"

int main(int argc, char *argv[]) {
  GameManager manager{}; 
  manager.GameLoop();
  return 0;
}