#include "../../include/client/PlayerManager.hpp"

constexpr float mouseSensitivity = 0.1f;
constexpr float playerSpeed = 5.0f;
constexpr float JumpHeight = 1.5f;
constexpr float JumpPower = 5.0f;
float JumpTimer = 0;
float bodyHeight = 1.6f;
Vector3 playerDirection = {0, 0, 0};
constexpr bool PLayerColistion = true;

PlayerManager::PlayerManager(GameClient &client) : 
gameClient(client), players(this->gameClient.get_players()){
  PlayerInit();
}

void PlayerManager::PlayerInit(){
  int myId = this->gameClient.get_my_id();
  Player localPlayer{};
  localPlayer.id = myId;
  localPlayer.name = this->gameClient.get_my_name();
  localPlayer.Position = {0, ChunkPrefab::ySize, 0};
  localPlayer.color = Color::GetColor(
      static_cast<PlayerColor>(myId % static_cast<int>(PlayerColor::COUNT)));

  this->gameClient.add_player(localPlayer);
}

int PlayerManager::FindSlot(std::vector<Slot> &Inventory, short Type, bool isEntity) {
  int index = 0;
  if (Type == 0)
    return -1;
  for (Slot slot : Inventory) {
    if ((slot.Type == Type && slot.isEntity == isEntity || slot.Type == 0) &&
        slot.Amount < 64) {
      return index;
    }
    index++;
  }
  return -1;
}

// ─── Input ──────────────────────────────────────────────────────────────────

void PlayerManager::PlayerInput(Vector3 &PlayerDirection, bool OnGround, bool InWater,
                 int &InventorySlots, Vector3 &PlayerRot, bool &LeftClick,
                 bool &RightClick) {
  const bool *KeyboardState = SDL_GetKeyboardState(NULL);
  const bool move_foward =
      (KeyboardState[SDL_SCANCODE_W] || KeyboardState[SDL_SCANCODE_UP]);
  const bool move_backward =
      (KeyboardState[SDL_SCANCODE_S] || KeyboardState[SDL_SCANCODE_DOWN]);
  const bool move_up = KeyboardState[SDL_SCANCODE_SPACE];
  const bool move_down = KeyboardState[SDL_SCANCODE_LSHIFT];
  const bool move_left =
      KeyboardState[SDL_SCANCODE_A] || KeyboardState[SDL_SCANCODE_LEFT];
  const bool move_right =
      KeyboardState[SDL_SCANCODE_D] || KeyboardState[SDL_SCANCODE_RIGHT];

  PlayerDirection.x = 0;
  PlayerDirection.z = 0;

  // Get mouse movement and button states
  float mouseX, mouseY;
  Uint32 mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

  LeftClick = (mouseState & SDL_BUTTON_LMASK) != 0;
  RightClick = (mouseState & SDL_BUTTON_RMASK) != 0;
  PlayerRot.y = mouseX;
  PlayerRot.x = mouseY;
  PlayerRot.z = 0;

  if (move_left || move_right) {
    PlayerDirection.x = move_left ? -1 : 1;
  }

  if (InWater) {
    // Swimming mechanics
    if (move_up) {
      PlayerDirection.y = JumpHeight * JumpPower; // Swim up
    } else if (move_down) {
      PlayerDirection.y -= 2.0f; // Swim down
      PlayerDirection.y = std::clamp(playerDirection.y, -6.0f, 0.f);
    } else {
      PlayerDirection.y = -2.5f; // Slow sink in water
    }
  } else {
    // Normal gravity and jumping
    if (!OnGround) {
      PlayerDirection.y -= 0.5f;
    } else {
      PlayerDirection.y = 0;
      if (move_up && JumpTimer > 0.1f) {
        PlayerDirection.y = JumpHeight * JumpPower;
        JumpTimer = 0;
      }
    }
  }

  if (move_backward || move_foward) {
    PlayerDirection.z = move_backward ? -1 : 1;
  }

  for (int i = 0; i < 9; ++i) {
    if (KeyboardState[SDL_SCANCODE_1 + i]) {
      InventorySlots = i;
      break;
    }
  }
}

// ─── Rotation ───────────────────────────────────────────────────────────────

