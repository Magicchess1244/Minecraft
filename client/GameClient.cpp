#include "GameClient.hpp"

#include <cstring>
#include <iostream>

constexpr float mouseSensitivity = 0.1f;
constexpr float playerSpeed = 1.0f;

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 8000

GameClient::GameClient() {
    makeClient(DEFAULT_IP, DEFAULT_PORT);
    this->GameLoop();
}

GameClient::~GameClient() {
    std::cout << "Closing connection..." << std::endl;
    disconnect();
}

bool GameClient::makeClient(const std::string& server_ip, int port) {
    try {
#ifdef _WIN32
        if (initSockets() != 0) {
            std::cerr << "Failed to initialize sockets: " << GET_LAST_ERROR << "\n";
            return false;
        }
#endif

        server_socket = std::make_unique<Socket>(AF_INET, SOCK_STREAM, 0);

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);

        if (inet_pton(AF_INET, server_ip.c_str(), &serverAddr.sin_addr) <= 0) {
            std::cerr << "Invalid server IP address: " << server_ip << "\n";
            server_socket.reset();
            return false;
        }

        if (::connect(server_socket->get(), reinterpret_cast<sockaddr*>(&serverAddr),
                      sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Failed to connect to server " << server_ip << ":" << port
                      << " - Error: " << GET_LAST_ERROR << "\n";
            server_socket.reset();
            return false;
        }

        std::cout << "Connected to the server.\n";
        return true;

    } catch (const std::exception& ex) {
        std::cerr << "Connection error: " << ex.what() << "\n";
        server_socket.reset();
        return false;
    }
}

void GameClient::disconnect() {
    if (server_socket) {
        server_socket.reset();
    }
#ifdef _WIN32
    cleanupSockets();
#endif
}

bool GameClient::isConnected() const { return server_socket != nullptr; }

bool GameClient::sendCommand(const std::string& command) {
    if (!server_socket) {
        std::cerr << "Not connected to server\n";
        return false;
    }

    int bytes_sent = ::send(server_socket->get(), command.c_str(), command.length(), 0);
    if (bytes_sent <= 0) {
        std::cerr << "Failed to send command: " << command << "\n";
        return false;
    }

    return true;
}

std::string GameClient::receiveResponse() {
    if (!server_socket) {
        std::cerr << "Not connected to server\n";
        return "";
    }

    char buffer[256] = {};
    int bytes_received = ::recv(server_socket->get(), buffer, sizeof(buffer) - 1, 0);

    if (bytes_received <= 0) {
        std::cerr << "Failed to receive response or connection closed\n";
        return "";
    }

    buffer[bytes_received] = '\0';
    return std::string(buffer);
}

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
    if (!sendCommand("seed")) {
        std::cerr << "Error requesting seed from the server." << std::endl;
        return;
    }

    std::string response = receiveResponse();
    if (response.empty()) {
        std::cerr << "Error receiving seed from the server." << std::endl;
        return;
    }

    try {
        unsigned int server_seed = static_cast<unsigned int>(std::stoul(response));
        std::cout << "buf: " << response << std::endl;
        std::cout << "New client seed: " << server_seed << std::endl;
        srand(server_seed);
        seed = server_seed;
    } catch (const std::exception& ex) {
        std::cerr << "Failed to parse seed response: " << response << std::endl;
    }
}

void GameClient::set_color() {
    if (!sendCommand("getColor")) {
        std::cerr << "Error requesting color from the server." << std::endl;
        return;
    }

    std::string response = receiveResponse();
    if (response.empty()) {
        std::cerr << "Error receiving color from the server." << std::endl;
        return;
    }

    std::cout << "received" << std::endl;
    std::cout << "color" << std::endl;

    try {
        std::vector<std::string> color_parts = split(response, ",");
        if (color_parts.size() != 3) {
            std::cerr << "Invalid color format received" << std::endl;
            return;
        }

        unsigned int server_color[3];
        for (int i = 0; i < 3; i++) {
            server_color[i] = static_cast<unsigned int>(std::stoul(color_parts[i]));
        }

        std::cout << "New client color: " << server_color[0] << "," << server_color[1] << ","
                  << server_color[2] << std::endl;

        std::lock_guard<std::mutex> lock(players_mutex);
        std::cout << "players online: " << players.size() << std::endl;
        if (!players.empty()) {
            players[0].color = Color{static_cast<unsigned int>(server_color[0]),
                                     static_cast<unsigned int>(server_color[1]),
                                     static_cast<unsigned int>(server_color[2])};
        }
    } catch (const std::exception& ex) {
        std::cerr << "Failed to parse color response: " << response << std::endl;
    }
}

void GameClient::add_player(const Player& player) {
    std::lock_guard<std::mutex> lock(players_mutex);
    if (players.size() < MAX_PLAYERS) {
        players.push_back(player);
        player_count++;
    }
    std::cout << "players size: " << players.size() << std::endl;
}

const std::vector<Player> GameClient::get_players() const {
    std::lock_guard<std::mutex> lock(players_mutex);
    return players;
}

Color GameClient::get_color() const {
    std::lock_guard<std::mutex> lock(players_mutex);
    if (!players.empty()) {
        return players[0].color;
    }
    return {0, 0, 0};
}

