#include "../../include/server/GameServer.hpp"
#include <asio.hpp>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <thread>
#include <utility>

// ─── Constructor / Destructor ────────────────────────────────────────────────

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

  // Close all client sockets
  {
    std::lock_guard<std::recursive_mutex> lock(players_mutex);
    for (auto &[id, sock] : client_sockets) {
      if (sock && sock->is_open()) {
        asio::error_code ec;
        sock->close(ec);
      }
    }
  }

  // Join all threads
  for (auto &thread : client_threads) {
    if (thread.joinable())
      thread.join();
  }

  if (acceptor.is_open()) {
    acceptor.close();
  }

  saveModifications();
  savePlayers();
  std::cout << "Server shutdown cleanly.\n";
}

// ─── Seed ────────────────────────────────────────────────────────────────────

void GameServer::set_seed(unsigned int new_seed) {
  std::cout << "Server seed set to: " << new_seed << std::endl;
  seed = new_seed;
}

unsigned int GameServer::get_seed() const { return seed; }

// ─── Accept Clients Loop ────────────────────────────────────────────────────

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

// ─── Message Handlers ───────────────────────────────────────────────────────

void GameServer::handleLogin(const std::string &message,
                             std::shared_ptr<tcp::socket> socket, int id) {
  std::string name = message.substr(6); // skip "login:"

  std::lock_guard<std::recursive_mutex> lock(players_mutex);

  Player p;
  if (persistentPlayers.count(name)) {
    p = persistentPlayers[name];
    p.id = id;
  } else {
    p.id = id;
    p.name = name;
    p.Position = {0, 64, 0};
    p.Rotation = {0, 0, 0};
    p.color =
        Color::GetColor(static_cast<PlayerColor>(id % (int)PlayerColor::COUNT));
  }
  players[id] = p;

  // Build the response: id, seed, and player data
  std::string msg;
  msg += "id:" + std::to_string(id) + "\n";
  msg += "s:" + std::to_string(seed) + "\n";

  // Position | Rotation | Inventory slots
  msg += "pData:";
  msg += std::to_string(p.Position.x) + "/" + std::to_string(p.Position.y) +
         "/" + std::to_string(p.Position.z) + "|";
  msg += std::to_string(p.Rotation.x) + "/" + std::to_string(p.Rotation.y) +
         "/" + std::to_string(p.Rotation.z) + "|";
  for (const auto &slot : p.inventory) {
    msg += std::to_string(slot.Type) + "," + std::to_string(slot.Amount) + "," +
           (slot.isEntity ? "1" : "0") + "|";
  }
  msg += "\n";

  asio::error_code ec;
  asio::write(*socket, asio::buffer(msg), ec);
  broadcastPlayers();
}

void GameServer::handleSeed(std::shared_ptr<tcp::socket> socket) {
  std::string s = "s:" + std::to_string(seed) + "\n";
  asio::error_code ec;
  asio::write(*socket, asio::buffer(s), ec);
}

void GameServer::handleGetColor(std::shared_ptr<tcp::socket> socket) {
  std::string color = "c:255,0,0\n";
  asio::error_code ec;
  asio::write(*socket, asio::buffer(color), ec);
}

void GameServer::handleUpdate(const std::string &message, int id) {
  // Format: up:<id>:<x/y/z>:<rx/ry/rz>
  auto parts = split(message, ':');
  if (parts.size() < 4)
    return;

  try {
    auto pos = split(parts[2], '/');
    auto rot = split(parts[3], '/');

    if (pos.size() != 3 || rot.size() != 3)
      return;

    float x = std::stof(pos[0]), y = std::stof(pos[1]), z = std::stof(pos[2]);
    float rx = std::stof(rot[0]), ry = std::stof(rot[1]),
          rz = std::stof(rot[2]);

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

  } catch (...) {
    std::cerr << "Error parsing 'up' message: " << message << std::endl;
  }
}

void GameServer::handleInventory(const std::string &message, int id) {
  // Format: inv:type,amount,isEntity|type,amount,isEntity|...
  std::string inv_str = message.substr(4); // skip "inv:"
  auto slots = split(inv_str, '|');

  std::lock_guard<std::recursive_mutex> lock(players_mutex);
  if (!players.count(id))
    return;

  int slotIdx = 0;
  for (const auto &slot_str : slots) {
    if (slot_str.empty() || slotIdx >= (int)players[id].inventory.size())
      break;

    auto fields = split(slot_str, ',');
    if (fields.size() >= 3) {
      players[id].inventory[slotIdx].Type =
          static_cast<short>(std::stoi(fields[0]));
      players[id].inventory[slotIdx].Amount =
          static_cast<short>(std::stoi(fields[1]));
      players[id].inventory[slotIdx].isEntity = (fields[2] == "1");
    }
    slotIdx++;
  }
  persistentPlayers[players[id].name] = players[id];
}

void GameServer::handleModification(const std::string &message) {
  // Format: mod:x/y/z:type
  auto parts = split(message, ':');
  if (parts.size() < 3)
    return;

  try {
    auto pos = split(parts[1], '/');
    if (pos.size() != 3)
      return;

    float x = std::round(std::stof(pos[0]));
    float y = std::round(std::stof(pos[1]));
    float z = std::round(std::stof(pos[2]));
    Uint8 type = static_cast<Uint8>(std::stoi(parts[2]));
    Vector3 position = {x, y, z};

    {
      std::lock_guard<std::recursive_mutex> lock(players_mutex);
      Modifications[position] = type;
    }
    saveModifications();
    broadcastModification(position, type);

  } catch (...) {
    std::cerr << "Error parsing 'mod' message: " << message << std::endl;
  }
}

// ─── Main Client Handler ────────────────────────────────────────────────────

void GameServer::handlePlayers(std::shared_ptr<tcp::socket> socket, int id) {
  try {
    asio::streambuf buffer;
    while (running && socket->is_open()) {
      asio::error_code error;
      asio::read_until(*socket, buffer, '\n', error);

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

      if (message.empty())
        continue;

      // Strip trailing CR (handle CRLF)
      if (message.back() == '\r')
        message.pop_back();

      // Dispatch to the appropriate handler
      if (message.find("login:") == 0) {
        handleLogin(message, socket, id);
      } else if (message.find("seed") != std::string::npos) {
        handleSeed(socket);
      } else if (message.find("getColor") != std::string::npos) {
        handleGetColor(socket);
      } else if (message.find("up:") == 0) {
        handleUpdate(message, id);
      } else if (message.find("inv:") == 0) {
        handleInventory(message, id);
      } else if (message.find("mod:") == 0) {
        handleModification(message);
      } else if (message.find("gm") == 0) {
        sendAllModifications(socket);
      }
    }
  } catch (std::exception &e) {
    std::cerr << "Client handler exception (Player " << id << "): " << e.what()
              << std::endl;
  }

  // Cleanup: persist player data and remove from active lists
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

// ─── Broadcasting ───────────────────────────────────────────────────────────

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

// ─── Persistence: Modifications ─────────────────────────────────────────────

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

// ─── Persistence: Players ───────────────────────────────────────────────────

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