void PlayerManager::PlayerRotation(Vector3 RotationDir) {
  if (RotationDir.x != 0 || RotationDir.y != 0) {
    this->players[0].Rotation.y += RotationDir.y * mouseSensitivity;
    this->players[0].Rotation.x += RotationDir.x * mouseSensitivity;
    this->players[0].Rotation.x = SDL_clamp(this->players[0].Rotation.x, -90.0f, 90.0f);
    if (this->players[0].Rotation.y > 360.0f)
      this->players[0].Rotation.y -= 360.0f;
    if (this->players[0].Rotation.y < 0.0f)
      this->players[0].Rotation.y += 360.0f;
  }
}

// ─── Movement & Collision ───────────────────────────────────────────────────

void PlayerManager::PlayerMove(Vector3 playerDirection,
                ChunkManager &manager, float deltaTime) {
  auto isColliding = [&](Vector3 pos) {
    if (!PLayerColistion)
      return false;
    float r = 0.4f;

    float yChecks[] = {-1.4f, -0.8f, 0.1f};
    float xzChecks[] = {-r, r};

    for (float yOff : yChecks) {
      for (float xOff : xzChecks) {
        for (float zOff : xzChecks) {
          if (manager.IsSolid(pos + Vector3(xOff, yOff, zOff))) {
            return true;
          }
        }
      }
    }
    return false;
  };

  if (playerDirection.x != 0 || playerDirection.y != 0 ||
      playerDirection.z != 0) {
    // Vertical movement
    if (playerDirection.y != 0) {
      Vector3 nextY = this->players[0].Position;
      float verticalSpeed =
          SDL_clamp(playerDirection.y * deltaTime, -10, JumpHeight);
      nextY.y += verticalSpeed;
      if (!isColliding(nextY)) {
        this->players[0].Position.y = nextY.y;
      }
    }

    // Horizontal movement
    Vector3 Rot = this->players[0].Rotation;
    Rot.x = 0;
    Vector3 Dir = Rot.Forward() * playerDirection.z +
                  Rot.Right() * playerDirection.x * 0.75f;

    if (Dir.LengthSquared() > 0.0001f) {
      Dir = Dir.Normalized() * playerSpeed * deltaTime;

      // Try X axis
      Vector3 nextX = this->players[0].Position;
      nextX.x += Dir.x;
      if (!isColliding(nextX)) {
        this->players[0].Position.x = nextX.x;
      }

      // Try Z axis
      Vector3 nextZ = this->players[0].Position;
      nextZ.z += Dir.z;
      if (!isColliding(nextZ)) {
        this->players[0].Position.z = nextZ.z;
      }
    }
  }
}

// ─── Block Break / Place ────────────────────────────────────────────────────

void PlayerManager::PlayerBreackPlace(bool Left, bool Right, ChunkManager &manager,
                       int inventorySlot,
                       std::vector<Slot> &inventory,
                       Renderer *renderer) {
  static bool lastLeft = false;
  static bool lastRight = false;

  bool justLeft = Left && !lastLeft;
  bool justRight = Right && !lastRight;

  lastLeft = Left;
  lastRight = Right;

  if (justLeft || justRight) {
    RaycastResult Ray =
        manager.RayCast(this->players[0].Position, this->players[0].Rotation.Forward(), 7);
    if (Ray.hit) {
      if (justLeft) {
        if (Ray.BlockID == 4)
          return; // Can't break bedrock

        manager.Place(Ray.pos, 0); // Break block (Air)
        std::string mod = "mod:" + std::to_string(Ray.pos.x) + "/" +
                          std::to_string(Ray.pos.y) + "/" +
                          std::to_string(Ray.pos.z) + ":0";

        int slotIdx = FindSlot(inventory, BlockDef[Ray.BlockID].Drop, false);
        if (slotIdx != -1) {
          if (inventory[slotIdx].Amount == 0) {
            inventory[slotIdx].Type = BlockDef[Ray.BlockID].Drop;
            inventory[slotIdx].isEntity = false;
          }
          inventory[slotIdx].Amount++;
        }
        this->gameClient.sendCommand(mod);
        this->gameClient.sync_inventory();
      } else if (!BlockDef[Ray.BlockID].hasUI()) {
        Vector3 placePos = Ray.prevPos;
        bool collides = false;

        for (float yOff = -1.5f; yOff <= 0.1f; yOff += 0.5f) {
          Vector3 check = (this->players[0].Position + Vector3(0, yOff, 0)).Truncate();
          if (check == placePos) {
            collides = true;
            break;
          }
        }

        if (!collides && inventory[inventorySlot].Amount > 0 &&
            !inventory[inventorySlot].isEntity &&
            BlockDef[inventory[inventorySlot].Type].CanPlace()) {
          Uint8 type = inventory[inventorySlot].Type;
          inventory[inventorySlot].Amount--;
          if (inventory[inventorySlot].Amount == 0)
            inventory[inventorySlot].Type = 0;
          manager.Place(placePos, type);
          std::string mod = "mod:" + std::to_string((int)placePos.x) + "/" +
                            std::to_string((int)placePos.y) + "/" +
                            std::to_string((int)placePos.z) + ":" +
                            std::to_string(type);
          this->gameClient.sendCommand(mod);
          this->gameClient.sync_inventory();
        }
      } else {
        renderer->SetUi(Ray.BlockID);
      }
    }
  }
}

