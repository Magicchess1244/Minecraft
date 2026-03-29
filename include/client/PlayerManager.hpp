#pragma once
#include "../../include/common/Common.hpp"
#include "../../include/client/GameClient.hpp"
#include "Renderer.hpp"
#include <vector>


class PlayerManager{
  private:
    GameClient &gameClient;
    Renderer &renderer;
    ChunkManager &chunkManager;
    std::vector<Player>& players;
    int inventorySlot = 0;
    std::vector<Slot> inventory{};

    void PlayerInit();
    int FindSlot(std::vector<Slot> &Inventory, short Type, bool isEntity);
    void PlayerInput(Vector3 &PlayerDirection, int &InventorySlots, Vector3 &PlayerRot, bool &LeftClick, bool &RightClick);
    void PlayerRotation( Vector3 RotationDir);
    void PlayerMove(Vector3 playerDirection, float deltaTime);
    void PlayerBreackPlace(bool Left, bool Right);
    bool OnGround();
    bool OnWater();
    void Break(RaycastResult ray);
    void Place(RaycastResult ray);

  public:
    PlayerManager(GameClient &client, Renderer &renderer, ChunkManager &manager);
    void PlayerAction(int inventorySlot, float deltaTime);
};