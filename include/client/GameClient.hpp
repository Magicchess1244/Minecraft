#ifndef __GAME_CLIENT_H
#define __GAME_CLIENT_H

#include "../common/Common.hpp"
#include "Renderer.hpp"
#include <algorithm>
#include <asio.hpp>
#include <asio/ip/address_v4.hpp>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>

using asio::ip::tcp;
#define PORT 12345

typedef enum { GetSeed, GetColor } Commands;

class GameClient {
private:
  asio::io_context io;
  tcp::socket socket;
  unsigned int seed;
  unsigned int player_count = 0;
  std::vector<Player> players;
  std::mutex players_mutex;
  bool running = true;
  int my_id = -1;
  asio::streambuf read_buffer;
  std::thread net_thread;

public:
  GameClient() : seed(0), player_count(0), socket(io) {
    try {
      tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), PORT);
      this->socket.connect(endpoint);
      std::cout << "Connected to server at 127.0.0.1:" << PORT << std::endl;

      // Receive ID
      std::string id_str = receiveMessage();
      if (id_str.find("id:") == 0) {
        my_id = std::stoi(id_str.substr(3));
        std::cout << "Assigned ID: " << my_id << std::endl;
      }
    } catch (std::exception &e) {
      std::cerr << "Failed to connect to server: " << e.what() << std::endl;
    }
  }

  void StartListener() {
    if (!net_thread.joinable()) {
      net_thread = std::thread(&GameClient::listen, this);
    }
  }

  ~GameClient() {
    running = false;
    if (net_thread.joinable())
      net_thread.join();
    if (this->socket.is_open()) {
      this->socket.close();
    }
    std::cout << "Client disconnected cleanly.\n";
  }

  void listen(); // Declaration for listener thread

  void sendCommand(const std::string &command) {
    asio::error_code error;
    std::string cmd = command + "\n";
    asio::write(socket, asio::buffer(cmd), error);
    if (error) {
      std::cerr << "Send error: " << error.message() << std::endl;
    }
  }

  std::string receiveMessage() {
    asio::error_code error;
    size_t n = asio::read_until(socket, read_buffer, '\n', error);
    if (error) {
      if (error != asio::error::eof) {
        std::cerr << "Receive error: " << error.message() << std::endl;
      }
      return "";
    }
    std::string s;
    std::istream is(&read_buffer);
    std::getline(is, s);
    if (!s.empty() && s.back() == '\r')
      s.pop_back();
    return s;
  }

  void set_seed();
  unsigned int get_seed() const { return seed; }

  void add_player(Player player) {
    if (players.size() < MAX_PLAYERS) {
      this->players.push_back(player);
      this->player_count++;
    }
  }

  const std::vector<Player> &get_players() const { return this->players; }
  std::vector<Player> &get_players() { return this->players; }

  void set_color();
  Color get_color() const {
    if (players.size() > 0) {
      return players[0].color;
    }
    return {0, 0, 0};
  }

  std::mutex &get_mutex() { return players_mutex; }
  int get_my_id() const { return my_id; }
  void update_pos();

  bool GetRunning() const { return running; }
  void Quit() {
    running = false;
    asio::error_code ec;
    socket.close(ec);
  }
};

namespace BitMiner {
int FindSlot(std::vector<Slot> &Inventory, short Type);
void PlayerInput(Vector3 &PlayerDirection, bool OnGround, int &InventorySlots,
                 Vector3 &PlayerRot);
void PlayerMovement(Player &player, int &inventorySlot);
void GameLoop(GameClient &game);
} // namespace BitMiner

#endif