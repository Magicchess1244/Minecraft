#include "../../include/client/PlayerManager.hpp"
#include "../../include/client/GameManager.hpp"

constexpr float mouseSensitivity = 0.1f;
constexpr float playerSpeed = 5.0f;
constexpr float sideSpeed = 0.75f;
constexpr float jumpHeight = 1.5f;
constexpr float jumpPower = 5.0f;
constexpr float coyeteTimer = 0.25f;
float jumpTimer = 0;
constexpr float bodyHeight = 1.6f;
constexpr float bounds = 0.3f;

Vector3 playerDirection = {0, 0, 0};
constexpr bool playerColistion = true;

PlayerManager::PlayerManager(GameManager &manager) : gameManager(manager), players(this->gameManager.GetGameClient().get_players()){
  for (int i = 0; i < 9; i++) inventory.push_back(Slot{});
  PlayerInit();
}

void PlayerManager::PlayerInit(){
  GameClient &gameClient = this->gameManager.GetGameClient();
  int myId = gameClient.get_my_id();
  Player localPlayer{};
  localPlayer.id = myId;
  localPlayer.name = gameClient.get_my_name();
  localPlayer.Position = {0, ChunkPrefab::ySize, 0};
  localPlayer.color = Color::GetColor(
      static_cast<PlayerColor>(myId % static_cast<int>(PlayerColor::COUNT)));

  gameClient.add_player(localPlayer);
}

int PlayerManager::FindSlot(std::vector<Slot> &Inventory, short Type, bool isEntity) {
  int index = 0;
  if (Type == 0) return -1;
  
  for (Slot slot : Inventory) {
    if ((slot.Type == Type && slot.isEntity == isEntity || slot.Type == 0) &&
        slot.Amount < 64) {
      return index;
    }
    index++;
  }
  return -1;
}

void PlayerManager::PlayerInput(Vector3 &playerDirection, int &inventorySlots, Vector3 &playerRot, bool &leftClick, bool &rightClick) {
  const bool *KeyboardState = SDL_GetKeyboardState(NULL);
  const bool move_foward = KeyboardState[SDL_SCANCODE_W] || KeyboardState[SDL_SCANCODE_UP];
  const bool move_backward = KeyboardState[SDL_SCANCODE_S] || KeyboardState[SDL_SCANCODE_DOWN];
  const bool move_up = KeyboardState[SDL_SCANCODE_SPACE];
  const bool move_down = KeyboardState[SDL_SCANCODE_LSHIFT];
  const bool move_left = KeyboardState[SDL_SCANCODE_A] || KeyboardState[SDL_SCANCODE_LEFT];
  const bool move_right = KeyboardState[SDL_SCANCODE_D] || KeyboardState[SDL_SCANCODE_RIGHT];

  playerDirection.x = 0;
  playerDirection.z = 0;

  float mouseX = 0, mouseY = 0;
  Uint32 mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

  leftClick = (mouseState & SDL_BUTTON_LMASK) != 0;
  rightClick = (mouseState & SDL_BUTTON_RMASK) != 0;
  
  playerRot.y = mouseX;
  playerRot.x = mouseY;
  playerRot.z = 0;

  if (move_left || move_right) {
    playerDirection.x = move_left ? -1 : 1;
  }
  if (move_backward || move_foward) {
    playerDirection.z = move_backward ? -1 : 1;
  }

  if (OnWater()) {
    if (move_up) playerDirection.y = jumpHeight * jumpPower;
    else if (move_down) playerDirection.y -= 2.0f;
    else playerDirection.y = -2.5f;
  } else {
    if (OnGround()) {
      if (move_up) playerDirection.y = jumpHeight * jumpPower;
      else playerDirection.y = 0;
    } else {
      if (move_up && (jumpTimer < coyeteTimer || !playerColistion)) playerDirection.y = jumpHeight * jumpPower;
      else playerDirection.y -= (playerColistion) ? 0.5f : 0;
    } 
  }

  for (int i = 0; i < 9; ++i) {
    if (KeyboardState[SDL_SCANCODE_1 + i]) {
      inventorySlots = i;
      break;
    }
  }
}

