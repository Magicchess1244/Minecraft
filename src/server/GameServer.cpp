#include "GameServer.hpp"

void GameServer::handle_client(std::unique_ptr<NetworkProtocol> client, int player_id) {
    std::cout << "Handling client " << player_id << std::endl;

    while (running && client->is_connected()) {
        NetworkMessage message;
        if (!client->receive_message(message)) {
            std::cout << "Client " << player_id << " disconnected" << std::endl;
            break;
        }

        switch (message.type) {
            case MessageType::SEED_REQUEST: {
                NetworkMessage response(MessageType::SEED_RESPONSE, std::to_string(seed));
                client->send_message(response);
                std::cout << "Sent seed to client " << player_id << ": " << seed << std::endl;
                break;
            }
            case MessageType::COLOR_REQUEST: {
                if (player_id < static_cast<int>(players.size())) {
                    std::string color_str = std::to_string(players[player_id].color.r) + "," +
                                           std::to_string(players[player_id].color.g) + "," +
                                           std::to_string(players[player_id].color.b);
                    NetworkMessage response(MessageType::COLOR_RESPONSE, color_str);
                    client->send_message(response);
                    std::cout << "Sent color to client " << player_id << ": " << color_str << std::endl;
                }
                break;
            }
            case MessageType::PING: {
                NetworkMessage pong(MessageType::PONG, "");
                client->send_message(pong);
                break;
            }
            case MessageType::DISCONNECT: {
                std::cout << "Client " << player_id << " requested disconnect" << std::endl;
                return;
            }
            case MessageType::SEED_RESPONSE:
            case MessageType::COLOR_RESPONSE:
            case MessageType::PLAYER_UPDATE:
            case MessageType::CHUNK_REQUEST:
            case MessageType::CHUNK_DATA:
            case MessageType::BLOCK_UPDATE:
            case MessageType::PONG:
                // These are client-to-server messages, ignore
                break;
            default:
                std::cout << "Unknown message type from client " << player_id << std::endl;
                break;
        }
    }
}

void GameServer::accept_clients() {
    std::unordered_map<int, Color> PlayerColors = {
        {0, {255, 0, 0}},   {1, {0, 255, 0}},   {2, {0, 0, 255}},     {3, {255, 255, 0}},
        {4, {255, 0, 255}}, {5, {0, 255, 255}}, {6, {128, 128, 128}}, {7, {255, 165, 0}}};

    if (!start_server()) {
        std::cerr << "Failed to start server" << std::endl;
        return;
    }

    while (running && player_count < MAX_PLAYERS) {
        sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        sock_t clientSocket = accept(listener.get(), (sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket == INVALID_SOCKET) {
            if (running) {
                std::cerr << "accept() failed with error: " << GET_LAST_ERROR << std::endl;
            }
            continue;
        }

        // Create new client connection
        auto client = std::make_unique<NetworkProtocol>();
        client->socket.reset(clientSocket);
        
        // Add player
        Player newPlayer{Vector3{0, 24, 0}, Vector3{0, 0, 0}, PlayerColors[player_count]};
        players.push_back(newPlayer);
        client_connections.push_back(std::move(client));
        
        // Start client handler thread
        client_threads.emplace_back(&GameServer::handle_client, this, 
                                   std::move(client_connections.back()), player_count);
        
        player_count++;
        std::cout << "Client " << player_count << " connected. Total: " << player_count << std::endl;
    }
}

void GameServer::stop_server() {
    running = false;
    
    // Close listener
    listener.reset();
    
    // Wait for all client threads to finish
    for (auto& thread : client_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // Clear connections
    client_connections.clear();
    client_threads.clear();
    players.clear();
    player_count = 0;
    
    cleanupSockets();
}
