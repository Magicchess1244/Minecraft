#include "../../include/server/GameServer.hpp"
#include <asio.hpp>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>
#include <utility>

GameServer::GameServer()
    : acceptor(io, tcp::endpoint(tcp::v4(), PORT)), seed(0), running(true) {
  std::srand(static_cast<unsigned int>(std::time(0)));
  seed = rand();

  std::cout << "Server listening on port " << PORT << " with seed " << seed
            << "...\n";
}

GameServer::~GameServer() {
  running = false;
  {
    std::lock_guard<std::recursive_mutex> lock(players_mutex);
    for (auto &[id, sock] : client_sockets) {
      if (sock && sock->is_open()) {
        asio::error_code ec;
        sock->close(ec);
      }
    }
  }
  for (auto &thread : client_threads) {
    if (thread.joinable())
      thread.join();
  }
  if (acceptor.is_open()) {
    acceptor.close();
  }
  std::cout << "Server shutdown cleanly.\n";
}

void GameServer::set_seed(unsigned int new_seed) {
  std::cout << "Server seed set to: " << new_seed << std::endl;
  seed = new_seed;
}

void GameServer::AcceptClients() {
  try {
    std::cout << "Waiting for clients..." << std::endl;
    while (true) {
      tcp::socket socket(io);
      acceptor.accept(socket);
      std::cout << "New client connected: " << socket.remote_endpoint()
                << std::endl;

      int id = next_id++;
      auto socket_ptr = std::make_shared<tcp::socket>(std::move(socket));
      {
        std::lock_guard<std::recursive_mutex> lock(players_mutex);
        players[id] = Player{id, {0, 0, 0}, {0, 0, 0}, {255, 0, 0}};
        client_sockets[id] = socket_ptr;
      }

      // Send ID to client
      std::string id_msg = "id:" + std::to_string(id) + "\n";
      asio::error_code error;
      asio::write(*socket_ptr, asio::buffer(id_msg), error);

      client_threads.emplace_back(&GameServer::handlePlayers, this, socket_ptr,
                                  id);
    }
  } catch (std::exception &e) {
    std::cerr << "Exception in AcceptClients: " << e.what() << std::endl;
  }
}

