#pragma once

#include "../common/Common.hpp"
#include <asio.hpp>
#include <map>
#include <mutex>

using asio::ip::tcp;
#define PORT 12345

class GameServer {
private:
  asio::io_context io;
  tcp::acceptor acceptor;
  unsigned int seed;
  std::atomic<bool> running;               // Thread-safe flag
  std::vector<std::thread> client_threads; // Track threads instead of sockets
  std::map<int, Player> players;
  std::map<int, std::shared_ptr<tcp::socket>> client_sockets;
  mutable std::recursive_mutex players_mutex;
  std::atomic<int> next_id{0};
  std::unordered_map<Vector3, Uint8> Modifications;

public:
  GameServer();
  ~GameServer();

  void set_seed(unsigned int new_seed);
  unsigned int get_seed() const;
  void AcceptClients();
  void handlePlayers(std::shared_ptr<tcp::socket> socket, int id);

  // Message handlers (called from handlePlayers)
  void handleLogin(const std::string &message,
                   std::shared_ptr<tcp::socket> socket, int id);
  void handleSeed(std::shared_ptr<tcp::socket> socket);
  void handleGetColor(std::shared_ptr<tcp::socket> socket);
  void handleUpdate(const std::string &message, int id);
  void handleInventory(const std::string &message, int id);
  void handleModification(const std::string &message);

  // Broadcasting
  void broadcastPlayers();
  void broadcastModification(Vector3 pos, Uint8 type);
  void sendAllModifications(std::shared_ptr<tcp::socket> socket);
  void saveModifications();
  void loadModifications();
  void savePlayers();
  void loadPlayers();

private:
  std::unordered_map<std::string, Player> persistentPlayers;
};