void PlayerManager::PlayerRotation(Vector3 rotationDir) {
  if (rotationDir.x != 0 || rotationDir.y != 0) {
    this->players[0].Rotation.y += rotationDir.y * mouseSensitivity;
    this->players[0].Rotation.x += rotationDir.x * mouseSensitivity;
    this->players[0].Rotation.x = SDL_clamp(this->players[0].Rotation.x, -90.0f, 90.0f);
    if (this->players[0].Rotation.y > 360.0f)
      this->players[0].Rotation.y -= 360.0f;
    if (this->players[0].Rotation.y < 0.0f)
      this->players[0].Rotation.y += 360.0f;
  }
}

void PlayerManager::PlayerMove(Vector3 playerDirection, float deltaTime) {
  auto isColliding = [&](Vector3 pos) {
    if (!playerColistion)
      return false;

    float yChecks[] = {-1.4f, -0.8f, 0.1f};
    float xzChecks[] = {-bounds, bounds};
    ChunkManager& chunkManager = this->gameManager.GetChunkManager();

    for (float yOff : yChecks) {
      for (float xOff : xzChecks) {
        for (float zOff : xzChecks) {
          if (chunkManager.IsSolid(pos + Vector3(xOff, yOff, zOff))) {
            return true;
          }
        }
      }
    }
    return false;
  };

  if (playerDirection == Vector3(0,0,0)) return;

  if (playerDirection.y != 0) {
    Vector3 nextY = this->players[0].Position;
    nextY.y += SDL_clamp(playerDirection.y * deltaTime, -10, jumpHeight);
    if (!isColliding(nextY)) this->players[0].Position.y = nextY.y;
  }

  Vector3 Rot = this->players[0].Rotation;
  Rot.x = 0;

  Vector3 Dir = Rot.Forward() * playerDirection.z + Rot.Right() * playerDirection.x * sideSpeed;

  if (Dir.LengthSquared() < 0.001f) return;

  Dir = Dir.Normalized() * playerSpeed * deltaTime;

  Vector3 nextX = this->players[0].Position;
  nextX.x += Dir.x;
  if (!isColliding(nextX)) this->players[0].Position.x = nextX.x;

  Vector3 nextZ = this->players[0].Position;
  nextZ.z += Dir.z;
  if (!isColliding(nextZ)) this->players[0].Position.z = nextZ.z;
}

void PlayerManager::PlayerBreackPlace(bool left, bool right) {
  if (!(left || right)) return;

  ChunkManager& chunkManager = this->gameManager.GetChunkManager();

  RaycastResult ray = chunkManager.RayCast(this->players[0].Position, this->players[0].Rotation.Forward(), 7);
  
  if (!ray.hit) return;

  if (left) Break(ray);
  else if (!BlockDef[ray.BlockID].hasUI()) Place(ray);
  //else this->gameManager.SetUi(ray.BlockID);
  //FIX need to show ui when right click ui blocks
}

void PlayerManager::Break(RaycastResult ray){
  if (ray.BlockID == 4) return;

  ChunkManager& chunkManager = this->gameManager.GetChunkManager();

  chunkManager.Place(ray.pos, 0);
  std::string mod = "mod:" + std::to_string(ray.pos.x) + "/" + std::to_string(ray.pos.y) + "/" + std::to_string(ray.pos.z) + ":0";

  int slotIdx = FindSlot(this->inventory, BlockDef[ray.BlockID].Drop, false);
  if (slotIdx != -1) {
    if (this->inventory[slotIdx].Amount == 0) {
      this->inventory[slotIdx].Type = BlockDef[ray.BlockID].Drop;
      this->inventory[slotIdx].isEntity = false;
    }
    this->inventory[slotIdx].Amount++;
  }
  GameClient& gameClient = this->gameManager.GetGameClient();
  gameClient.sendCommand(mod);
  gameClient.sync_inventory();
}

