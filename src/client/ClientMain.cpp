#include "../../include/client/GameClient.hpp"
#include <iostream>
#include <cstdlib>
#include <ctime>

int main() {
    srand(static_cast<unsigned int>(time(0)));

    std::cout << "Starting Minecraft3D Client..." << std::endl;
    
    GameClient client;
    
    // Enable standalone mode by default
    client.enable_standalone_mode();
    std::cout << "Running in standalone mode (no networking)" << std::endl;
    
    // Start the game loop
    BitMiner::GameLoop(client);

    std::cout << "Client exiting..." << std::endl;
    return 0;
}
