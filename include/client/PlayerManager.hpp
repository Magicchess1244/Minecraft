#pragma once
#include "../../include/common/Common.hpp"
#include <vector>

class GameManager;
class RaycastResult;

class PlayerManager{
  private:
    GameManager& gameManager;
    std::vector<Player>& players;
    int inventorySlot = 0;
    std::vector<Slot> inventory{};

    void PlayerInit();
    int FindSlot(std::vector<Slot> &Inventory, short Type, bool isEntity);
    void PlayerInput(Vector3 &PlayerDirection, int &InventorySlots, Vector3 &PlayerRot, bool &LeftClick, bool &RightClick);
    void PlayerRotation( Vector3 RotationDir);
    void PlayerMove(Vector3 playerDirection, float deltaTime);
    void PlayerBreackPlace(bool Left, bool Right, float deltaTime);
    bool OnGround();
    bool OnWater();
    void Break(RaycastResult ray, float deltaTime);
    void Place(RaycastResult ray);

  public:
    PlayerManager(GameManager& manager);
    void PlayerAction(int inventorySlot, float deltaTime);
};