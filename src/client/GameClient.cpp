#include "../../include/client/GameClient.hpp"
#include "../../include/common/PerlinNoise.hpp"
#include <ostream>

constexpr float mouseSensitivity = 0.1f;
constexpr float playerSpeed = 0.05f;

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
                 Vector3 &PlayerRot) {
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
  PlayerDirection.y = 0;
  PlayerDirection.z = 0;

  // Get mouse movement
  float mouseX, mouseY;
  SDL_GetRelativeMouseState(&mouseX, &mouseY);

  // Apply mouse movement to rotation
  // Mouse X controls yaw (Y-axis rotation)
  // Mouse Y controls pitch (X-axis rotation)
  PlayerRot.y = mouseX; // Yaw (left/right)
  PlayerRot.x = mouseY; // Pitch (up/down)
  PlayerRot.z = 0;      // Roll (not used for FPS camera)

  if (move_left || move_right) {
    PlayerDirection.x = move_left ? -1 : 1;
  }
  if (move_down || move_up) {
    PlayerDirection.y = move_down ? -1 : 1;
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
void PlayerMovement(Player &player, int &inventorySlot) {
  Vector3 playerDirection = {0, 0, 0};
  Vector3 RotationDir = {0, 0, 0};

  PlayerInput(playerDirection, true, inventorySlot, RotationDir);

  if (RotationDir.x != 0 || RotationDir.y != 0) {

    player.Rotation.y += RotationDir.y * mouseSensitivity;
    player.Rotation.x += RotationDir.x * mouseSensitivity;
    player.Rotation.x = SDL_clamp(player.Rotation.x, -90.0f, 90.0f);
    if (player.Rotation.y > 360.0f)
      player.Rotation.y -= 360.0f;
    if (player.Rotation.y < 0.0f)
      player.Rotation.y += 360.0f;
  }

  // Handle movement
  if (playerDirection.x != 0 || playerDirection.y != 0 ||
      playerDirection.z != 0) {
    float deltaTime = 1.0f;
    player.Position.y += playerDirection.y * playerSpeed * deltaTime * 2;

    Vector3 Rot = player.Rotation;
    Rot.x = 0;

    Vector3 Dir =
        Rot.Forward() * playerDirection.z + Rot.Right() * playerDirection.x;

    Dir = Dir.Normalized();

    player.Position.x += Dir.x * playerSpeed * deltaTime;
    player.Position.z += Dir.z * playerSpeed * deltaTime;
  }
}
void GameLoop(GameClient &game) {
  game.add_player({
      {0, 0, 0},
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
  Renderer RendererObject(game);

  while (game.GetRunning()) {
    PlayerMovement(p[0], inventorySlot);
    RendererObject.MainRenderLoop(inventory, inventorySlot, p);
  }

  std::cout << "Exiting game..." << std::endl;
}
} // namespace BitMiner