void GameServer::handlePlayers(std::shared_ptr<tcp::socket> socket, int id) {
  try {
    asio::streambuf buffer;
    while (running && socket->is_open()) {
      asio::error_code error;
      size_t n = asio::read_until(*socket, buffer, '\n', error);

      if (error) {
        if (error != asio::error::eof) {
          std::cerr << "Read error (Player " << id << "): " << error.message()
                    << std::endl;
        } else {
          std::cout << "Player " << id << " disconnected." << std::endl;
          next_id--;
        }
        break;
      }

      std::string message;
      std::istream is(&buffer);
      std::getline(is, message);

      if (!message.empty()) {
        if (message.back() == '\r')
          message.pop_back(); // Handle CRLF

        if (message.find("seed") != std::string::npos) {
          std::string s = "s:" + std::to_string(seed) + "\n";
          asio::write(*socket, asio::buffer(s), error);
        } else if (message.find("getColor") != std::string::npos) {
          std::string color = "c:255,0,0\n";
          asio::write(*socket, asio::buffer(color), error);
        } else if (message.find("up:") == 0) {
          size_t first_colon = message.find(':');
          size_t second_colon = message.find(':', first_colon + 1);
          if (first_colon != std::string::npos &&
              second_colon != std::string::npos) {
            try {
              int msg_id = std::stoi(message.substr(
                  first_colon + 1, second_colon - first_colon - 1));
              std::string pos = message.substr(second_colon + 1);

              size_t first_slash = pos.find('/');
              size_t last_slash = pos.find_last_of('/');
              if (first_slash != std::string::npos &&
                  last_slash != std::string::npos &&
                  first_slash != last_slash) {
                float x = std::stof(pos.substr(0, first_slash));
                float y = std::stof(
                    pos.substr(first_slash + 1, last_slash - first_slash - 1));
                float z = std::stof(pos.substr(last_slash + 1));

                std::lock_guard<std::recursive_mutex> lock(players_mutex);
                players[msg_id].Position = {x, y, z};
                broadcastPlayers();
              }
            } catch (...) {
              std::cerr << "Error parsing 'up' message: " << message
                        << std::endl;
            }
          }
        } else if (message.find("mod:") == 0) {
          // Format: mod:x/y/z:type
          size_t first_colon = message.find(':');
          size_t second_colon = message.find(':', first_colon + 1);
          if (first_colon != std::string::npos &&
              second_colon != std::string::npos) {
            try {
              std::string pos_str = message.substr(
                  first_colon + 1, second_colon - first_colon - 1);
              Uint8 type = static_cast<Uint8>(
                  std::stoi(message.substr(second_colon + 1)));

              size_t first_slash = pos_str.find('/');
              size_t last_slash = pos_str.find_last_of('/');
              if (first_slash != std::string::npos &&
                  last_slash != std::string::npos &&
                  first_slash != last_slash) {
                float x = std::stof(pos_str.substr(0, first_slash));
                float y = std::stof(pos_str.substr(
                    first_slash + 1, last_slash - first_slash - 1));
                float z = std::stof(pos_str.substr(last_slash + 1));
                Vector3 pos = {x, y, z};

                {
                  std::lock_guard<std::recursive_mutex> lock(players_mutex);
                  Modifications[pos] = type;
                }
                broadcastModification(pos, type);
              }
            } catch (...) {
              std::cerr << "Error parsing 'mod' message: " << message
                        << std::endl;
            }
          }
        } else if (message.find("gm") == 0) {
          sendAllModifications(socket);
        }
      }
    }
  } catch (std::exception &e) {
    std::cerr << "Client handler exception (Player " << id << "): " << e.what()
              << std::endl;
  }

  {
    std::lock_guard<std::recursive_mutex> lock(players_mutex);
    players.erase(id);
    client_sockets.erase(id);
    broadcastPlayers();
  }

  if (socket->is_open()) {
    socket->close();
  }
}

void GameServer::broadcastPlayers() {
  std::string broadcast_msg = "p:";
  std::vector<std::shared_ptr<tcp::socket>> targets;
  {
    std::lock_guard<std::recursive_mutex> lock(players_mutex);
    for (const auto &[pid, player] : players) {
      broadcast_msg += std::to_string(pid) + ":" +
                       std::to_string(player.Position.x) + "/" +
                       std::to_string(player.Position.y) + "/" +
                       std::to_string(player.Position.z) + "|";
    }
    for (auto &[pid, sock] : client_sockets) {
      if (sock && sock->is_open()) {
        targets.push_back(sock);
      }
    }
  }
  broadcast_msg += "\n";

  for (auto &sock : targets) {
    asio::error_code ec;
    // Use write with a timeout or just accept this might block for a bit,
    // but at least it won't hold the global players_mutex.
    asio::write(*sock, asio::buffer(broadcast_msg), ec);
  }
}

void GameServer::broadcastModification(Vector3 pos, Uint8 type) {
  std::string msg = "bm:" + std::to_string(pos.x) + "/" +
                    std::to_string(pos.y) + "/" + std::to_string(pos.z) + ":" +
                    std::to_string(type) + "\n";
  std::vector<std::shared_ptr<tcp::socket>> targets;
  {
    std::lock_guard<std::recursive_mutex> lock(players_mutex);
    for (auto &[pid, sock] : client_sockets) {
      if (sock && sock->is_open()) {
        targets.push_back(sock);
      }
    }
  }
  for (auto &sock : targets) {
    asio::error_code ec;
    asio::write(*sock, asio::buffer(msg), ec);
  }
}

void GameServer::sendAllModifications(std::shared_ptr<tcp::socket> socket) {
  std::lock_guard<std::recursive_mutex> lock(players_mutex);
  for (auto const &[pos, type] : Modifications) {
    std::string msg = "bm:" + std::to_string(pos.x) + "/" +
                      std::to_string(pos.y) + "/" + std::to_string(pos.z) +
                      ":" + std::to_string(type) + "\n";
    asio::error_code ec;
    asio::write(*socket, asio::buffer(msg), ec);
  }
}