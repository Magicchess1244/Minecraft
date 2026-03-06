#include "../../include/client/GameClient.hpp"
#include "../../include/common/PerlinNoise.hpp"
#include <SDL3/SDL_stdinc.h>
#include <algorithm>
#include <chrono>
#include <ostream>
#include <sstream>
#include <string>

// ─── Constants ──────────────────────────────────────────────────────────────

constexpr float mouseSensitivity = 0.1f;
constexpr float playerSpeed = 5.0f;
constexpr float JumpHeight = 1.5f;
constexpr float JumpPower = 5.0f;
float deltaTime = 1.0f;
float JumpTimer = 0;
float bodyHeight = 1.6f;
Vector3 playerDirection = {0, 0, 0};
constexpr bool PLayerColistion = true;

// ─── Seed / Color commands ──────────────────────────────────────────────────

void GameClient::set_seed() {
  // Seed is now received in the constructor.
}
void GameClient::set_color() { this->sendCommand("getColor"); }

// ─── Helper: parse "x/y/z" into a Vector3 ──────────────────────────────────

static Vector3 parseVec3(const std::string &s) {
  auto parts = split(s, '/');
  if (parts.size() != 3)
    return {0, 0, 0};
  return {std::stof(parts[0]), std::stof(parts[1]), std::stof(parts[2])};
}

// ─── Message Handlers ───────────────────────────────────────────────────────

void GameClient::handlePlayersBroadcast(const std::string &msg) {
  // Format: p:<id>:<x/y/z>|<id>:<x/y/z>|...
  std::string data = msg.substr(2);
  auto entries = split(data, '|');

  std::lock_guard<std::mutex> lock(players_mutex);
  std::vector<int> current_ids;

  for (const auto &entry : entries) {
    if (entry.empty())
      continue;

    auto parts = split(entry, ':');
    if (parts.size() < 2)
      continue;

    int id = std::stoi(parts[0]);
    if (id == my_id)
      continue;

    current_ids.push_back(id);
    Vector3 pos = parseVec3(parts[1]);

    // Update existing player or add new one
    bool found = false;
    for (auto &p : players) {
      if (p.id == id) {
        p.Position = pos;
        found = true;
        break;
      }
    }
    if (!found) {
      Player new_player;
      new_player.id = id;
      new_player.Position = pos;
      new_player.color = Color::GetColor(
          static_cast<PlayerColor>(id % static_cast<int>(PlayerColor::COUNT)));
      players.push_back(new_player);
    }
  }

  // Remove players who left
  players.erase(std::remove_if(players.begin(), players.end(),
                               [&](const Player &p) {
                                 if (p.id == my_id)
                                   return false;
                                 return std::find(current_ids.begin(),
                                                  current_ids.end(),
                                                  p.id) == current_ids.end();
                               }),
                players.end());
}

void GameClient::handleSeedMsg(const std::string &msg) {
  // Format: s:<seed>
  unsigned int s = static_cast<unsigned int>(std::stoul(msg.substr(2)));
  SetSeed(s);
}

void GameClient::handleColorMsg(const std::string &msg) {
  // Format: c:r,g,b
  auto rgb = split(msg.substr(2), ',');
  if (rgb.size() < 3)
    return;

  unsigned int r = std::stoul(rgb[0]);
  unsigned int g = std::stoul(rgb[1]);
  unsigned int b = std::stoul(rgb[2]);

  std::lock_guard<std::mutex> lock(players_mutex);
  if (!players.empty())
    players[0].color = Color{static_cast<Uint8>(r), static_cast<Uint8>(g),
                             static_cast<Uint8>(b)};
}

void GameClient::handleBlockModification(const std::string &msg) {
  // Format: bm:x/y/z:type
  auto parts = split(msg, ':');
  if (parts.size() < 3)
    return;

  try {
    Vector3 pos = parseVec3(parts[1]);
    Uint8 type = static_cast<Uint8>(std::stoi(parts[2]));
    pending_mods.push_back({pos, type});
  } catch (...) {
  }
}

void GameClient::handlePlayerData(const std::string &msg) {
  // Format: pData:x/y/z|rx/ry/rz|type,amount,isEntity|type,amount,isEntity|...
  std::string data = msg.substr(6);
  auto sections = split(data, '|');
  if (sections.size() < 2)
    return;

  std::lock_guard<std::mutex> lock(players_mutex);
  if (players.empty())
    return;

  // Section 0: position, Section 1: rotation, Section 2+: inventory slots
  players[0].Position = parseVec3(sections[0]);
  players[0].Rotation = parseVec3(sections[1]);

  // Parse inventory slots (sections[2] onward)
  int slotIdx = 0;
  for (size_t i = 2;
       i < sections.size() && slotIdx < (int)players[0].inventory.size(); ++i) {
    if (sections[i].empty())
      continue;

    auto fields = split(sections[i], ',');
    if (fields.size() >= 3) {
      players[0].inventory[slotIdx].Type =
          static_cast<short>(std::stoi(fields[0]));
      players[0].inventory[slotIdx].Amount =
          static_cast<short>(std::stoi(fields[1]));
      players[0].inventory[slotIdx].isEntity = (fields[2] == "1");
    }
    slotIdx++;
  }
}

