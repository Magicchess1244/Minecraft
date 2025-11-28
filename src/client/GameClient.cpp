#include "../../include/client/GameClient.hpp"
#include "../../include/common/PerlinNoise.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <ostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void GameClient::MakeClient() {
  int PORT = 8080;
  const char *SERVER_IP = "127.0.0.1";

  while (true) {
    this->server = socket(AF_INET, SOCK_STREAM, 0);
    if (this->server == -1) {
      std::cerr << "Failed to create socket.\n";
      sleep(1);
      continue;
    }

    sockaddr_in serverAddr = {0};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
      std::cerr << "Invalid address/ Address not supported \n";
      close(this->server);
      return;
    }

    std::cout << "Attempting to connect to server...\n";
    if (connect(this->server, (sockaddr *)&serverAddr, sizeof(serverAddr)) <
        0) {
      std::cerr << "Connection Failed. Retrying in 2 seconds...\n";
      close(this->server);
      sleep(2);
      continue;
    }

    std::cout << "Connected to the server.\n";
    break;
  }
}

constexpr float mouseSensitivity = 0.01f;
constexpr float playerSpeed = 0.1f;

void GameClient::set_seed() {
  int res;
  char buf[16];
  unsigned int server_seed = 0;
  const char *command = "seed";
  std::cout << "seed" << std::endl;
  res = send(this->server, command, strlen(command) + 1, 0);
  if (res <= 0)
    std::cerr << "Error requesting seed from the server.";
  res = recv(this->server, buf, sizeof(buf), 0);
  if (res <= 0)
    std::cerr << "Error receiving seed from the server.";
  std::cout << "buf: " << buf << std::endl;
  server_seed = static_cast<unsigned int>(std::stoi(std::string(buf)));
  std::cout << "New client seed: " << server_seed << std::endl;
  srand(server_seed);
  this->seed = server_seed;
}
void GameClient::set_color() {
  int res;
  char buf[11];
  unsigned int server_color[3] = {};
  const char *command = "getColor";

  std::cout << "getColor" << std::endl;

  res = send(this->server, command, strlen(command) + 1, 0);
  if (res <= 0)
    std::cerr << "Error requesting color from the server.";
  res = recv(this->server, buf, sizeof(buf), 0);
  if (res <= 0)
    std::cerr << "Error receiving color from the server.";
  else
    std::cout << "recived" << std::endl;

  std::cout << "color" << std::endl;
  int i = 0;
  // Split string manually or use a helper if available. The original code used
  // split(buf, ",") but split is not defined in this file. I will assume split
  // is available or implement a simple parser. Looking at imports,
  // PerlinNoise.hpp is included. Maybe split is in Common.hpp? I'll check
  // Common.hpp for split. If not, I'll implement it or use sscanf. Actually,
  // sscanf is easier.
  sscanf(buf, "%u,%u,%u", &server_color[0], &server_color[1], &server_color[2]);

  std::cout << "New client color: " << server_color[0] << "," << server_color[1]
            << "," << server_color[2] << std::endl;

  std::cout << "players online: " << this->players.size() << std::endl;
  if (this->players.size() > 0) {
    this->players[0].color =
        Color{server_color[0], server_color[1], server_color[2]};
  }
}

void GameClient::RequestChunk(int x, int z) {
  std::string command =
      "getChunk " + std::to_string(x) + " " + std::to_string(z);
  send(this->server, command.c_str(), command.size() + 1, 0);
  // std::cout << "Requested chunk " << x << ", " << z << std::endl;
}

void GameClient::ReceiveChunks(ChunkManager &chunkManager) {
  char buf[256];

  // Non-blocking check for header
  ssize_t res =
      recv(this->server, buf, sizeof(buf) - 1, MSG_DONTWAIT | MSG_PEEK);

  if (res > 0) {
    buf[res] = 0;
    if (strncmp(buf, "CHUNK", 5) == 0) {
      // Consume the header
      int x, z;
      size_t size;
      // Find newline
      char *newline = strchr(buf, '\n');
      if (newline) {
        *newline = 0;
        sscanf(buf, "CHUNK %d %d %zu", &x, &z, &size);

        // Consume header from socket
        recv(this->server, buf, (newline - buf) + 1, 0);

        // Now read the body
        std::vector<int> blockData(size / sizeof(int));
        size_t totalReceived = 0;
        char *dataPtr = (char *)blockData.data();

        while (totalReceived < size) {
          res = recv(this->server, dataPtr + totalReceived,
                     size - totalReceived, 0); // Blocking for body
          if (res <= 0)
            break;
          totalReceived += res;
        }

        if (totalReceived == size) {
          Vector3 key = {(float)x, 0, (float)z};
          ChunkPrefab &chunk = chunkManager.get_chunk(key);
          chunk.Blocks = blockData;
          chunk.RebuildMesh(); // Important: Rebuild mesh after receiving data
          chunk.isLoaded = true;
          std::cout << "Received chunk " << x << ", " << z << std::endl;
        }
      }
    } else {
      // Consume unknown data to avoid stuck loop
      recv(this->server, buf, res, 0);
    }
  }
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
  PlayerRot.y = -mouseX; // Yaw (left/right)
  PlayerRot.x = -mouseY; // Pitch (up/down)
  PlayerRot.z = 0;       // Roll (not used for FPS camera)

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
    player.Position.y += playerDirection.y * playerSpeed * deltaTime;

    Vector3 Dir = player.Rotation.Forward() * playerDirection.z +
                  player.Rotation.Right() * playerDirection.x;

    Dir = Dir.Normalized();

    player.Position.x += Dir.x * playerSpeed * deltaTime;
    player.Position.z += Dir.z * playerSpeed * deltaTime;
  }
}
void GameLoop(GameClient &game) {
  game.add_player({
      {0, 50, 0},
      {0.0f, 0.0f, 0.0f},
      {255, 0, 0},
  });
  auto p = game.get_players();

  game.MakeClient();
  game.set_seed();
  game.set_color();
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