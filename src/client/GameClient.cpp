#include "../../include/client/GameClient.hpp"
#include "../../include/common/PerlinNoise.hpp"
#include <SDL3/SDL_stdinc.h>
#include <chrono>
#include <ostream>

constexpr float mouseSensitivity = 0.1f;
constexpr float playerSpeed = 5.0f;
float deltaTime = 1.0f;
float JumpTimer = 0;
float bodyHeight = 1.8f;
Vector3 playerDirection = {0, 0, 0};
constexpr bool PLayerColistion = true;

void GameClient::set_seed() {
  /*int res;
  char buf[16];
  unsigned int server_seed = 0;
  const char* command = "seed";
  std::cout << "seed" << std::endl;
  res = send(this->server, command, sizeof(command), 0);
  if (res <= 0) std::cerr << "Error requesting seed from the server.";
  res = recv(this->server, buf, sizeof(buf), 0);
  if (res <= 0) std::cerr << "Error receiving seed from the server.";
  std::cout << "buf: " << buf << std::endl;
  server_seed = static_cast<unsigned int>(std::stoi(std::string(buf)));
  std::cout << "New client seed: " << server_seed << std::endl;
  srand(server_seed);
  this->seed = server_seed;*/
}
void GameClient::set_color() {
  /*	int res;
          char buf[11];
          unsigned int server_color[3] = {};
          const char* command = "getColor";

          std::cout << "getColor" << std::endl;

          res = send(this->server, command, sizeof(command), 0);
          if (res <= 0) std::cerr << "Error requesting color from the server.";
          res = recv(this->server, buf, sizeof(buf), 0);
          if (res <= 0) std::cerr << "Error receiving color from the server.";
          else std::cout << "recived" << std::endl;

          std::cout << "color" << std::endl;
          int i = 0;
          for (std::string w : split(buf, ",")) {
                  server_color[i++] = std::stoi(w);
          }

          std::cout << "New client color: " << server_color[0] << "," <<
     server_color[1] <<"," << server_color[2] << std::endl;

          std::cout << "niggas online: " << this->players.size() << std::endl;
          if (this->players.size() > 0) {
                  this->players[0].color = Color{ server_color[0],
     server_color[1], server_color[2] };
          }*/
}

