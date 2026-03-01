#include "../../include/server/GameServer.hpp"
#include <asio.hpp>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <thread>
#include <utility>

GameServer::GameServer()
    : acceptor(io, tcp::endpoint(tcp::v4(), PORT)), seed(0), running(true) {
  std::srand(static_cast<unsigned int>(std::time(0)));
  seed = rand();

  loadModifications();
  loadPlayers();

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
  saveModifications();
  savePlayers();
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
        client_sockets[id] = socket_ptr;
      }

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
        }
        break;
      }

      std::string message;
      std::istream is(&buffer);
      std::getline(is, message);

      if (!message.empty()) {
        if (message.back() == '\r')
          message.pop_back(); // Handle CRLF

        if (message.find("login:") == 0) {
          std::string name = message.substr(6);
          std::lock_guard<std::recursive_mutex> lock(players_mutex);

          Player p;
          if (persistentPlayers.count(name)) {
            p = persistentPlayers[name];
            p.id = id; // Always use current session ID
          } else {
            p.id = id;
            p.name = name;
            p.Position = {0, 64, 0}; // Default spawn
            p.Rotation = {0, 0, 0};
            p.color = Color::GetColor(
                static_cast<PlayerColor>(id % (int)PlayerColor::COUNT));
          }
          players[id] = p;

          // Send ID and Seed
          std::string msg = "id:" + std::to_string(id) + "\n";
          msg += "s:" + std::to_string(seed) + "\n";

          // Send stored inventory/pos/rot to client if we have it
          msg += "pData:" + std::to_string(p.Position.x) + "/" +
                 std::to_string(p.Position.y) + "/" +
                 std::to_string(p.Position.z) + "|";
          msg += std::to_string(p.Rotation.x) + "/" +
                 std::to_string(p.Rotation.y) + "/" +
                 std::to_string(p.Rotation.z) + "|";
          for (const auto &slot : p.inventory) {
            msg += std::to_string(slot.Type) + "," +
                   std::to_string(slot.Amount) + "," +
                   (slot.isEntity ? "1" : "0") + "|";
          }
          msg += "\n";

          asio::write(*socket, asio::buffer(msg), error);
          broadcastPlayers();
        } else if (message.find("seed") != std::string::npos) {
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
              std::string data = message.substr(second_colon + 1);
              size_t pipe = data.find('|');
              if (pipe != std::string::npos) {
                std::string pos_str = data.substr(0, pipe);
                std::string rot_str = data.substr(pipe + 1);

                size_t f1 = pos_str.find('/'), l1 = pos_str.find_last_of('/');
                size_t f2 = rot_str.find('/'), l2 = rot_str.find_last_of('/');

                if (f1 != std::string::npos && l1 != std::string::npos &&
                    f1 != l1 && f2 != std::string::npos &&
                    l2 != std::string::npos && f2 != l2) {
                  float x = std::stof(pos_str.substr(0, f1));
                  float y = std::stof(pos_str.substr(f1 + 1, l1 - f1 - 1));
                  float z = std::stof(pos_str.substr(l1 + 1));

                  float rx = std::stof(rot_str.substr(0, f2));
                  float ry = std::stof(rot_str.substr(f2 + 1, l2 - f2 - 1));
                  float rz = std::stof(rot_str.substr(l2 + 1));

                  std::lock_guard<std::recursive_mutex> lock(players_mutex);
                  if (players.count(id)) {
                    players[id].Position = {x, y, z};
                    players[id].Rotation = {rx, ry, rz};
                    persistentPlayers[players[id].name] = players[id];
                  }
                  broadcastPlayers();

                  static int saveCounter = 0;
                  if (++saveCounter % 100 == 0)
                    savePlayers();
                }
              }
            } catch (...) {
              std::cerr << "Error parsing 'up' message: " << message
                        << std::endl;
            }
          }
        } else if (message.find("inv:") == 0) {
          std::string inv_str = message.substr(4);
          std::lock_guard<std::recursive_mutex> lock(players_mutex);
          if (players.count(id)) {
            size_t start = 0;
            size_t end = inv_str.find('|');
            int slotIdx = 0;
            while (end != std::string::npos &&
                   slotIdx < (int)players[id].inventory.size()) {
              std::string slot_info = inv_str.substr(start, end - start);
              size_t comma = slot_info.find(',');
              if (comma != std::string::npos) {
                size_t second_comma = slot_info.find(',', comma + 1);
                if (second_comma != std::string::npos) {
                  players[id].inventory[slotIdx].Type =
                      static_cast<short>(std::stoi(slot_info.substr(0, comma)));
                  players[id].inventory[slotIdx].Amount =
                      static_cast<short>(std::stoi(slot_info.substr(
                          comma + 1, second_comma - comma - 1)));
                  players[id].inventory[slotIdx].isEntity =
                      (slot_info.substr(second_comma + 1) == "1");
                }
              }
              start = end + 1;
              end = inv_str.find('|', start);
              slotIdx++;
            }
            persistentPlayers[players[id].name] = players[id];
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
                float x = std::round(std::stof(pos_str.substr(0, first_slash)));
                float y = std::round(std::stof(pos_str.substr(
                    first_slash + 1, last_slash - first_slash - 1)));
                float z = std::round(std::stof(pos_str.substr(last_slash + 1)));
                Vector3 pos = {x, y, z};

                {
                  std::lock_guard<std::recursive_mutex> lock(players_mutex);
                  Modifications[pos] = type;
                }
                saveModifications();
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
    if (players.count(id)) {
      persistentPlayers[players[id].name] = players[id];
      savePlayers();
    }
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
  std::string msg =
      "bm:" + std::to_string((int)pos.x) + "/" + std::to_string((int)pos.y) +
      "/" + std::to_string((int)pos.z) + ":" + std::to_string(type) + "\n";
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
    std::string msg =
        "bm:" + std::to_string((int)pos.x) + "/" + std::to_string((int)pos.y) +
        "/" + std::to_string((int)pos.z) + ":" + std::to_string(type) + "\n";
    asio::error_code ec;
    asio::write(*socket, asio::buffer(msg), ec);
  }
}

