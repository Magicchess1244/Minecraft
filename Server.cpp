#include "server/GameServer.hpp"

int main() {
    srand(static_cast<unsigned int>(time(0)));
    GameServer server;
    std::cout << "Starting game server..." << std::endl;
    server.set_seed(rand());
    server.AcceptClients();
    std::cout << "Exiting game server..." << std::endl;
    return 0;
}
