#include "../../include/server/GameServer.hpp"
#include "../../include/common/Chunck.hpp"
#include "../../include/common/Logger.hpp"
#include "../../include/common/PerlinNoise.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

void GameServer::MakeServer() {
  this->listener = socket(AF_INET, SOCK_STREAM, 0);
  if (this->listener == -1) {
    std::string error =
        "Socket creation failed: " + std::string(strerror(errno));
    LOG_ERROR(error);
    throw std::runtime_error(error);
  }
  LOG_DEBUG("Socket created successfully");

  int opt = 1;
  if (setsockopt(this->listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
      -1) {
    LOG_WARN(std::string("setsockopt failed: ") + strerror(errno));
  }

  constexpr int PORT = 8080;
  sockaddr_in serverAddr{};
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(PORT);

  if (bind(this->listener, (sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
    std::string error = "Bind failed: " + std::string(strerror(errno));
    LOG_ERROR(error);
    close(this->listener);
    throw std::runtime_error(error);
  }
  LOG_DEBUG("Socket bound successfully");

  if (listen(this->listener, SOMAXCONN) == -1) {
    std::string error = "Listen failed: " + std::string(strerror(errno));
    LOG_ERROR(error);
    close(this->listener);
    throw std::runtime_error(error);
  }

  LOG_INFO(std::string("Server listening on port ") + std::to_string(PORT) +
           "...");
}

void GameServer::handlePlayers(int player, int Id) {
  LOG_INFO(std::string("Started handling client socket: ") +
           std::to_string(player) + " (Player ID: " + std::to_string(Id) + ")");

  char buf[256] = {}; // Increased buffer size
  int res = 0;

  while (true) {
    memset(buf, 0, sizeof(buf));
    res = recv(player, buf, sizeof(buf) - 1, 0);

    if (res <= 0) {
      if (res == 0) {
        LOG_INFO(std::string("Client disconnected gracefully (socket: ") +
                 std::to_string(player) + ")");
      } else {
        LOG_WARN(std::string("Client connection error (socket: ") +
                 std::to_string(player) + "): " + strerror(errno));
      }
      close(player);
      LOG_DEBUG(std::string("Closed socket: ") + std::to_string(player));
      return;
    }

    LOG_DEBUG(std::string("Received from client ") + std::to_string(player) +
              ": " + std::string(buf, res));

    if (strncmp(buf, "seed", 4) == 0) {
      LOG_DEBUG("Client requested seed");
      std::string seed_str = std::to_string(this->seed);
      send(player, seed_str.c_str(), seed_str.size(), 0);
      LOG_DEBUG(std::string("Sent seed: ") + seed_str);
    } else if (strncmp(buf, "getColor", 8) == 0) {
      LOG_DEBUG("Client requested color");
      std::string color_str = std::to_string(this->players[Id].color.r) + "," +
                              std::to_string(this->players[Id].color.g) + "," +
                              std::to_string(this->players[Id].color.b);
      send(player, color_str.c_str(), color_str.size(), 0);
      LOG_DEBUG(std::string("Sent color: ") + color_str);
    } else if (strncmp(buf, "getChunk", 8) == 0) {
      int x, z;
      if (sscanf(buf, "getChunk %d %d", &x, &z) == 2) {
        LOG_DEBUG(std::string("Client requested chunk (") + std::to_string(x) +
                  ", " + std::to_string(z) + ")");
        // Ensure seed is set for generation
        SetSeed(this->seed);

        Vector3 key = {(float)x, 0, (float)z};
        ChunkPrefab &chunk = this->chunkManager.get_chunk(key);

        // Serialize
        size_t dataSize = chunk.Blocks.size() * sizeof(int);
        std::string header = "CHUNK " + std::to_string(x) + " " +
                             std::to_string(z) + " " +
                             std::to_string(dataSize) + "\n";

        // Send Header
        send(player, header.c_str(), header.size(), 0);

        // Wait for client to be ready? Or just blast it?
        // TCP stream, so just blast it. But maybe a small sleep or ack is safer
        // if we had a complex protocol. For now, just send the data.

        send(player, (const char *)chunk.Blocks.data(), dataSize, 0);
        LOG_DEBUG(std::string("Sent chunk ") + std::to_string(x) + ", " +
                  std::to_string(z) + " (" + std::to_string(dataSize) +
                  " bytes)");
      } else {
        LOG_WARN(std::string("Invalid chunk request format: ") + buf);
      }
    } else {
      LOG_WARN(std::string("Unknown message from client: ") + buf);
      std::string hex;
      for (int i = 0; i < res; i++) {
        char hexBuf[4];
        snprintf(hexBuf, sizeof(hexBuf), "%02X ", (unsigned char)buf[i]);
        hex += hexBuf;
      }
      LOG_WARN(std::string("Hex: ") + hex);
    }
  }
}

void GameServer::AcceptClients() {
  std::unordered_map<int, Color> PlayerColors = {
      {0, {255, 0, 0}},     {1, {0, 255, 0}},   {2, {0, 0, 255}},
      {3, {255, 255, 0}},   {4, {255, 0, 255}}, {5, {0, 255, 255}},
      {6, {128, 128, 128}}, {7, {255, 165, 0}}};
  this->MakeServer();
  LOG_DEBUG(std::string("Listener socket: ") + std::to_string(listener));

  // Pre-generate chunks
  int radius = 10;
  LOG_INFO("Pre-generating chunks...");
  for (int x = -radius; x <= radius; x++) {
    for (int z = -radius; z <= radius; z++) {
      Vector3 key = {(float)x, 0, (float)z};
      this->chunkManager.get_chunk(key);
    }
  }
  LOG_INFO("Chunk pre-generation complete.");

  while (player_count < MAX_PLAYERS) {
    LOG_DEBUG("Waiting for client connections...");
    int clientSocket = accept(this->listener, NULL, NULL);

    if (clientSocket == -1) {
      LOG_ERROR(std::string("accept() failed: ") + strerror(errno));
      continue;
    }

    this->add_socket(clientSocket, Player{Vector3{0, 24, 0}, Vector3{0, 0, 0},
                                          PlayerColors[player_count]});

    LOG_INFO(std::string("Client connected! Socket: ") +
             std::to_string(clientSocket) +
             " | Total clients: " + std::to_string(player_count));

    // Spawn thread to handle this client
    std::thread(&GameServer::handlePlayers, this, clientSocket,
                this->player_count - 1)
        .detach();
    LOG_DEBUG(std::string("Spawned handler thread for socket: ") +
              std::to_string(clientSocket));
  }

  LOG_INFO("Max players reached, no longer accepting connections.");
}