void GameServer::saveModifications() {
  std::ofstream ofs("modifications.dat", std::ios::binary);
  if (!ofs) {
    std::cerr << "Could not open modifications.dat for writing" << std::endl;
    return;
  }

  std::lock_guard<std::recursive_mutex> lock(players_mutex);
  size_t size = Modifications.size();
  ofs.write(reinterpret_cast<const char *>(&size), sizeof(size));
  for (const auto &[pos, type] : Modifications) {
    ofs.write(reinterpret_cast<const char *>(&pos), sizeof(pos));
    ofs.write(reinterpret_cast<const char *>(&type), sizeof(type));
  }
}

void GameServer::loadModifications() {
  std::ifstream ifs("modifications.dat", std::ios::binary);
  if (!ifs) {
    std::cout
        << "No modifications.dat found, starting with empty modifications."
        << std::endl;
    return;
  }

  size_t size;
  ifs.read(reinterpret_cast<char *>(&size), sizeof(size));
  for (size_t i = 0; i < size; ++i) {
    Vector3 pos;
    Uint8 type;
    ifs.read(reinterpret_cast<char *>(&pos), sizeof(pos));
    ifs.read(reinterpret_cast<char *>(&type), sizeof(type));
    Modifications[pos] = type;
  }
  std::cout << "Loaded " << Modifications.size() << " modifications from disk."
            << std::endl;
}

void GameServer::savePlayers() {
  std::ofstream ofs("players.dat", std::ios::binary);
  if (!ofs)
    return;

  std::lock_guard<std::recursive_mutex> lock(players_mutex);
  size_t count = persistentPlayers.size();
  ofs.write(reinterpret_cast<const char *>(&count), sizeof(count));
  for (auto const &[name, p] : persistentPlayers) {
    size_t nameLen = name.length();
    ofs.write(reinterpret_cast<const char *>(&nameLen), sizeof(nameLen));
    ofs.write(name.data(), nameLen);
    ofs.write(reinterpret_cast<const char *>(&p.Position), sizeof(p.Position));
    ofs.write(reinterpret_cast<const char *>(&p.Rotation), sizeof(p.Rotation));
    ofs.write(reinterpret_cast<const char *>(&p.color), sizeof(p.color));
    size_t invSize = p.inventory.size();
    ofs.write(reinterpret_cast<const char *>(&invSize), sizeof(invSize));
    ofs.write(reinterpret_cast<const char *>(p.inventory.data()),
              invSize * sizeof(Slot));
  }
}

void GameServer::loadPlayers() {
  std::ifstream ifs("players.dat", std::ios::binary);
  if (!ifs)
    return;

  size_t count;
  ifs.read(reinterpret_cast<char *>(&count), sizeof(count));
  for (size_t i = 0; i < count; ++i) {
    size_t nameLen;
    ifs.read(reinterpret_cast<char *>(&nameLen), sizeof(nameLen));
    std::string name(nameLen, ' ');
    ifs.read(&name[0], nameLen);
    Player p;
    p.name = name;
    ifs.read(reinterpret_cast<char *>(&p.Position), sizeof(p.Position));
    ifs.read(reinterpret_cast<char *>(&p.Rotation), sizeof(p.Rotation));
    ifs.read(reinterpret_cast<char *>(&p.color), sizeof(p.color));
    size_t invSize;
    ifs.read(reinterpret_cast<char *>(&invSize), sizeof(invSize));
    p.inventory.resize(invSize);
    ifs.read(reinterpret_cast<char *>(p.inventory.data()),
             invSize * sizeof(Slot));
    persistentPlayers[name] = p;
  }
  std::cout << "Loaded " << persistentPlayers.size() << " persistent players."
            << std::endl;
}