// ─── Network Listener ───────────────────────────────────────────────────────

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

    // Dispatch to the appropriate handler
    if (msg.find("p:") == 0) {
      handlePlayersBroadcast(msg);
    } else if (msg.find("s:") == 0) {
      handleSeedMsg(msg);
    } else if (msg.find("c:") == 0) {
      handleColorMsg(msg);
    } else if (msg.find("bm:") == 0) {
      handleBlockModification(msg);
    } else if (msg.find("pData:") == 0) {
      handlePlayerData(msg);
    }
  }
}

// ─── Outgoing Messages ─────────────────────────────────────────────────────

void GameClient::update_pos() {
  auto Player = get_players()[0];
  std::string Pos = "up:" + std::to_string(my_id) + ":" +
                    std::to_string(Player.Position.x) + "/" +
                    std::to_string(Player.Position.y) + "/" +
                    std::to_string(Player.Position.z) + ":" +
                    std::to_string(Player.Rotation.x) + "/" +
                    std::to_string(Player.Rotation.y) + "/" +
                    std::to_string(Player.Rotation.z);
  this->sendCommand(Pos);
}

void GameClient::sync_inventory() {
  auto &p = get_players()[0];
  std::string msg = "inv:";
  for (const auto &slot : p.inventory) {
    msg += std::to_string(slot.Type) + "," + std::to_string(slot.Amount) + "," +
           (slot.isEntity ? "1" : "0") + "|";
  }
  this->sendCommand(msg);
}

// ─── Game Logic (BitMiner namespace) ────────────────────────────────────────

namespace BitMiner {

int FindSlot(std::vector<Slot> &Inventory, short Type, bool isEntity) {
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

void PlayerInput(Vector3 &PlayerDirection, bool OnGround, bool InWater,
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

// ─── Movement & Collision ───────────────────────────────────────────────────

void PlayerMove(Player &player, Vector3 playerDirection,
                ChunkManager &manager) {
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
      Vector3 nextY = player.Position;
      float verticalSpeed =
          SDL_clamp(playerDirection.y * deltaTime, -10, JumpHeight);
      nextY.y += verticalSpeed;
      if (!isColliding(nextY)) {
        player.Position.y = nextY.y;
      }
    }

    // Horizontal movement
    Vector3 Rot = player.Rotation;
    Rot.x = 0;
    Vector3 Dir = Rot.Forward() * playerDirection.z +
                  Rot.Right() * playerDirection.x * 0.75f;

    if (Dir.LengthSquared() > 0.0001f) {
      Dir = Dir.Normalized() * playerSpeed * deltaTime;

      // Try X axis
      Vector3 nextX = player.Position;
      nextX.x += Dir.x;
      if (!isColliding(nextX)) {
        player.Position.x = nextX.x;
      }

      // Try Z axis
      Vector3 nextZ = player.Position;
      nextZ.z += Dir.z;
      if (!isColliding(nextZ)) {
        player.Position.z = nextZ.z;
      }
    }
  }
}

// ─── Block Break / Place ────────────────────────────────────────────────────

void PlayerBreackPlace(bool Left, bool Right, ChunkManager &manager,
                       Player &player, int inventorySlot,
                       std::vector<Slot> &inventory, GameClient &game,
                       Renderer *renderer) {
  static bool lastLeft = false;
  static bool lastRight = false;

  bool justLeft = Left && !lastLeft;
  bool justRight = Right && !lastRight;

  lastLeft = Left;
  lastRight = Right;

  if (justLeft || justRight) {
    RaycastResult Ray =
        manager.RayCast(player.Position, player.Rotation.Forward(), 7);
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
        game.sendCommand(mod);
        game.sync_inventory();
      } else if (!BlockDef[Ray.BlockID].hasUI()) {
        Vector3 placePos = Ray.prevPos;
        bool collides = false;

        for (float yOff = -1.5f; yOff <= 0.1f; yOff += 0.5f) {
          Vector3 check = (player.Position + Vector3(0, yOff, 0)).Truncate();
          if (check == placePos) {
            collides = true;
            break;
          }
        }

        if (!collides && inventory[inventorySlot].Amount > 0 &&
            !inventory[inventorySlot].isEntity &&
            BlockDef[inventory[inventorySlot].Type].canPlace) {
          Uint8 type = inventory[inventorySlot].Type;
          inventory[inventorySlot].Amount--;
          if (inventory[inventorySlot].Amount == 0)
            inventory[inventorySlot].Type = 0;
          manager.Place(placePos, type);
          std::string mod = "mod:" + std::to_string((int)placePos.x) + "/" +
                            std::to_string((int)placePos.y) + "/" +
                            std::to_string((int)placePos.z) + ":" +
                            std::to_string(type);
          game.sendCommand(mod);
          game.sync_inventory();
        }
      } else {
        renderer->SetUi(Ray.BlockID);
      }
    }
  }
}

