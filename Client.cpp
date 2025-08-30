#include "client/GameClient.hpp"

int main() {
    GameClient client;
    std::cout << "Starting game client..." << std::endl;
    BitMiner::GameLoop(client);
    std::cout << "Exiting game client..." << std::endl;
    return 0;
}
