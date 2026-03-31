#include "../../include/client/GameClient.hpp"
#include "../../include/common/PerlinNoise.hpp"
#include <SDL3/SDL_stdinc.h>
#include <algorithm>
#include <ostream>
#include <string>
#include <utility>
#include <mutex>
#include <iostream>
#include <asio/ip/address_v4.hpp>
// ─── Constants ──────────────────────────────────────────────────────────────



std::pair<std::string,std::string> GetCredentials(){
  std::string ip;
  std::cout << "Enter server IP address (default 127.0.0.1): ";
  std::getline(std::cin, ip);

  if (ip.empty()) {
    ip = "127.0.0.1";
  }

  std::string name;
  std::cout << "Enter player name: ";
  std::getline(std::cin, name);
  if (name.empty()) {
    name = "Player" + std::to_string(rand() % 1000);
  }
  return std::pair(ip, name);
}
GameClient::GameClient() : socket(io) {
  std::pair<std::string, std::string> credential = GetCredentials();
  tcp::endpoint endpoint(asio::ip::make_address(credential.first), PORT);
  this->socket.connect(endpoint);
  PrintLog("Connected to server at " + credential.first + ":" + std::to_string(PORT));

  // Send login command first
  sendCommand("login:" + credential.second);

  // Receive ID and Seed
  std::string id_str = receiveMessage();
  if (id_str.find("id:") == 0) {
    my_id = std::stoi(id_str.substr(3));
    PrintLog("Assigned ID: " + std::to_string(my_id) + " (Name: " + credential.second + ")");
  }
  std::string seed_str = receiveMessage();
  if (seed_str.find("s:") == 0) {
    unsigned int s =
        static_cast<unsigned int>(std::stoul(seed_str.substr(2)));
    SetSeed(s);
  }
  
  this->set_color();
  this->sendCommand("gm");
  this->StartListener();
}

void GameClient::set_color() { this->sendCommand("getColor"); }

// ─── Helper: parse "x/y/z" into a Vector3 ──────────────────────────────────

static Vector3 parseVec3(const std::string &s) {
  auto parts = split(s, '/');
  if (parts.size() != 3)
    return {0, 0, 0};
  return {std::stof(parts[0]), std::stof(parts[1]), std::stof(parts[2])};
}

// ─── Message Handlers ───────────────────────────────────────────────────────

void GameClient::handlePlayersBroadcast(const std::string &msg) {
  // Format: p:<id>:<x/y/z>|<id>:<x/y/z>|...
  std::string data = msg.substr(2);
  auto entries = split(data, '|');

  std::lock_guard<std::mutex> lock(players_mutex);
  std::vector<int> current_ids;

  for (const auto &entry : entries) {
    if (entry.empty())
      continue;

    auto parts = split(entry, ':');
    if (parts.size() < 2)
      continue;

    int id = std::stoi(parts[0]);
    if (id == my_id)
      continue;

    current_ids.push_back(id);
    Vector3 pos = parseVec3(parts[1]);

    // Update existing player or add new one
    bool found = false;
    for (auto &p : players) {
      if (p.id == id) {
        p.Position = pos;
        found = true;
        break;
      }
    }
    if (!found) {
      Player new_player;
      new_player.id = id;
      new_player.Position = pos;
      new_player.color = Color::GetColor(
          static_cast<PlayerColor>(id % static_cast<int>(PlayerColor::COUNT)));
      players.push_back(new_player);
    }
  }

  // Remove players who left
  players.erase(std::remove_if(players.begin(), players.end(),
                               [&](const Player &p) {
                                 if (p.id == my_id)
                                   return false;
                                 return std::find(current_ids.begin(),
                                                  current_ids.end(),
                                                  p.id) == current_ids.end();
                               }),
                players.end());
}

void GameClient::handleSeedMsg(const std::string &msg) {
  // Format: s:<seed>
  unsigned int s = static_cast<unsigned int>(std::stoul(msg.substr(2)));
  SetSeed(s);
}