// ─── Player Action (per-frame) ──────────────────────────────────────────────

void PlayerAction(Player &player, int &inventorySlot, ChunkManager &manager,
                  std::vector<Slot> &inventory, GameClient &game,
                  Renderer *renderer) {

  JumpTimer += deltaTime;
  Vector3 RotationDir = {0, 0, 0};
  bool LeftClick = false, RightClick = false;
  float bounds = 0.3f;
  bool OnGround =
      manager.RayCast(player.Position, {0, -1, 0}, bodyHeight + 0.15f).hit ||
      manager
          .RayCast(player.Position + Vector3(bounds, 0, 0), {0, -1, 0},
                   bodyHeight + 0.15f)
          .hit ||
      manager
          .RayCast(player.Position + Vector3(-bounds, 0, 0), {0, -1, 0},
                   bodyHeight + 0.15f)
          .hit ||
      manager
          .RayCast(player.Position + Vector3(0, 0, bounds), {0, -1, 0},
                   bodyHeight + 0.15f)
          .hit ||
      manager
          .RayCast(player.Position + Vector3(0, 0, -bounds), {0, -1, 0},
                   bodyHeight + 0.15f)
          .hit;

  // Check if player is in water
  player.Inwater = false;
  try {
    player.Inwater =
        (manager.GetBlockID(player.Position - Vector3(0, 0.8f, 0)) == 5) ||
        (manager.GetBlockID(player.Position) == 5);
  } catch (...) {
    player.Inwater = false;
  }
  if (renderer->UsingUI())
    return;

  PlayerInput(playerDirection, OnGround, player.Inwater, inventorySlot,
              RotationDir, LeftClick, RightClick);
  PlayerRotation(player, RotationDir);
  PlayerMove(player, playerDirection, manager);
  PlayerBreackPlace(LeftClick, RightClick, manager, player, inventorySlot,
                    inventory, game, renderer);
}

// ─── Main Game Loop ─────────────────────────────────────────────────────────

void GameLoop(GameClient &game) {
  int myId = game.get_my_id();
  Player localPlayer;
  localPlayer.id = myId;
  localPlayer.name = game.get_my_name();
  localPlayer.Position = {0, ChunkPrefab::ySize, 0};
  localPlayer.color = Color::GetColor(
      static_cast<PlayerColor>(myId % static_cast<int>(PlayerColor::COUNT)));

  game.add_player(localPlayer);
  auto &p = game.get_players();
  game.set_seed();
  game.set_color();
  game.sendCommand("gm"); // Request all world modifications
  game.StartListener();

  // Wait a bit for server data if any
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  int inventorySlot = 0;

  ChunkManager chunkManager;
  Renderer RendererObject(game, chunkManager);

  auto lastTime = std::chrono::high_resolution_clock::now();
  float netTimer = 0.0f;
  float invTimer = 0.0f;

  while (game.GetRunning()) {
    // Process pending mods from server
    {
      std::lock_guard<std::mutex> lock(game.get_mutex());
      auto &mods = game.get_pending_mods();
      for (auto const &m : mods) {
        chunkManager.Place(m.pos, m.type);
      }
      mods.clear();
    }

    netTimer += deltaTime;
    invTimer += deltaTime;
    static float waterTimer = 0.0f;
    waterTimer += deltaTime;

    if (netTimer >= 0.05f) { // 20 updates per second
      game.update_pos();
      netTimer = 0.0f;
    }

    if (invTimer >= 2.0f) { // Sync inventory every 2 seconds
      game.sync_inventory();
      invTimer = 0.0f;
    }

    if (waterTimer >= 0.2f) { // 5 water ticks per second
      chunkManager.TickWater();
      waterTimer = 0.0f;
    }

    PlayerAction(p[0], inventorySlot, chunkManager, p[0].inventory, game,
                 &RendererObject);
    RendererObject.MainRenderLoop(p[0].inventory, inventorySlot, p);

    auto currentTime = std::chrono::high_resolution_clock::now();
    deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
    deltaTime = SDL_clamp(deltaTime, 0.0f, 0.1f); // Max 100ms per frame
    lastTime = currentTime;
  }

  std::cout << "Exiting game..." << std::endl;
}

} // namespace BitMiner