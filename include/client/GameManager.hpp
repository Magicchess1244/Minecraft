#pragma once
#include "../client/GameClient.hpp"
#include "../client/Renderer.hpp"
#include "../common/ChunkManager.hpp"
#include <memory>
#include <chrono>
#include <ratio>

class GameManager {
private:
  std::shared_ptr<ChunkManager> chunkManager = std::make_shared<ChunkManager>();
  std::shared_ptr<GameClient> gameClient = std::make_shared<GameClient>();
  std::shared_ptr<Renderer> renderer = std::make_shared<Renderer>(this->gameClient, this->chunkManager);
  Player localPlayer;
  float deltaTime = 1.0f;
  float acumulatedDeltaTime = 1.0f;
  int tick = 0;
  std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<long, std::ratio<1, 1000000000>>> lastTime;

  void PlayerInit();
  void GameLoop();
  void TimeUpdate();

public:
    GameManager();
};