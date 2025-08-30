#include <atomic>
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

#include "client/GameClient.hpp"

std::atomic<bool> keep_running{true};
GameClient* global_client = nullptr;

void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down..." << std::endl;
    keep_running = false;
    if (global_client) {
        global_client->Quit();
    }
}

int main() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    srand(static_cast<unsigned int>(time(0)));
    std::cout << "Starting game client..." << std::endl;

    GameClient client;
    global_client = &client;

    std::cout << "Client running. Press Ctrl+C to exit..." << std::endl;

    while (keep_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "Client has been shut down..." << std::endl;

    return 0;
}