void GameClient::GameLoop() {
    // Add initial player
    this->add_player({
        {100, 66, 100},
        {0.0f, 0.0f, 0.0f},
        {255, 0, 0},
    });

    this->set_seed();
    this->set_color();

    if (this->players.empty()) {
        std::cerr << "No players available!" << std::endl;
        return;
    }

    std::vector<Slot> inventory;
    for (int i = 0; i < 8; i++) {
        inventory.push_back({0, 0});
    }

    inventory[0] = {60, 1};
    inventory[1] = {5, 5};

    // Initialize renderer
    try {
        int inventorySlot = 0;
        Renderer rendererObject;

        std::cout << "Starting game loop..." << std::endl;

        while (this->GetRunning()) {
            // Get current players (thread-safe)
            auto current_players = this->get_players();
            if (!current_players.empty()) {
                BitMiner::PlayerMovement(current_players[0], inventorySlot);

                // FIXME: s'han de sync millor
                std::lock_guard<std::mutex> lock(this->players_mutex);
                if (!this->players.empty()) {
                    this->players[0] = current_players[0];
                }
            }

            rendererObject.mainRenderLoop(inventory, inventorySlot, current_players);
        }
    } catch (const std::exception& ex) {
        std::cerr << "Renderer error: " << ex.what() << std::endl;
    }
}

// BitMiner namespace functions
namespace BitMiner {

int FindSlot(const std::vector<Slot>& Inventory, short Type) {
    for (size_t i = 0; i < Inventory.size(); ++i) {
        const auto& slot = Inventory[i];
        if ((slot.Type == Type || slot.Type == 0) && slot.Amount < 64) {
            std::cout << "Found slot " << i << std::endl;
            return static_cast<int>(i);
        }
    }
    std::cout << "Inventory full, cannot add item of type: " << Type << std::endl;
    return -1;
}

void PlayerInput(Vector3& PlayerDirection, bool OnGround, int& InventorySlots, Vector3& PlayerRot) {
    const bool* KeyboardState = SDL_GetKeyboardState(NULL);
    const bool move_forward = (KeyboardState[SDL_SCANCODE_W] || KeyboardState[SDL_SCANCODE_UP]);
    const bool move_backward = (KeyboardState[SDL_SCANCODE_S] || KeyboardState[SDL_SCANCODE_DOWN]);
    const bool move_up = KeyboardState[SDL_SCANCODE_SPACE];
    const bool move_down = KeyboardState[SDL_SCANCODE_LSHIFT];
    const bool move_left = KeyboardState[SDL_SCANCODE_A] || KeyboardState[SDL_SCANCODE_LEFT];
    const bool move_right = KeyboardState[SDL_SCANCODE_D] || KeyboardState[SDL_SCANCODE_RIGHT];

    // FIXME: no s'està fent servir
    (void)OnGround;

    PlayerDirection.x = 0;
    PlayerDirection.y = 0;
    PlayerDirection.z = 0;

    // Get mouse movement
    float mouseX, mouseY;
    SDL_GetRelativeMouseState(&mouseX, &mouseY);

    // Apply mouse movement to rotation
    PlayerRot.y = -static_cast<float>(mouseX);  // Yaw (left/right)
    PlayerRot.x = -static_cast<float>(mouseY);  // Pitch (up/down)
    PlayerRot.z = 0;                            // Roll (not used for FPS camera)

    if (move_left || move_right) {
        PlayerDirection.x = move_left ? -1.0f : 1.0f;
    }
    if (move_down || move_up) {
        PlayerDirection.y = move_down ? -1.0f : 1.0f;
    }
    if (move_backward || move_forward) {
        PlayerDirection.z = move_backward ? -1.0f : 1.0f;
    }

    for (int i = 0; i < 8; ++i) {
        if (KeyboardState[SDL_SCANCODE_1 + i]) {
            InventorySlots = i;
            break;
        }
    }
}

void PlayerMovement(Player& player, int& inventorySlot) {
    Vector3 playerDirection = {0, 0, 0};
    Vector3 rotationDir = {0, 0, 0};

    PlayerInput(playerDirection, true, inventorySlot, rotationDir);

    if (rotationDir.x != 0 || rotationDir.y != 0) {
        player.Rotation.y += rotationDir.y * mouseSensitivity;
        player.Rotation.x += rotationDir.x * mouseSensitivity;

        // Clamp pitch to prevent over-rotation
        if (player.Rotation.x > 89.0f) player.Rotation.x = 89.0f;
        if (player.Rotation.x < -89.0f) player.Rotation.x = -89.0f;

        // Wrap yaw around 360 degrees
        if (player.Rotation.y > 360.0f) player.Rotation.y -= 360.0f;
        if (player.Rotation.y < 0.0f) player.Rotation.y += 360.0f;
    }

    if (playerDirection.x != 0 || playerDirection.y != 0 || playerDirection.z != 0) {
        player.Position.y += playerDirection.y;
        playerDirection.y = 0;

        float length =
            sqrt(playerDirection.x * playerDirection.x + playerDirection.z * playerDirection.z);
        if (length > 0) {
            playerDirection.x /= length;
            playerDirection.z /= length;
        }

        float radiansY = player.Rotation.y * (M_PI / 180.0f);
        float cosY = cosf(radiansY);
        float sinY = sinf(radiansY);

        Vector3 rotatedDirection = {playerDirection.x * cosY - playerDirection.z * sinY, 0,
                                    playerDirection.x * sinY + playerDirection.z * cosY};

        player.Position.x += rotatedDirection.x * playerSpeed;
        player.Position.z += rotatedDirection.z * playerSpeed;
    }
}

}  // namespace BitMiner