// ─── Player Action (per-frame) ──────────────────────────────────────────────

void PlayerManager::PlayerAction(int inventorySlot, ChunkManager &manager,
                  Renderer *renderer, float deltaTime) {

  static bool hasInitialChunkLoaded = false;
  if (!hasInitialChunkLoaded) {
    Vector3 chunkKey;
    chunkKey.x = std::floor(this->players[0].Position.x / ChunkPrefab::xSize);
    chunkKey.y = 0;
    chunkKey.z = std::floor(this->players[0].Position.z / ChunkPrefab::zSize);

    if (!manager.get_chunk(chunkKey).isGenerated) {
      return;
    }
    hasInitialChunkLoaded = true;

    if (this->gameClient.get_is_new_player()) {
      int height = manager.get_chunk(chunkKey).GetHeight(
          {this->players[0].Position.x, this->players[0].Position.z});
      this->players[0].Position.x = std::floor(this->players[0].Position.x) + 0.5f;
      this->players[0].Position.z = std::floor(this->players[0].Position.z) + 0.5f;
      this->players[0].Position.y = height + 2.5f;
      this->gameClient.set_is_new_player(false);
    }
  }

  JumpTimer += deltaTime;
  Vector3 RotationDir = {0, 0, 0};
  bool LeftClick = false, RightClick = false;
  float bounds = 0.3f;
  bool OnGround =
      manager.RayCast(this->players[0].Position, {0, -1, 0}, bodyHeight + 0.15f).hit ||
      manager
          .RayCast(this->players[0].Position + Vector3(bounds, 0, 0), {0, -1, 0},
                   bodyHeight + 0.15f)
          .hit ||
      manager
          .RayCast(this->players[0].Position + Vector3(-bounds, 0, 0), {0, -1, 0},
                   bodyHeight + 0.15f)
          .hit ||
      manager
          .RayCast(this->players[0].Position + Vector3(0, 0, bounds), {0, -1, 0},
                   bodyHeight + 0.15f)
          .hit ||
      manager
          .RayCast(this->players[0].Position + Vector3(0, 0, -bounds), {0, -1, 0},
                   bodyHeight + 0.15f)
          .hit;

  // Check if this->players[0] is in water
  this->players[0].Inwater = false;
  try {
    this->players[0].Inwater =
        (manager.GetBlockID(this->players[0].Position - Vector3(0, 0.8f, 0)) == 5) ||
        (manager.GetBlockID(this->players[0].Position) == 5);
  } catch (...) {
    this->players[0].Inwater = false;
  }
  if (renderer->UsingUI())
    return;

  PlayerInput(playerDirection, OnGround, this->players[0].Inwater, inventorySlot,
              RotationDir, LeftClick, RightClick);
  PlayerRotation(RotationDir);
  PlayerMove(playerDirection, manager, deltaTime);
  PlayerBreackPlace(LeftClick, RightClick, manager, inventorySlot,
                    this->players[0].inventory, renderer);
}