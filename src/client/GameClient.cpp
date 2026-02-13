#include "../../include/client/GameClient.hpp"
#include "../../include/common/PerlinNoise.hpp"
#include <SDL3/SDL_stdinc.h>
#include <algorithm>
#include <chrono>
#include <ostream>
#include <string>

constexpr float mouseSensitivity = 0.1f;
constexpr float playerSpeed = 5.0f;
constexpr float JumpHeight = 1.5f;
constexpr float JumpPower = 5.0f;
float deltaTime = 1.0f;
float JumpTimer = 0;
float bodyHeight = 1.8f;
Vector3 playerDirection = {0, 0, 0};
constexpr bool PLayerColistion = true;

void GameClient::set_seed() {
  this->sendCommand("seed");
  // The seed is received by the main thread before the listener starts,
  // or by the listener after it starts.
  // For simplicity, let's keep it sync for now but it's risky.
}
void GameClient::set_color() { this->sendCommand("getColor"); }

void GameClient::listen() {
  while (running && socket.is_open()) {
    std::string msg = receiveMessage();
    if (msg.empty()) {
      if (running) {
        std::cerr << "Disconnected from server." << std::endl;
        running = false;
      }
      break;
    }

    if (msg.find("p:") == 0) {
      std::lock_guard<std::mutex> lock(players_mutex);
      std::string data = msg.substr(2);
      size_t start = 0;
      size_t end = data.find('|');

      std::vector<int> current_ids;

      while (end != std::string::npos) {
        std::string player_info = data.substr(start, end - start);
        size_t id_end = player_info.find(':');
        if (id_end != std::string::npos) {
          int id = std::stoi(player_info.substr(0, id_end));
          std::string pos_str = player_info.substr(id_end + 1);

          if (id != my_id) {
            current_ids.push_back(id);
            size_t first = pos_str.find('/');
            size_t last = pos_str.find_last_of('/');
            if (first != std::string::npos && last != std::string::npos &&
                first != last) {
              float x = std::stof(pos_str.substr(0, first));
              float y = std::stof(pos_str.substr(first + 1, last - first - 1));
              float z = std::stof(pos_str.substr(last + 1));

              bool found = false;
              for (auto &p : players) {
                if (p.id == id) {
                  p.Position = {x, y, z};
                  found = true;
                  break;
                }
              }
              if (!found) {
                Player new_player;
                new_player.id = id;
                new_player.Position = {x, y, z};
                new_player.color = {0, 255, 0}; // Other players are green
                players.push_back(new_player);
              }
            }
          }
        }
        start = end + 1;
        end = data.find('|', start);
      }

      // Remove players who left
      players.erase(std::remove_if(players.begin(), players.end(),
                                   [&](const Player &p) {
                                     if (p.id == my_id)
                                       return false;
                                     return std::find(current_ids.begin(),
                                                      current_ids.end(),
                                                      p.id) ==
                                            current_ids.end();
                                   }),
                    players.end());
    } else if (msg.find("s:") == 0) {
      unsigned int s = static_cast<unsigned int>(std::stoul(msg.substr(2)));
      SetSeed(s);
    } else if (msg.find("c:") == 0) {
      std::string buf = msg.substr(2);
      size_t f = buf.find(','), l = buf.find_last_of(',');
      if (f != std::string::npos && l != std::string::npos && f != l) {
        unsigned int r = std::stoul(buf.substr(0, f));
        unsigned int g = std::stoul(buf.substr(f + 1, l - f - 1));
        unsigned int b = std::stoul(buf.substr(l + 1));
        std::lock_guard<std::mutex> lock(players_mutex);
        if (!players.empty())
          players[0].color = Color{r, g, b};
      }
    }
  }
}
void GameClient::update_pos() {
  auto Player = get_players()[0];
  std::string Pos = "up:" + std::to_string(my_id) + ":" +
                    std::to_string(Player.Position.x) + "/" +
                    std::to_string(Player.Position.y) + "/" +
                    std::to_string(Player.Position.z);
  this->sendCommand(Pos);
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
    PlayerDirection.y -= 0.5f;
  } else {
    PlayerDirection.y = 0;
    if (move_up && JumpTimer > 0.1f) {
      PlayerDirection.y = JumpHeight * JumpPower;
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
      float verticalSpeed =
          SDL_clamp(playerDirection.y * deltaTime, -10, JumpHeight);
      nextY.y += verticalSpeed;
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
  bool OnGround =
      manager.RayCast(player.Position, {0, -1, 0}, bodyHeight + 0.15f).hit;
  PlayerInput(playerDirection, OnGround, inventorySlot, RotationDir, LeftClick,
              RightClick);
  PlayerRotation(player, RotationDir);
  PlayerMove(player, playerDirection, manager);
  PlayerBreackPlace(LeftClick, RightClick, manager, player, inventorySlot,
                    inventory);
}
void GameLoop(GameClient &game) {
  game.add_player({
      game.get_my_id(),
      {0, ChunkPrefab::ySize, 0},
      {0.0f, 0.0f, 0.0f},
      {255, 0, 0},
  });
  auto &p = game.get_players();
  game.set_seed();
  game.set_color();
  game.StartListener();

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
  float netTimer = 0.0f;

  while (game.GetRunning()) {
    netTimer += deltaTime;
    if (netTimer >= 0.05f) { // 20 updates per second
      game.update_pos();
      netTimer = 0.0f;
    }

    PlayerAction(p[0], inventorySlot, chunkManager, inventory);
    RendererObject.MainRenderLoop(inventory, inventorySlot, p);

    auto currentTime = std::chrono::high_resolution_clock::now();
    deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    deltaTime = SDL_clamp(deltaTime, 0.0f, 0.1f); // Max 100ms per frame
    lastTime = currentTime;
  }

  std::cout << "Exiting game..." << std::endl;
}

} // namespace BitMiner