#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "server/GameServer.hpp"

std::atomic<bool> keep_running{true};
GameServer* global_server = nullptr;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down..." << std::endl;
    keep_running = false;
    if (global_server) {
        global_server->stop();
    }
}

int main() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    srand(static_cast<unsigned int>(time(0)));
    std::cout << "Starting game server..." << std::endl;

    GameServer server;
    global_server = &server;
    server.setSeed(rand());

    std::cout << "Server running. Press Ctrl+C to exit..." << std::endl;

    while (keep_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    server.waitForCompletion();
    std::cout << "Server has been shut down..." << std::endl;

    return 0;
}
