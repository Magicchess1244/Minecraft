#include "../../include/client/GameManager.hpp"
#include <memory>

constexpr unsigned int tickPerSeconds = 20;
// FIX Make clases use shared ptr
GameManager::GameManager() {

  PlayerInit();

  if (!gameClient->GetRunning()) {
    PrintError("Could not connect to server. Exiting.");
    return;
  } else {
    this->lastTime = std::chrono::high_resolution_clock::now();
    GameLoop();
  }
}
void GameManager::PlayerInit(){
  int myId = this->gameClient->get_my_id();
  this->localPlayer.id = myId;
  this->localPlayer.name = this->gameClient->get_my_name();
  this->localPlayer.Position = {0, ChunkPrefab::ySize, 0};
  this->localPlayer.color = Color::GetColor(
      static_cast<PlayerColor>(myId % static_cast<int>(PlayerColor::COUNT)));

  this->gameClient->add_player(localPlayer);
  auto &p = this->gameClient->get_players();
  this->gameClient->set_seed();
  this->gameClient->set_color();
  this->gameClient->sendCommand("gm");
  this->gameClient->StartListener();
}

void GameManager::GameLoop(){
   while (this->gameClient->GetRunning()) {
    std::lock_guard<std::mutex> lock(this->gameClient->get_mutex());
    auto &mods = this->gameClient->get_pending_mods();
    for (auto const &m : mods) {
      this->chunkManager->Place(m.pos, m.type);
    }
    mods.clear();

    if (this->tick % 1) {
      this->gameClient->update_pos();
    }
    if (this->tick % 40) {
      this->gameClient->sync_inventory();
    }
    if (this->tick % 40) {
      chunkManager->TickWater();
    }

    BitMiner::PlayerAction(this->localPlayer, 0, this->chunkManager, this->localPlayer.inventory, this->gameClient,
                 this->renderer);
    this->renderer->MainRenderLoop(p[0].inventory, 0, p);

    TimeUpdate();
  }
}

void GameManager::TimeUpdate(){
  auto currentTime = std::chrono::high_resolution_clock::now();
  this->deltaTime = std::chrono::duration<float>(currentTime - this->lastTime).count();
  this->deltaTime = SDL_clamp(this->deltaTime, 0.0f, 0.1f); // Max 100ms per frame
  this->lastTime = this->currentTime;
  acumulatedDeltaTime += deltaTime;
  float lastTick = this->tick;
  this->tick += this->acumulatedDeltaTime * tickPerSeconds;

  if (this->tick > lastTick) {
    acumulatedDeltaTime = 0;
  }
}