#include "GameServer.hpp"

#include <iostream>

const std::unordered_map<int, Color> GameServer::PLAYER_COLORS = {
    {0, {255, 0, 0}},      // Red
    {1, {0, 255, 0}},      // Green
    {2, {0, 0, 255}},      // Blue
    {3, {255, 255, 0}},    // Yellow
    {4, {255, 0, 255}},    // Magenta
    {5, {0, 255, 255}},    // Cyan
    {6, {128, 128, 128}},  // Gray
    {7, {255, 165, 0}}     // Orange
};

GameServer::GameServer() : listener(AF_INET, SOCK_STREAM, 0) { makeServer(); }

GameServer::~GameServer() { cleanup(); }

void GameServer::makeServer() {
    try {
#ifdef _WIN32
        if (initSockets() != 0) {
            throw std::runtime_error("Failed to initialize sockets: " +
                                     std::to_string(GET_LAST_ERROR));
        }
#endif
        listener.setReuseAddr();
#ifdef SO_REUSEPORT
        listener.setReusePort();
#endif

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(8080);

        if (::bind(listener.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            throw std::runtime_error("bind failed: " + std::to_string(getLastError()));
        }

        if (::listen(listener.get(), 10) < 0) {
            throw std::runtime_error("listen failed: " + std::to_string(getLastError()));
        }

        std::cout << "Server listening on port 8080...\n";

        // Start accepting clients in a separate thread
        std::thread(&GameServer::acceptClients, this).detach();

    } catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        cleanup();
        throw;
    }
}

void GameServer::acceptClients() {
    while (running && player_count < MAX_PLAYERS) {
        try {
            sock_t rawSock = ::accept(listener.get(), nullptr, nullptr);
            if (rawSock == INVALID_SOCKET) {
                if (running) {
                    std::cerr << "accept() failed: " << getLastError() << "\n";
                }
                continue;
            }

            auto client = std::make_unique<Socket>(rawSock);

            {
                std::lock_guard<std::mutex> lock(clients_mutex);
                int client_id = player_count.load();

                // Add client and player
                clients.push_back(std::move(client));

                Color player_color = PLAYER_COLORS.count(client_id) ? PLAYER_COLORS.at(client_id)
                                                                    : Color(255, 255, 255);
                players.emplace_back(Vector3{0, 24, 0}, Vector3{0, 0, 0}, player_color);

                // Start client handling thread
                client_threads.emplace_back(&GameServer::handleClient, this, client_id);

                player_count++;
                std::cout << "Client " << client_id << " connected. Total players: " << player_count
                          << "\n";
            }

        } catch (const std::exception& ex) {
            std::cerr << "Error in accept loop: " << ex.what() << "\n";
        }
    }
}

void GameServer::handleClient(int client_id) {
    try {
        std::lock_guard<std::mutex> lock(clients_mutex);
        if (client_id < 0 || static_cast<size_t>(client_id) >= clients.size() ||
            !clients[client_id]) {
            std::cerr << "Invalid client ID: " << client_id << "\n";
            return;
        }

        const auto& client_socket = *clients[client_id];

        while (running && players[client_id].connected) {
            std::string message = receiveMessage(client_socket);

            if (message.empty()) {
                std::cerr << "Client " << client_id << " disconnected\n";
                players[client_id].connected = false;
                break;
            }

            std::cout << "Client " << client_id << " sent: " << message << "\n";

            if (message == "seed") {
                std::string response = std::to_string(seed.load());
                if (!sendMessage(client_socket, response)) {
                    std::cerr << "Failed to send seed to client " << client_id << "\n";
                    break;
                }
            } else if (message == "getColor") {
                const auto& color = players[client_id].color;
                std::string response = std::to_string(color.r) + "," + std::to_string(color.g) +
                                       "," + std::to_string(color.b);
                if (!sendMessage(client_socket, response)) {
                    std::cerr << "Failed to send color to client " << client_id << "\n";
                    break;
                }
                std::cout << "Sent color to client " << client_id << ": " << response << "\n";
            } else {
                std::cout << "Unknown command from client " << client_id << ": " << message << "\n";
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error handling client " << client_id << ": " << ex.what() << "\n";
    }
}

bool GameServer::sendMessage(const Socket& client_socket, const std::string& message) {
    int bytes_sent = ::send(client_socket.get(), message.c_str(), message.length(), 0);
    return bytes_sent == static_cast<int>(message.length());
}

std::string GameServer::receiveMessage(const Socket& client_socket) {
    char buffer[256] = {};
    int bytes_received = ::recv(client_socket.get(), buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
        return "";
    }

    buffer[bytes_received] = '\0';
    return std::string(buffer);
}

void GameServer::setSeed(unsigned int new_seed) {
    seed = new_seed;
    std::cout << "Seed set to: " << new_seed << "\n";
}

unsigned int GameServer::getSeed() const { return seed.load(); }

void GameServer::stop() {
    running = false;
    listener.close();
}

void GameServer::waitForCompletion() {
    for (auto& thread : client_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void GameServer::cleanup() {
    stop();
    waitForCompletion();

    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.clear();
    players.clear();

#ifdef _WIN32
    cleanupSockets();
#endif
}
