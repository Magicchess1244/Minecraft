#include "../../include/client/GameClient.hpp"
#include "../../include/common/PerlinNoise.hpp"

constexpr float mouseSensitivity = 0.1f;
constexpr float playerSpeed = 2.0f;

std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end;

    while ((end = s.find(delimiter, start)) != std::string::npos) {
        tokens.push_back(s.substr(start, end - start));
        start = end + delimiter.length();
    }
    tokens.push_back(s.substr(start));
    return tokens;
}

void GameClient::set_seed() {
    if (!networked_mode || !network) {
        // In standalone mode, generate a random seed
        this->seed = static_cast<unsigned int>(time(0));
        srand(this->seed);
        std::cout << "Standalone mode - Generated seed: " << this->seed << std::endl;
        return;
    }

    // Networked mode - request seed from server
    NetworkMessage request(MessageType::SEED_REQUEST, "");
    if (!network->send_message(request)) {
        std::cerr << "Failed to send seed request" << std::endl;
        return;
    }

    NetworkMessage response;
    if (!network->receive_message(response) || response.type != MessageType::SEED_RESPONSE) {
        std::cerr << "Failed to receive seed response" << std::endl;
        return;
    }

    std::string seed_str(response.data.begin(), response.data.end());
    unsigned int server_seed = static_cast<unsigned int>(std::stoi(seed_str));
    std::cout << "Received seed from server: " << server_seed << std::endl;
    srand(server_seed);
    this->seed = server_seed;
}
void GameClient::set_color() {
    if (!networked_mode || !network) {
        // In standalone mode, assign a default color
        if (this->players.size() > 0) {
            this->players[0].color = Color{255, 0, 0}; // Red color for standalone
            std::cout << "Standalone mode - Assigned color: Red" << std::endl;
        }
        return;
    }

    // Networked mode - request color from server
    NetworkMessage request(MessageType::COLOR_REQUEST, "");
    if (!network->send_message(request)) {
        std::cerr << "Failed to send color request" << std::endl;
        return;
    }

    NetworkMessage response;
    if (!network->receive_message(response) || response.type != MessageType::COLOR_RESPONSE) {
        std::cerr << "Failed to receive color response" << std::endl;
        return;
    }

    std::string color_str(response.data.begin(), response.data.end());
    std::vector<std::string> color_parts = split(color_str, ",");
    
    if (color_parts.size() >= 3 && this->players.size() > 0) {
        unsigned int r = std::stoi(color_parts[0]);
        unsigned int g = std::stoi(color_parts[1]);
        unsigned int b = std::stoi(color_parts[2]);
        
        this->players[0].color = Color{r, g, b};
        std::cout << "Received color from server: " << r << "," << g << "," << b << std::endl;
    }
}

// UI function
namespace BitMiner {
int FindSlot(const std::vector<Slot>& Inventory, short Type) {
    int index = 0;
    for (Slot slot : Inventory) {
        if ((slot.Type == Type || slot.Type == 0) && slot.Amount < 64) {
            std::cout << "Found slot" << index << std::endl;
            return index;
        }
        index++;
    }
    std::cout << "Inventory full, cannot add item of type: " << Type << std::endl;
    return -1;
}

void PlayerInput(Vector3& PlayerDirection, bool OnGround, int& InventorySlots, Vector3& PlayerRot) {
    const bool* KeyboardState = SDL_GetKeyboardState(NULL);
    const bool move_foward = (KeyboardState[SDL_SCANCODE_W] || KeyboardState[SDL_SCANCODE_UP]);
    const bool move_backward = (KeyboardState[SDL_SCANCODE_S] || KeyboardState[SDL_SCANCODE_DOWN]);
    const bool move_up = KeyboardState[SDL_SCANCODE_SPACE];
    const bool move_down = KeyboardState[SDL_SCANCODE_LSHIFT];
    const bool move_left = KeyboardState[SDL_SCANCODE_A] || KeyboardState[SDL_SCANCODE_LEFT];
    const bool move_right = KeyboardState[SDL_SCANCODE_D] || KeyboardState[SDL_SCANCODE_RIGHT];

    // FIXME: no es fa servir !!
    (void)(void*)OnGround;

    PlayerDirection.x = 0;
    PlayerDirection.y = 0;
    PlayerDirection.z = 0;

    // Get mouse movement
    float mouseX, mouseY;
    SDL_GetRelativeMouseState(&mouseX, &mouseY);

    // Apply mouse movement to rotation
    // Mouse X controls yaw (Y-axis rotation)
    // Mouse Y controls pitch (X-axis rotation)
    PlayerRot.y = -mouseX;  // Yaw (left/right)
    PlayerRot.x = mouseY;   // Pitch (up/down)
    PlayerRot.z = 0;        // Roll (not used for FPS camera)

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

    const Uint32 MouseState = SDL_GetMouseState(NULL, NULL);
    const bool left_click = (MouseState & SDL_BUTTON_LMASK);
    const bool right_click = (MouseState & SDL_BUTTON_RMASK);

    if (right_click) {
        std::cout << "Right click" << std::endl;
    }
    if (left_click) {
        std::cout << "Left click" << std::endl;
    }
}

void PlayerMovement(Player& player, int& inventorySlot) {
    Vector3 playerDirection = {0, 0, 0};
    Vector3 RotationDir = {0, 0, 0};

    PlayerInput(playerDirection, true, inventorySlot, RotationDir);

    if (RotationDir.x != 0 || RotationDir.y != 0) {
        player.Rotation.y += RotationDir.y * mouseSensitivity;
        player.Rotation.x += RotationDir.x * mouseSensitivity;
        player.Rotation.x = SDL_clamp(player.Rotation.x, -90.0f, 90.0f);
        if (player.Rotation.y > 360.0f) player.Rotation.y -= 360.0f;
        if (player.Rotation.y < 0.0f) player.Rotation.y += 360.0f;
    }

    // Handle movement
    if (playerDirection.x != 0 || playerDirection.y != 0 || playerDirection.z != 0) {
        // Move in Y first (e.g. jumping or flying)
        // std::cout << "Player Direction: " << playerDirection.x << ", " << playerDirection.y << ",
        // " << playerDirection.z << std::endl;
        player.Position.y += playerDirection.y;
        playerDirection.y = 0;

        playerDirection = playerDirection.Normalized();
        // Rotate the horizontal movement by player rotation (Y-axis)

        Vector3 rotatedDirection = player.Rotation.Forward() * playerDirection.z +
                                   player.Rotation.Right() * playerDirection.x;
        // Apply movement
        player.Position.x += rotatedDirection.x * playerSpeed;
        player.Position.z += rotatedDirection.z * playerSpeed;
    }
}
void GameLoop(GameClient& game) {
    game.add_player({
        {100, 66, 100},
        {0.0f, 0.0f, 0.0f},
        {255, 0, 0},
    });
    auto p = game.get_players();

    // Set up seed and color based on mode
    game.set_seed();
    game.set_color();
    SetSeed(game.get_seed());

    /*
        int width = 600;
        int height = 400;

        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;

        bool fullScreen = false;
        Vector3 playerDirection = {0, 0};
        */

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
        RendererObject.MainRenderLoop(p);
    }

    std::cout << "Exiting game..." << std::endl;
}
}  // namespace BitMiner
