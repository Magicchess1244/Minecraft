#include "../../include/client/GameManager.hpp"

constexpr unsigned int tickPerSeconds = 20;

GameManager::GameManager() : playerManager(*this), renderer(*this) {
  if (!gameClient.GetRunning()) {
    PrintError("Could not connect to server. Exiting.");
  } else {
    this->lastTime = std::chrono::high_resolution_clock::now();
  }
}

GameManager::~GameManager() { PrintSuccesfull("Exciting game"); }

void GameManager::GameLoop() {
  while (this->gameClient.GetRunning()) {
    ModUpdate();
    TickUpdate();

    this->playerManager.PlayerAction(0, this->deltaTime);
    auto &p = this->gameClient.get_players();
    this->renderer.MainRenderLoop(p[0].inventory, 0, p);

    TimeUpdate();
  }
}

void GameManager::TimeUpdate() {
  auto currentTime = std::chrono::high_resolution_clock::now();
  this->deltaTime =
      std::chrono::duration<float>(currentTime - this->lastTime).count();
  this->deltaTime =
      SDL_clamp(this->deltaTime, 0.0f, 0.1f); // Max 100ms per frame
  this->lastTime = currentTime;
  this->acumulatedDeltaTime += this->deltaTime;
  float lastTick = this->tick;
  this->tick += this->acumulatedDeltaTime * tickPerSeconds;

  if (this->tick > lastTick)
    acumulatedDeltaTime = 0;
}

void GameManager::TickUpdate() {
  if (this->tick % 1) {
    this->gameClient.update_pos();
  }
  if (this->tick % 40) {
    this->gameClient.sync_inventory();
    chunkManager.TickWater();
    auto &p = this->gameClient.get_players();
    if (!p.empty()) {
      chunkManager.UnloadFarChunks(p[0].Position, (9 + 12) * 16);
    }
  }
}

void GameManager::ModUpdate() {
  std::lock_guard<std::mutex> lock(this->gameClient.get_mutex());
  auto &mods = this->gameClient.get_pending_mods();
  for (auto const &m : mods) {
    this->chunkManager.Place(m.pos, m.type);
  }
  mods.clear();
}