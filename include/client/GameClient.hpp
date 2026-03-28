#ifndef __GAME_CLIENT_H
#define __GAME_CLIENT_H

#include "../common/Common.hpp"
#include "../common/PerlinNoise.hpp"
#include "Renderer.hpp"
#include <asio.hpp>
#include <asio/ip/address_v4.hpp>
#include <iostream>
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
  bool is_new_player = false;
  int my_id = -1;
  asio::streambuf read_buffer;
  std::vector<BlockMod> pending_mods;
  std::thread net_thread;

  std::string my_name;

public:
  GameClient();
  void StartListener() {
    if (!net_thread.joinable()) {
      net_thread = std::thread(&GameClient::listen, this);
    }
  }

  ~GameClient() {
    running = false;
    io.stop(); // Stop all asynchronous operations
    if (this->socket.is_open()) {
      asio::error_code ec;
      this->socket.shutdown(tcp::socket::shutdown_both, ec);
      this->socket.close(ec);
    }
    if (net_thread.joinable())
      net_thread.join();
    std::cout << "Client disconnected cleanly.\n";
  }

  void listen(); // Declaration for listener thread

  // Message handlers (called from listen)
  void handlePlayersBroadcast(const std::string &msg);
  void handleSeedMsg(const std::string &msg);
  void handleColorMsg(const std::string &msg);
  void handleBlockModification(const std::string &msg);
  void handlePlayerData(const std::string &msg);

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
  std::vector<BlockMod> &get_pending_mods() { return pending_mods; }

  int get_my_id() const { return my_id; }
  std::string get_my_name() const { return my_name; }
  bool get_is_new_player() const { return is_new_player; }
  void set_is_new_player(bool v) { is_new_player = v; }
  void update_pos();
  void sync_inventory();

  bool GetRunning() const { return running; }
  void Quit() { this->running = false; }
};

#endif