void GameClient::handleColorMsg(const std::string &msg) {
  // Format: c:r,g,b
  auto rgb = split(msg.substr(2), ',');
  if (rgb.size() < 3)
    return;

  unsigned int r = std::stoul(rgb[0]);
  unsigned int g = std::stoul(rgb[1]);
  unsigned int b = std::stoul(rgb[2]);

  std::lock_guard<std::mutex> lock(players_mutex);
  if (!players.empty())
    players[0].color = Color{static_cast<Uint8>(r), static_cast<Uint8>(g),
                             static_cast<Uint8>(b)};
}

void GameClient::handleBlockModification(const std::string &msg) {
  // Format: bm:x/y/z:type
  auto parts = split(msg, ':');
  if (parts.size() < 3)
    return;

  try {
    Vector3 pos = parseVec3(parts[1]);
    Uint8 type = static_cast<Uint8>(std::stoi(parts[2]));
    pending_mods.push_back({pos, type});
  } catch (...) {
  }
}

void GameClient::handlePlayerData(const std::string &msg) {
  // Format: pData:x/y/z|rx/ry/rz|type,amount,isEntity|type,amount,isEntity|...
  std::string data = msg.substr(6);
  auto sections = split(data, '|');
  if (sections.size() < 2)
    return;

  std::lock_guard<std::mutex> lock(players_mutex);
  if (players.empty())
    return;

  // Section 0: position, Section 1: rotation, Section 2+: inventory slots
  players[0].Position = parseVec3(sections[0]);
  players[0].Rotation = parseVec3(sections[1]);

  // Parse inventory slots (sections[2] onward)
  int slotIdx = 0;
  for (size_t i = 2;
       i < sections.size() && slotIdx < (int)players[0].inventory.size(); ++i) {
    if (sections[i].empty())
      continue;

    auto fields = split(sections[i], ',');
    if (fields.size() >= 3) {
      players[0].inventory[slotIdx].Type =
          static_cast<short>(std::stoi(fields[0]));
      players[0].inventory[slotIdx].Amount =
          static_cast<short>(std::stoi(fields[1]));
      players[0].inventory[slotIdx].isEntity = (fields[2] == "1");
    }
    slotIdx++;
  }
}

// ─── Network Listener ───────────────────────────────────────────────────────

void GameClient::listen() {
  while (running && socket.is_open()) {
    std::string msg = receiveMessage();
    if (msg.empty()) {
      if (running) {
        std::cerr << "Disconnected from server." << std::endl;
        running = false;
      }
      break;
    }

    // Dispatch to the appropriate handler
    if (msg.find("p:") == 0) {
      handlePlayersBroadcast(msg);
    } else if (msg.find("s:") == 0) {
      handleSeedMsg(msg);
    } else if (msg.find("c:") == 0) {
      handleColorMsg(msg);
    } else if (msg.find("bm:") == 0) {
      handleBlockModification(msg);
    } else if (msg.find("pData:") == 0) {
      handlePlayerData(msg);
    } else if (msg.find("new:") == 0) {
      this->is_new_player = true;
    }
  }
}

// ─── Outgoing Messages ─────────────────────────────────────────────────────
void GameClient::update_pos() {
  auto Player = get_players()[0];
  std::string Pos = "up:" + std::to_string(my_id) + ":" +
                    std::to_string(Player.Position.x) + "/" +
                    std::to_string(Player.Position.y) + "/" +
                    std::to_string(Player.Position.z) + ":" +
                    std::to_string(Player.Rotation.x) + "/" +
                    std::to_string(Player.Rotation.y) + "/" +
                    std::to_string(Player.Rotation.z);
  this->sendCommand(Pos);
}

void GameClient::sync_inventory() {
  auto &p = get_players()[0];
  std::string msg = "inv:";
  for (const auto &slot : p.inventory) {
    msg += std::to_string(slot.Type) + "," + std::to_string(slot.Amount) + "," +
           (slot.isEntity ? "1" : "0") + "|";
  }
  this->sendCommand(msg);
}