// UI function
namespace BitMiner {
int FindSlot(std::vector<Slot> &Inventory, short Type) {
  int index = 0;
  for (Slot slot : Inventory) {
    if ((slot.Type == Type || slot.Type == 0) && slot.Amount < 64) {
      std::cout << "Found slot" << index << std::endl;
      return index;
    }
    index++;
  }
  std::cout << "Inventory full, cannot add item of type: " << Type << std::endl;
  // FIXME: return Option<int>
  return 0;
}
void PlayerInput(Vector3 &PlayerDirection, bool OnGround, int &InventorySlots,
                 Vector3 &PlayerRot, bool &LeftClick, bool &RightClick) {
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

  // Detect mouse button clicks
  LeftClick = (mouseState & SDL_BUTTON_LMASK) != 0;
  RightClick = (mouseState & SDL_BUTTON_RMASK) != 0;

  // Apply mouse movement to rotation
  // Mouse X controls yaw (Y-axis rotation)
  // Mouse Y controls pitch (X-axis rotation)
  PlayerRot.y = mouseX; // Yaw (left/right)
  PlayerRot.x = mouseY; // Pitch (up/down)
  PlayerRot.z = 0;      // Roll (not used for FPS camera)

  if (move_left || move_right) {
    PlayerDirection.x = move_left ? -1 : 1;
  }
  if (!OnGround) {
    PlayerDirection.y -= 0.1f;
    PlayerDirection.y = SDL_clamp(PlayerDirection.y, -4, 10);
  } else {
    PlayerDirection.y = 0;
    if (move_up && JumpTimer > 0.2f) {
      PlayerDirection.y = 2.0f;
      JumpTimer = 0;
    }
  }
  if (move_backward || move_foward) {
    PlayerDirection.z = move_backward ? -1 : 1;
  }

  for (int i = 0; i < 8; ++i) {
    if (KeyboardState[SDL_SCANCODE_1 + i]) {
      InventorySlots = i;
      break;
    }
  }
}
void PlayerRotation(Player &player, Vector3 RotationDir) {
  if (RotationDir.x != 0 || RotationDir.y != 0) {
    player.Rotation.y += RotationDir.y * mouseSensitivity;
    player.Rotation.x += RotationDir.x * mouseSensitivity;
    player.Rotation.x = SDL_clamp(player.Rotation.x, -90.0f, 90.0f);
    if (player.Rotation.y > 360.0f)
      player.Rotation.y -= 360.0f;
    if (player.Rotation.y < 0.0f)
      player.Rotation.y += 360.0f;
  }
}
void PlayerMove(Player &player, Vector3 playerDirection,
                ChunkManager &manager) {
  // Define player collision box
  // Collision points relative to camera (player.Position)
  // Assuming camera is at eye level, roughly 1.6 units above feet
  auto isColliding = [&](Vector3 pos) {
    if (!PLayerColistion)
      return false;
    float r = 0.3f; // player radius
    float eyeHeight = 1.6f;

    // Points to check: corners of the box at foot level, waist level, and head
    // level
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
  // Handle movement
  if (playerDirection.x != 0 || playerDirection.y != 0 ||
      playerDirection.z != 0) {
    // 1. Vertical Movement
    if (playerDirection.y != 0) {
      Vector3 nextY = player.Position;
      nextY.y += playerDirection.y * playerSpeed * deltaTime;
      if (!isColliding(nextY)) {
        player.Position.y = nextY.y;
      }
    }

    // 2. Horizontal Movement
    Vector3 Rot = player.Rotation;
    Rot.x = 0;
    Vector3 Dir = Rot.Forward() * playerDirection.z +
                  Rot.Right() * playerDirection.x * 0.75f;

    if (Dir.LengthSquared() > 0.0001f) {
      Dir = Dir.Normalized() * playerSpeed * deltaTime;

      // Try Move X
      Vector3 nextX = player.Position;
      nextX.x += Dir.x;
      if (!isColliding(nextX)) {
        player.Position.x = nextX.x;
      }

      // Try Move Z
      Vector3 nextZ = player.Position;
      nextZ.z += Dir.z;
      if (!isColliding(nextZ)) {
        player.Position.z = nextZ.z;
      }
    }
  }
}
void PlayerBreackPlace(bool Left, bool Right, ChunkManager &manager,
                       Player &player, int inventorySlot,
                       std::vector<Slot> &inventory) {
  static bool lastLeft = false;
  static bool lastRight = false;

  bool justLeft = Left && !lastLeft;
  bool justRight = Right && !lastRight;

  lastLeft = Left;
  lastRight = Right;

  if (justLeft || justRight) {
    RaycastResult Ray =
        manager.RayCast(player.Position, player.Rotation.Forward(), 10);
    if (Ray.hit) {
      if (justLeft) {
        manager.Place(Ray.pos, 0); // Break block (Air)
      } else {
        // Prevent placing block inside player's body
        Vector3 placePos = Ray.prevPos;
        bool collides = false;

        // Check if the placed block intersects with the player's bounding box
        // Player is roughly from CameraY-1.6 to CameraY+0.2
        for (float yOff = -1.5f; yOff <= 0.1f; yOff += 0.5f) {
          Vector3 check = (player.Position + Vector3(0, yOff, 0)).Truncate();
          if (check == placePos) {
            collides = true;
            break;
          }
        }

        if (!collides) {
          manager.Place(placePos, inventory[inventorySlot].Type);
        }
      }
    }
  }
}
void PlayerAction(Player &player, int &inventorySlot, ChunkManager &manager,
                  std::vector<Slot> &inventory) {

  JumpTimer += deltaTime;
  Vector3 RotationDir = {0, 0, 0};
  bool LeftClick = false, RightClick = false;
  bool OnGround = manager.RayCast(player.Position, {0, -1, 0}, bodyHeight + 0.15f).hit;
  PlayerInput(playerDirection, OnGround, inventorySlot, RotationDir, LeftClick,
              RightClick);
  PlayerRotation(player, RotationDir);
  PlayerMove(player, playerDirection, manager);
  PlayerBreackPlace(LeftClick, RightClick, manager, player, inventorySlot,
                    inventory);
}
void GameLoop(GameClient &game) {
  game.add_player({
      {0, 40, 0},
      {0.0f, 0.0f, 0.0f},
      {255, 0, 0},
  });
  auto p = game.get_players();

  // game.MakeClient();
  // game.set_seed();
  // game.set_color();
  SetSeed(game.get_seed());

  Vector3 playerDirection = {0, 0};

  std::vector<Slot> inventory;

  for (int i = 0; i < 8; i++) {
    inventory.push_back({0, 0});
  }

  inventory[0] = {60, 1};
  inventory[1] = {5, 5};

  int inventorySlot = 0;

  // ChunckManager::Size(width, height, Range.y, Range.x);
  ChunkManager chunkManager;
  Renderer RendererObject(game, chunkManager);

  auto lastTime = std::chrono::high_resolution_clock::now();

  while (game.GetRunning()) {

    PlayerAction(p[0], inventorySlot, chunkManager, inventory);
    RendererObject.MainRenderLoop(inventory, inventorySlot, p);

    auto currentTime = std::chrono::high_resolution_clock::now();
    deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    lastTime = currentTime;
  }

  std::cout << "Exiting game..." << std::endl;
}

} // namespace BitMiner