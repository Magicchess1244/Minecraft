#include "GameServer.hpp"
#include <iostream>
#include <cstdlib>
#include <ctime>

int main() {
    srand(static_cast<unsigned int>(time(0)));

    std::cout << "Starting Minecraft3D Server..." << std::endl;
    
    GameServer server;
    server.set_seed(rand());
    server.accept_clients();

    std::cout << "Server exiting..." << std::endl;
    return 0;
}