void PlayerManager::Place(RaycastResult ray){
  if (this->inventory[inventorySlot].Amount == 0 && this->inventory[inventorySlot].isEntity && !BlockDef[inventory[inventorySlot].Type].CanPlace()) return;

  Uint8 type = this->inventory[inventorySlot].Type;
  this->inventory[inventorySlot].Amount--;
  if (this->inventory[inventorySlot].Amount == 0) this->inventory[inventorySlot].Type = 0;
  this->gameManager.GetChunkManager().Place(ray.pos, type);

  std::string mod = "mod:" + std::to_string((int)ray.pos.x) + "/" + std::to_string((int)ray.pos.y) + "/" + std::to_string((int)ray.pos.z) + ":" + std::to_string(type);

  GameClient& gameClient = this->gameManager.GetGameClient();
  gameClient.sendCommand(mod);
  gameClient.sync_inventory();
}

void PlayerManager::PlayerAction(int inventorySlot, float deltaTime) {

  static bool hasInitialChunkLoaded = false;
  if (!hasInitialChunkLoaded) {
    Vector3 chunkKey;
    chunkKey.x = std::floor(this->players[0].Position.x / ChunkPrefab::xSize);
    chunkKey.y = 0;
    chunkKey.z = std::floor(this->players[0].Position.z / ChunkPrefab::zSize);

    ChunkManager& chunkManager = this->gameManager.GetChunkManager();
    if (!chunkManager.get_chunk(chunkKey).isGenerated) {
      return;
    }
    hasInitialChunkLoaded = true;
    GameClient& gameClient = this->gameManager.GetGameClient();
    if (gameClient.get_is_new_player()) {
      int height = chunkManager.get_chunk(chunkKey).GetHeight(
          {this->players[0].Position.x, this->players[0].Position.z});
      this->players[0].Position.x = std::floor(this->players[0].Position.x) + 0.5f;
      this->players[0].Position.z = std::floor(this->players[0].Position.z) + 0.5f;
      this->players[0].Position.y = height + 2.5f;
      gameClient.set_is_new_player(false);
    }
  }

  if (this->gameManager.GetUsingUI()) return;

  jumpTimer += deltaTime;
  Vector3 rotationDir = {0, 0, 0};
  bool leftClick = false, RightClick = false;
  
  PlayerInput(playerDirection, inventorySlot, rotationDir, leftClick, RightClick);
  PlayerRotation(rotationDir);
  PlayerMove(playerDirection, deltaTime);
  PlayerBreackPlace(leftClick, RightClick);
}

bool PlayerManager::OnGround(){
  ChunkManager& chunkManager = this->gameManager.GetChunkManager();
  return chunkManager.RayCast(this->players[0].Position, {0, -1, 0}, bodyHeight + 0.15f).hit ||
  chunkManager.RayCast(this->players[0].Position + Vector3(bounds, 0, 0), {0, -1, 0}, bodyHeight + 0.15f).hit ||
  chunkManager.RayCast(this->players[0].Position + Vector3(-bounds, 0, 0), {0, -1, 0}, bodyHeight + 0.15f).hit ||
  chunkManager.RayCast(this->players[0].Position + Vector3(0, 0, bounds), {0, -1, 0}, bodyHeight + 0.15f).hit ||
  chunkManager.RayCast(this->players[0].Position + Vector3(0, 0, -bounds), {0, -1, 0}, bodyHeight + 0.15f).hit;
}

bool PlayerManager::OnWater(){
  ChunkManager& chunkManager = this->gameManager.GetChunkManager();
  return (chunkManager.GetBlockID(this->players[0].Position - Vector3(0, 0.8f, 0)) == 5) ||
    (chunkManager.GetBlockID(this->players[0].Position) == 5);
}