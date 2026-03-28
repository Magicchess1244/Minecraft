#pragma once
#include "../../include/common/Common.hpp"
#include "../../include/client/GameClient.hpp"
#include <vector>


class PlayerManager{
  private:
    GameClient &gameClient;
    std::vector<Player>& players;
    int inventorySlot = 0;

    void PlayerInit();
    int FindSlot(std::vector<Slot> &Inventory, short Type, bool isEntity);
    void PlayerInput(Vector3 &PlayerDirection, bool OnGround, bool InWater, int &InventorySlots, Vector3 &PlayerRot, bool &LeftClick, bool &RightClick);
    void PlayerRotation( Vector3 RotationDir);
    void PlayerMove(Vector3 playerDirection,ChunkManager &manager);
    void PlayerBreackPlace(bool Left, bool Right, ChunkManager &manager, int inventorySlot, std::vector<Slot> &inventory, Renderer *renderer);

  public:
    PlayerManager(GameClient &client);
    void PlayerAction(int &inventorySlot, ChunkManager &manager, Renderer *renderer);
};