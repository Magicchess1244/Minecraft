#pragma once
#include "../client/GameClient.hpp"
#include "../client/Renderer.hpp"
#include "../client/PlayerManager.hpp"
#include "../common/ChunkManager.hpp"
#include "PlayerManager.hpp"
#include <chrono>
#include <ratio>

class GameManager {
  private:
    ChunkManager chunkManager{};
    GameClient gameClient{};
    Renderer renderer{this->gameClient, this->chunkManager};
    PlayerManager playerManager{this->gameClient};
    float deltaTime = 1.0f;
    float acumulatedDeltaTime = 1.0f;
    int tick = 0;
    std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<long, std::ratio<1, 1000000000>>> lastTime{};

    void PlayerInit();
    void GameLoop();
    void TimeUpdate();
    void TickUpdate();
    void ModUpdate();

  public:
    GameManager();
    ~GameManager();
};