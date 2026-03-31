#pragma once
#include "GameClient.hpp"
#include "PlayerManager.hpp"
#include "Renderer.hpp"
#include "../../include/common/ChunkManager.hpp"

class ChunkManager;

class GameManager {
  private:
    ChunkManager chunkManager{};
    GameClient gameClient{};
    PlayerManager playerManager;
    Renderer renderer;
    float deltaTime = 1.0f;
    float acumulatedDeltaTime = 1.0f;
    int tick = 0;
    std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<long, std::ratio<1, 1000000000>>> lastTime{};
    bool activeUI = false;

    void PlayerInit();
    void TimeUpdate();
    void TickUpdate();
    void ModUpdate();

  public:
    GameManager();
    ~GameManager();

    void GameLoop();
    Renderer& GetRenderer(){
      return renderer;
    }
    PlayerManager& GetPlayerManager(){
      return playerManager;
    }
    ChunkManager& GetChunkManager(){
      return chunkManager;
    }
    GameClient& GetGameClient(){
      return gameClient;
    }

    bool GetUsingUI(){
      return activeUI;
    }
    void SetUi(bool a){
      activeUI = a;
    }
};