#include "Renderer.hpp"

#include "../common/BlockTypes.hpp"
#include "GameClient.hpp"

const float FOV = 90.0f * (PI / 180.0f);
const float Znear = 0.1f;
constexpr float Zfar = 1000.0f;
constexpr int RenderDistance = 1;
const Vector3 Verts[6][4] = {{// Front (-Z)
                             {-0.5, -0.5, -0.5},
                             {0.5, -0.5, -0.5},
                             {-0.5, 0.5, -0.5},
                             {0.5, 0.5, -0.5}},
                            {// Back (+Z)
                             {0.5, -0.5, 0.5},
                             {-0.5, -0.5, 0.5},
                             {0.5, 0.5, 0.5},
                             {-0.5, 0.5, 0.5}},
                            {// Right (+X)
                             {0.5, -0.5, -0.5},
                             {0.5, -0.5, 0.5},
                             {0.5, 0.5, -0.5},
                             {0.5, 0.5, 0.5}},
                            {// Left (-X)
                             {-0.5, -0.5, 0.5},
                             {-0.5, -0.5, -0.5},
                             {-0.5, 0.5, 0.5},
                             {-0.5, 0.5, -0.5}},
                            {// Top (+Y)
                             {-0.5, 0.5, -0.5},
                             {0.5, 0.5, -0.5},
                             {-0.5, 0.5, 0.5},
                             {0.5, 0.5, 0.5}},
                            {// Bottom (-Y)
                             {-0.5, -0.5, -0.5},
                             {0.5, -0.5, -0.5},
                             {-0.5, -0.5, 0.5},
                             {0.5, -0.5, 0.5}}};
const Vector3 Direction[6] = {
    {0, 0, 1},   // Front
    {0, 0, -1},  // Back
    {1, 0, 0},   // Right
    {-1, 0, 0},  // Left
    {0, -1, 0},  // Top
    {0, 1, 0}    // Bottom
};
const Color Colors[3] = {
    {0, 0, 0},     // Front / Back
    {5, 5, 5},     // Right / Left
    {10, 10, 10},  // Top / Bottom
};

constexpr Uint32 MAX_FACES_PER_CHUNK = 30000;
constexpr Uint32 VERTICES_PER_FACE = 4;
constexpr Uint32 INDICES_PER_FACE = 6;

// Validate these are reasonable
static_assert(sizeof(Vertex) > 0, "Vertex size must be positive");
static_assert(MAX_FACES_PER_CHUNK <= 30000, "Too many faces per chunk");

constexpr Uint32 vertexSize = sizeof(Vertex) * VERTICES_PER_FACE * MAX_FACES_PER_CHUNK;
constexpr Uint32 indexSize = sizeof(Uint32) * INDICES_PER_FACE * MAX_FACES_PER_CHUNK;

// Add debug output for these sizes
void PrintBufferSizes() {
    std::cout << "Vertex struct size: " << sizeof(Vertex) << " bytes" << std::endl;
    std::cout << "Max faces per chunk: " << MAX_FACES_PER_CHUNK << std::endl;
    std::cout << "Vertex buffer size: " << vertexSize << " bytes (" << vertexSize / 1024 << " KB)"
              << std::endl;
    std::cout << "Index buffer size: " << indexSize << " bytes (" << indexSize / 1024 << " KB)"
              << std::endl;
}

Matrix Perspective(float fovRadians, float aspect, float Near, float Far) {
    const float f = 1.0f / std::tan(fovRadians * 0.5f);
    Matrix m(4, 4, 0.0f);
    m(0, 0) = f / aspect;
    m(1, 1) = f;
    m(2, 2) = (Far + Near) / (Near - Far);
    m(2, 3) = (2.0f * Far * Near) / (Near - Far);
    m(3, 2) = 1.0f;
    return m;
}
Matrix RotationX(float angleRad) {
    Matrix m = Matrix::Identity(4);
    float c = std::cos(angleRad);
    float s = std::sin(angleRad);

    // Rotation around X-axis
    m(1, 1) = c;   // cos
    m(1, 2) = -s;  // -sin
    m(2, 1) = s;   // sin
    m(2, 2) = c;   // cos

    return m;
}
Matrix RotationY(float angleRad) {
    Matrix m = Matrix::Identity(4);
    float c = std::cos(angleRad);
    float s = std::sin(angleRad);

    // Rotation around Y-axis
    m(0, 0) = c;   // cos
    m(0, 2) = s;   // sin
    m(2, 0) = -s;  // -sin
    m(2, 2) = c;   // cos

    return m;
}
Matrix RotationZ(float angleRad) {
    Matrix m = Matrix::Identity(4);
    float c = std::cos(angleRad);
    float s = std::sin(angleRad);

    // Rotation around Z-axis
    m(0, 0) = c;   // cos
    m(0, 1) = -s;  // -sin
    m(1, 0) = s;   // sin
    m(1, 1) = c;   // cos

    return m;
}
// Combined rotation functions
Matrix Rotation(float x, float y, float z) {
    // Apply rotations in order: Z * Y * X (standard Euler angle order)
    return RotationZ(z) * RotationY(y) * RotationX(x);
}
Matrix Translation(float x, float y, float z) {
    Matrix m = Matrix::Identity(4);
    m(0, 3) = x;  // X translation
    m(1, 3) = y;  // Y translation
    m(2, 3) = z;  // Z translation
    return m;
}
Matrix Scale(float x, float y, float z) {
    Matrix m = Matrix::Identity(4);
    m(0, 0) = x;  // X scale
    m(1, 1) = y;  // Y scale
    m(2, 2) = z;  // Z scale
    return m;
}
// LookAt View matrix (like glm::lookAt)
Matrix LookAt(const std::vector<float>& eye, const std::vector<float>& target,
              const std::vector<float>& up) {
    auto normalize = [](std::vector<float> v) {
        float len = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        for (auto& x : v) x /= len;
        return v;
    };
    auto cross = [](const std::vector<float>& a, const std::vector<float>& b) {
        return std::vector<float>{a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
                                  a[0] * b[1] - a[1] * b[0]};
    };
    auto dot = [](const std::vector<float>& a, const std::vector<float>& b) {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    };

    std::vector<float> f = normalize({target[0] - eye[0], target[1] - eye[1], target[2] - eye[2]});
    std::vector<float> s = normalize(cross(f, up));
    std::vector<float> u = cross(s, f);

    Matrix m = Matrix::Identity(4);
    m(0, 0) = s[0];
    m(0, 1) = s[1];
    m(0, 2) = s[2];
    m(0, 3) = -dot(s, eye);
    m(1, 0) = u[0];
    m(1, 1) = u[1];
    m(1, 2) = u[2];
    m(1, 3) = -dot(u, eye);
    m(2, 0) = -f[0];
    m(2, 1) = -f[1];
    m(2, 2) = -f[2];
    m(2, 3) = -dot(f, eye);
    return m;
}
bool isFaceInFrustum(const Frustum& frustum, const Vector3 faceVerts[4]) {
    const Plane planes[6] = {frustum.nearFace, frustum.farFace, frustum.rightFace,
                             frustum.leftFace, frustum.topFace, frustum.bottomFace};

    // For each plane, check if all 4 vertices are outside
    for (const auto& plane : planes) {
        int outsideCount = 0;
        for (int i = 0; i < 4; ++i) {
            if (plane.getSignedDistanceToPlane(faceVerts[i]) < 0.0f) {
                ++outsideCount;
            }
        }
        if (outsideCount == 4) {
            return false;  // entire face is outside this plane
        }
    }

    return true;  // at least partially inside all planes
}
SDL_FPoint Renderer::getUV(int tileIndex, int cornerX, int cornerY) {
    const int tileSize = 16;
    const int atlasSize = 64;
    const float pixelNudge = 0.25f;  // or 0.25f if needed

    int tilesPerRow = atlasSize / tileSize;
    int tileX = tileIndex % tilesPerRow;
    int tileY = tileIndex / tilesPerRow;

    float u =
        (tileX * tileSize + cornerX * (tileSize - pixelNudge * 2) + pixelNudge) / (float)atlasSize;
    float v =
        (tileY * tileSize + cornerY * (tileSize - pixelNudge * 2) + pixelNudge) / (float)atlasSize;

    SDL_FPoint point = {(float)u, (float)v};
    return point;
}
Vector3 RotateVector(const Vector3& v, const Vector3& v1) {
    float xRad = v1.x;
    float yRad = v1.y;
    float zRad = v1.z;
    float cx = std::cos(xRad), sx = std::sin(xRad);
    float cy = std::cos(yRad), sy = std::sin(yRad);
    float cz = std::cos(zRad), sz = std::sin(zRad);

    // Rotation order: Z * Y * X
    float m00 = cy * cz;
    float m01 = -cy * sz;
    float m02 = sy;

    float m10 = sx * sy * cz + cx * sz;
    float m11 = -sx * sy * sz + cx * cz;
    float m12 = -sx * cy;

    float m20 = -cx * sy * cz + sx * sz;
    float m21 = cx * sy * sz + sx * cz;
    float m22 = cx * cy;

    return {m00 * v.x + m01 * v.y + m02 * v.z, m10 * v.x + m11 * v.y + m12 * v.z,
            m20 * v.x + m21 * v.y + m22 * v.z};
}
void Renderer::DrawFace(Player& player, Vector3 blocks, int blockID, int Side, Mesh* mesh,
                        Vertex* Vertexdata, Uint32* Indexdata) {
    const Vector3* verts = Verts[Side];
    const int baseVertex = mesh->faces * 4;
    const int baseIndex = mesh->faces * 6;

    (void)player;

    // Get block definition
    const BlockDefinition& blockDef = g_BlockRegistry.getBlock(blockID);
    Color baseColor = blockDef.color;

    // Calculate lighting based on face direction
    float lightLevel = 1.0f;
    switch (Side) {
        case 0:                 // Front face
        case 1:                 // Back face
        case 2:                 // Right face
        case 3:                 // Left face
            lightLevel = 0.8f;  // Side faces are darker
            break;
        case 4:                 // Top face
            lightLevel = 1.0f;  // Top faces are brightest
            break;
        case 5:                 // Bottom face
            lightLevel = 0.6f;  // Bottom faces are darkest
            break;
    }

    // Apply ambient lighting
    lightLevel = std::max(0.1f, lightLevel);

    // Calculate final color with lighting
    Color faceColor = {static_cast<unsigned int>(baseColor.r * lightLevel),
                       static_cast<unsigned int>(baseColor.g * lightLevel),
                       static_cast<unsigned int>(baseColor.b * lightLevel)};

    for (int i = 0; i < 4; i++) {
        Vector3 worldPos = verts[i] + blocks;
        Vertex vertex = {worldPos, faceColor.ToFloat()};
        Vertexdata[baseVertex + i] = vertex;
    }

    Indexdata[baseIndex] = baseVertex + 0;
    Indexdata[baseIndex + 1] = baseVertex + 1;
    Indexdata[baseIndex + 2] = baseVertex + 2;

    Indexdata[baseIndex + 3] = baseVertex + 2;
    Indexdata[baseIndex + 4] = baseVertex + 1;
    Indexdata[baseIndex + 5] = baseVertex + 3;

    mesh->faces++;
}
void Renderer::RenderChunk(const ChunkPrefab& chunk, Player& player, int NumChunk) {
    // Vector3 Radiants = player.Rotation.AngleToRadians(); // TODO: Use for rotation calculations
    auto* mesh = &this->Terrain[NumChunk];
    mesh->faces = 0;

    std::vector<DrawnFace> Faces;

    for (const auto& face : chunk.allFaces) {
        // std::cout << face << std::endl;
        // Only backface-cull slightly; keep conservative to ensure something draws
        // Face normal points out of the block; if dot with view dir > 0, it's facing away
        if (player.Rotation.Forward().Dot(Direction[face.side]) > 0.0f) continue;
        Vector3 local[4];
        for (int j = 0; j < 4; j++) {
            Vector3 worldFacePos = face.blockPos + Verts[face.side][j];
            local[j] = RotateVector((worldFacePos - player.Position),
                                    player.Rotation.AngleToRadians() * -1);
        }
        // if (Max.z < Znear) continue;
        if (!isFaceInFrustum(this->frustum, local)) continue;
        //  std::cout << Max.x << std::endl;
        Faces.push_back({face.blockPos, face.side, face.blockID, face.blockID == 5});
    }

    Vertex* Vertexdata =
        (Vertex*)SDL_MapGPUTransferBuffer(this->GPU, mesh->VertextransferBuffer, true);
    Uint32* Indexdata =
        (Uint32*)SDL_MapGPUTransferBuffer(this->GPU, mesh->IndextransferBuffer, true);

    for (auto& Face : Faces) {
        DrawFace(player, Face.blockPos, Face.blockID, Face.side, mesh, Vertexdata, Indexdata);
    }

    //---------------Vertex-----------------

    SDL_UnmapGPUTransferBuffer(this->GPU, mesh->VertextransferBuffer);

    // where is the data
    SDL_GPUTransferBufferLocation Vertexlocation{};
    Vertexlocation.transfer_buffer = mesh->VertextransferBuffer;
    Vertexlocation.offset = 0;  // start from the beginning

    // where to upload the data
    SDL_GPUBufferRegion Vertexregion{};
    Vertexregion.buffer = mesh->VertexBuffer.buffer;
    Vertexregion.size = vertexSize;
    Vertexregion.offset = 0;

    // upload the data
    SDL_UploadToGPUBuffer(this->copyPass, &Vertexlocation, &Vertexregion, true);

    //---------------Index-----------------

    SDL_UnmapGPUTransferBuffer(this->GPU, mesh->IndextransferBuffer);

    // where is the data
    SDL_GPUTransferBufferLocation Indexlocation{};
    Indexlocation.transfer_buffer = mesh->IndextransferBuffer;
    Indexlocation.offset = 0;  // start from the beginning

    // where to upload the data
    SDL_GPUBufferRegion Indexregion{};
    Indexregion.buffer = mesh->IndexBuffer.buffer;
    Indexregion.size = indexSize;
    Indexregion.offset = 0;

    // upload the data
    SDL_UploadToGPUBuffer(this->copyPass, &Indexlocation, &Indexregion, true);
}
void Renderer::DrawTerrain(Player& player) {
    Vector3 PlayerChunk = (player.Position / 32).Truncate();
    const int side = (RenderDistance * 2) + 1;
    for (int i = -RenderDistance; i <= RenderDistance; i++) {
        for (int j = -RenderDistance; j <= RenderDistance; j++) {
            Vector3 Chunk = {(float)i, 0, (float)j};
            Chunk += PlayerChunk;

            // Map (i,j) to mesh index in Terrain vector
            const int meshIndex = (i + RenderDistance) * side + (j + RenderDistance);
            if (meshIndex >= 0 && meshIndex < (int)this->Terrain.size()) {
                RenderChunk(std::ref(chunkManager.get_chunk(Chunk)), std::ref(player), meshIndex);
            }
        }
    }
}
void Renderer::DrawPlayer(SDL_Renderer* Renderer, Vector3 Range,
                          const std::vector<Player>& PlayerPos) {
    // FIXME: unused function & params
    (void)(void)Renderer;
    (void)(void)Range;
    (void)(void)PlayerPos;

    /* 2D
    //Other player
    for (const Player& Position : PlayerPos)
    {
            int dx = (int)(Position.Position.x - PlayerPos[0].Position.x);
            int dy = (int)(Position.Position.y - PlayerPos[0].Position.y);

            bool InsideX = std::abs(dx) <= (Range.x / 2);
            if (InsideX) {
                    int RelativeX = (int)((Range.x / 2 - 1 + dx) * BlockPixelSize);
                    int RelativeY = (int)((Range.y / 2 - 2 - dy) * BlockPixelSize);

                    SDL_FRect OtherPlayerRect = {
                            (float)RelativeX,
                            (float)RelativeY,
                            (float)BlockPixelSize,
                            (float)BlockPixelSize * 2
                    };
                    SDL_SetRenderDrawColor(Renderer, (Uint8)SDL_clamp(PlayerPos[0].color.r, 0, 255),
    (Uint8)SDL_clamp(PlayerPos[0].color.g, 0, 255), (Uint8)SDL_clamp(PlayerPos[0].color.b, 0, 255),
    255); SDL_RenderFillRect(Renderer, &OtherPlayerRect);
            }
    }

    // Your player
    SDL_SetRenderDrawColor(Renderer, SDL_clamp(PlayerPos[0].color.r, 0, 255),
    SDL_clamp(PlayerPos[0].color.g, 0, 255), SDL_clamp(PlayerPos[0].color.b, 0, 255), 255);
    SDL_FRect PlayerRect = {
            (float)(Range.x / 2 - 1) *  BlockPixelSize,
            (float)(Range.y / 2 - 2) * this->BlockPixelSize,
            (float)this->BlockPixelSize,
            (float)this->BlockPixelSize * 2
    };
    SDL_RenderFillRect(Renderer, &PlayerRect);

    SDL_SetRenderDrawColor(Renderer, SDL_clamp(PlayerPos[0].color.r + 90, 0, 255),
    SDL_clamp(PlayerPos[0].color.g + 90, 0, 255), SDL_clamp(PlayerPos[0].color.b + 90, 0, 255),
    255); SDL_FRect InsidePlayerRect = { (float)(Range.x / 2 - 1) * this->BlockPixelSize +
    (this->BlockPixelSize * 0.1f), (float)(Range.y / 2 - 2) * this->BlockPixelSize +
    (this->BlockPixelSize * 0.1f), (float)(this->BlockPixelSize * 0.8f),
            (float)(this->BlockPixelSize * 0.9f) * 2
    };
    SDL_RenderFillRect(Renderer, &InsidePlayerRect);
    */
}

void Renderer::Stats(const Player& player) {
    std::string text = std::to_string(player.Position.x) + ", " +
                       std::to_string(player.Position.y) + ", " + std::to_string(player.Position.z);
    SDL_Color White = {200, 200, 200, 255};

    if (!this->font) {
        std::cerr << "Font not loaded!\n";
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(this->font, text.c_str(), text.length(), White);
    if (!surface) {
        return;
    }

    // Create GPU texture
    SDL_GPUTextureCreateInfo textureCreateInfo = {};
    textureCreateInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureCreateInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    textureCreateInfo.width = surface->w;
    textureCreateInfo.height = surface->h;
    textureCreateInfo.layer_count_or_depth = 1;
    textureCreateInfo.num_levels = 1;
    textureCreateInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    SDL_GPUTexture* texture = SDL_CreateGPUTexture(this->GPU, &textureCreateInfo);
    if (!texture) {
        std::cerr << "SDL_CreateGPUTexture failed: " << SDL_GetError() << "\n";
        SDL_DestroySurface(surface);
        return;
    }

    // Create transfer buffer for uploading texture data
    SDL_GPUTransferBufferCreateInfo VertextransferInfo{};
    VertextransferInfo.size = 16;
    VertextransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    VertextransferInfo.props = 0;
    SDL_GPUTransferBuffer* transferBuffer =
        SDL_CreateGPUTransferBuffer(this->GPU, &VertextransferInfo);

    if (!transferBuffer) {
        std::cerr << "SDL_CreateGPUTransferBuffer failed: " << SDL_GetError() << "\n";
        SDL_ReleaseGPUTexture(this->GPU, texture);
        SDL_DestroySurface(surface);
        return;
    }

    // Map transfer buffer and copy surface data
    void* mapped = SDL_MapGPUTransferBuffer(this->GPU, transferBuffer, false);
    if (mapped) {
        memcpy(mapped, surface->pixels, surface->w * surface->h * 4);
        SDL_UnmapGPUTransferBuffer(this->GPU, transferBuffer);

        SDL_GPUTextureTransferInfo transferInfo = {};
        transferInfo.transfer_buffer = transferBuffer;
        transferInfo.offset = 0;

        SDL_GPUTextureRegion textureRegion = {};
        textureRegion.texture = texture;
        textureRegion.mip_level = 0;
        textureRegion.layer = 0;
        textureRegion.x = 0;
        textureRegion.y = 0;
        textureRegion.z = 0;
        textureRegion.w = surface->w;
        textureRegion.h = surface->h;
        textureRegion.d = 1;

        SDL_UploadToGPUTexture(copyPass, &transferInfo, &textureRegion, false);
        SDL_EndGPUCopyPass(copyPass);

        // Now you would need to render this texture using a render pass
        // This requires setting up graphics pipelines, vertex buffers, etc.
        // which is much more complex than the simple blit operation you had before

        // TODO: Add your rendering logic here using SDL_BeginGPURenderPass()
        // and appropriate graphics pipeline setup
    }

    SDL_ReleaseGPUTransferBuffer(this->GPU, transferBuffer);
    SDL_ReleaseGPUTexture(this->GPU, texture);
    SDL_DestroySurface(surface);
}

void Renderer::MainRenderLoop(std::vector<Slot>& inventory, int inventorySlot,
                              std::vector<Player>& players) {
    while (SDL_PollEvent(&this->event)) {
        switch (this->event.type) {
            case SDL_EVENT_QUIT:
                this->gameClient.Quit();
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                this->Width = (int)this->event.window.data1;
                this->Height = (int)this->event.window.data2;
                this->depthTexture = CreateDepthTexture(this->Width, this->Height);
                break;

            case SDL_EVENT_KEY_DOWN: {
                SDL_Keycode key = this->event.key.scancode;

                if (key == SDL_SCANCODE_ESCAPE) {
                    this->gameClient.Quit();
                    break;
                } else if (key == SDL_SCANCODE_F11) {
                    this->fullScreen = !this->fullScreen;
                    SDL_SetWindowFullscreen(this->window, this->fullScreen);
                }
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                if (this->event.button.button == SDL_BUTTON_LEFT) {
                    // Break block: raycast to hit and set to AIR
                    Vector3 hit, place;
                    if (RaycastBlock(players[0], 6.0f, hit, place)) {
                        Vector3 chunkKey = {(float)floorf(hit.x / 32.0f), 0, (float)floorf(hit.z / 32.0f)};
                        ChunkPrefab& chunk = chunkManager.get_chunk(chunkKey);
                        Vector3 local = {hit.x - chunk.xPos, hit.y, hit.z - chunk.zPos};
                        auto it = chunk.Blocks.find(local);
                        if (it != chunk.Blocks.end()) {
                            it->second = (int)BlockType::AIR;
                            chunk.VisableFaces();
                        }
                    }
                } else if (this->event.button.button == SDL_BUTTON_RIGHT &&
                           inventory[inventorySlot].Amount > 0) {
                    // Place block at placePos using current slot Type
                    Vector3 hit, place;
                    if (RaycastBlock(players[0], 6.0f, hit, place)) {
                        short type = inventory[inventorySlot].Type;
                        if (type != 0) {
                            Vector3 chunkKey = {(float)floorf(place.x / 32.0f), 0, (float)floorf(place.z / 32.0f)};
                            ChunkPrefab& chunk = chunkManager.get_chunk(chunkKey);
                            Vector3 local = {place.x - chunk.xPos, place.y, place.z - chunk.zPos};
                            chunk.Blocks[local] = (int)type;
                            chunk.VisableFaces();
                            inventory[inventorySlot].Amount--;
                            if (inventory[inventorySlot].Amount == 0) inventory[inventorySlot].Type = 0;
                        }
                    }
                }
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL: {
                // Scroll inventory slot 0..7
                int delta = (int)this->event.wheel.y;
                inventorySlot = (inventorySlot - delta) & 7;
                break;
            }
        }
    }

    this->cmdRender = SDL_AcquireGPUCommandBuffer(this->GPU);
    this->cmdCopy = SDL_AcquireGPUCommandBuffer(this->GPU);

    SDL_GPUTexture* swap_texture;
    SDL_WaitAndAcquireGPUSwapchainTexture(this->cmdRender, this->window, &swap_texture,
                                          &this->Width, &this->Height);

    if (swap_texture == NULL) return;

    SDL_GPUColorTargetInfo colorInfo = {};
    colorInfo.clear_color = SDL_FColor{0.0f, 0.69f, 1.0f, 1.0f};
    colorInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorInfo.store_op = SDL_GPU_STOREOP_STORE;
    colorInfo.texture = swap_texture;

    SDL_GPUDepthStencilTargetInfo depth_target_info;
    SDL_zero(depth_target_info);
    depth_target_info.clear_depth = 1.0f;
    depth_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_target_info.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depth_target_info.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depth_target_info.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    depth_target_info.texture = depthTexture;
    depth_target_info.cycle = true;

    this->copyPass = SDL_BeginGPUCopyPass(this->cmdCopy);

    DrawTerrain(players[0]);
    /*
    Vertex* Vertexdata = (Vertex*)SDL_MapGPUTransferBuffer(this->GPU,
    this->Terrain[0].VertextransferBuffer, true); Uint32* Indexdata =
    (Uint32*)SDL_MapGPUTransferBuffer(this->GPU, this->Terrain[0].IndextransferBuffer, true);

    std::memcpy(Vertexdata, DefaultVertex, sizeof(DefaultVertex));
    std::memcpy(Indexdata, DefaultIndex, sizeof(DefaultIndex));
    */

    // Stats(player);
    // DrawBG(renderer, players[0],{ (float)width, (float)height, 0}, texture);
    // ChunckManager::ShowInventor(renderer, width, height, std::ref(inventory), inventorySlot,
    // font); DrawPlayer(renderer, Range, std::ref(players)); SDL_Delay(1000 / 10);
    SDL_EndGPUCopyPass(this->copyPass);

    if (!SDL_SubmitGPUCommandBuffer(this->cmdCopy)) {
        std::cout << "Heil la memoria de Puigdemont\n";
    }
    this->pass = SDL_BeginGPURenderPass(this->cmdRender, &colorInfo, 1, NULL);
    SDL_BindGPUGraphicsPipeline(this->pass, this->graphicsPipeline);

    // int i = 0; // TODO: Use for iteration
    // Vector3 Rotationplayers = players[0].Rotation.AngleToRadians(); // TODO: Use for rotation
    // calculations Vector3 Positionplayers = players[0].Position; // TODO: Use for position
    // calculations
    Player player = players[0];
    Matrix model = Matrix::Identity(4);

    std::vector<float> eye = {player.Position.x, player.Position.y, player.Position.z};
    Vector3 forward = player.Rotation.Forward();  // You'll need to implement this
    std::vector<float> target = {player.Position.x + forward.x, player.Position.y + forward.y,
                                 player.Position.z + forward.z};
    std::vector<float> up = {0.0f, 1.0f, 0.0f};
    Matrix view = LookAt(eye, target, up);
    float aspect = (float)this->Width / (float)this->Height;
    Matrix proj = Perspective(FOV, aspect, Znear, Zfar);

    // Final MVP matrix
    Matrix mvp = proj * view * model;

    SDL_PushGPUVertexUniformData(this->cmdRender, 0, mvp.data.data(),
                                 sizeof(float) * mvp.data.size());
    // std::cout << "MVP matrix:\n";
    // mvp.print();

    for (auto& mesh : this->Terrain) {
        SDL_BindGPUVertexBuffers(this->pass, 0, &mesh.VertexBuffer, 1);
        SDL_BindGPUIndexBuffer(this->pass, &mesh.IndexBuffer, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_DrawGPUIndexedPrimitives(this->pass, mesh.faces * 6, 1, 0, 0, 0);
    }

    SDL_EndGPURenderPass(this->pass);

    if (!SDL_SubmitGPUCommandBuffer(this->cmdRender)) {
        std::cout << "Heil el render de Puigdemont\n";
    }

    // After 3D pass is submitted, start a UI pass: acquire new cmd buffer and draw bar with ortho
    this->cmdRender = SDL_AcquireGPUCommandBuffer(this->GPU);
    SDL_GPUColorTargetInfo uiColorInfo = {};
    uiColorInfo.clear_color = SDL_FColor{0,0,0,0};
    uiColorInfo.load_op = SDL_GPU_LOADOP_LOAD; // keep the 3D image
    uiColorInfo.store_op = SDL_GPU_STOREOP_STORE;
    uiColorInfo.texture = swap_texture;
    this->pass = SDL_BeginGPURenderPass(this->cmdRender, &uiColorInfo, 1, NULL);
    SDL_BindGPUGraphicsPipeline(this->pass, this->graphicsPipeline);
    DrawInventoryBar(inventory, inventorySlot);
    SDL_EndGPURenderPass(this->pass);
    SDL_SubmitGPUCommandBuffer(this->cmdRender);
}

bool Renderer::RaycastBlock(const Player& player, float maxDistance, Vector3& hitBlock, Vector3& placePos) {
    // Step along the view ray in small increments
    Vector3 dir = player.Rotation.Forward();
    const float step = 0.25f;
    Vector3 pos = player.Position;
    Vector3 lastEmpty = pos;

    for (float t = 0.0f; t <= maxDistance; t += step) {
        Vector3 p = pos + dir * t;
        Vector3 block = { floorf(p.x), floorf(p.y), floorf(p.z) };

        // Determine which chunk the block belongs to
        Vector3 chunkKey = { (float)floorf(block.x / 32.0f), 0, (float)floorf(block.z / 32.0f) };
        ChunkPrefab& chunk = chunkManager.get_chunk(chunkKey);
        Vector3 local = { block.x - chunk.xPos, block.y, block.z - chunk.zPos };

        auto it = chunk.Blocks.find(local);
        if (it != chunk.Blocks.end() && it->second != (int)BlockType::AIR) {
            hitBlock = { (float)block.x, (float)block.y, (float)block.z };
            placePos = { floorf(lastEmpty.x), floorf(lastEmpty.y), floorf(lastEmpty.z) };
            return true;
        }

        lastEmpty = block;
    }
    return false;
}

void Renderer::DrawInventoryBar(std::vector<Slot>& inventory, int inventorySlot) {
    // Build a simple screen-space quad strip for 8 slots (no textures, colored faces)
    // We'll draw in NDC by pre-transforming vertices with an orthographic mapping via MVP slot
    // Simpler: render as world-space quads anchored to camera with large negative z to avoid depth conflicts
    const float barWidth = 0.8f * (float)this->Width;
    const float barHeight = 50.0f;
    const float x0 = ((float)this->Width - barWidth) * 0.5f;
    const float y0 = (float)this->Height - barHeight - 10.0f;
    const float slotW = barWidth / 8.0f;
    const float pad = 6.0f;

    // Prepare UI mesh transfer buffers (reuse uiMesh)
    uiMesh.faces = 0;
    Vertex* vdata = (Vertex*)SDL_MapGPUTransferBuffer(this->GPU, uiMesh.VertextransferBuffer, true);
    Uint32* idata = (Uint32*)SDL_MapGPUTransferBuffer(this->GPU, uiMesh.IndextransferBuffer, true);
    if (!vdata || !idata) return;

    auto push_rect = [&](float x, float y, float w, float h, const Color& col) {
        int baseVertex = uiMesh.faces * 4;
        int baseIndex = uiMesh.faces * 6;
        Vector3 c = col.ToFloat();
        vdata[baseVertex + 0] = { {x,     y,     0}, c };
        vdata[baseVertex + 1] = { {x + w, y,     0}, c };
        vdata[baseVertex + 2] = { {x,     y + h, 0}, c };
        vdata[baseVertex + 3] = { {x + w, y + h, 0}, c };
        idata[baseIndex + 0] = baseVertex + 0;
        idata[baseIndex + 1] = baseVertex + 1;
        idata[baseIndex + 2] = baseVertex + 2;
        idata[baseIndex + 3] = baseVertex + 2;
        idata[baseIndex + 4] = baseVertex + 1;
        idata[baseIndex + 5] = baseVertex + 3;
        uiMesh.faces++;
    };

    // Bar background
    push_rect(x0, y0, barWidth, barHeight, {50, 25, 0});
    // Slots
    for (int i = 0; i < 8; ++i) {
        Color slotColor = (i == inventorySlot) ? Color{255, 180, 80} : Color{160, 80, 0};
        float sx = x0 + i * slotW + pad * 0.5f;
        float sy = y0 + pad * 0.5f;
        float sw = slotW - pad;
        float sh = barHeight - pad;
        push_rect(sx, sy, sw, sh, slotColor);

        if (inventory[i].Type != 0) {
            const BlockDefinition& def = g_BlockRegistry.getBlock((int)inventory[i].Type);
            Color blockColor = def.color;
            push_rect(sx + sw * 0.2f, sy + sh * 0.2f, sw * 0.6f, sh * 0.6f, blockColor);
        }
    }

    SDL_UnmapGPUTransferBuffer(this->GPU, uiMesh.VertextransferBuffer);
    SDL_UnmapGPUTransferBuffer(this->GPU, uiMesh.IndextransferBuffer);

    SDL_GPUTransferBufferLocation vloc{}; vloc.transfer_buffer = uiMesh.VertextransferBuffer; vloc.offset = 0;
    SDL_GPUBufferRegion vreg{}; vreg.buffer = uiMesh.VertexBuffer.buffer; vreg.size = vertexSize; vreg.offset = 0;
    SDL_UploadToGPUBuffer(this->copyPass, &vloc, &vreg, true);

    SDL_GPUTransferBufferLocation iloc{}; iloc.transfer_buffer = uiMesh.IndextransferBuffer; iloc.offset = 0;
    SDL_GPUBufferRegion ireg{}; ireg.buffer = uiMesh.IndexBuffer.buffer; ireg.size = indexSize; ireg.offset = 0;
    SDL_UploadToGPUBuffer(this->copyPass, &iloc, &ireg, true);

    // Create an orthographic MVP for screen space
    Matrix ortho = Matrix::Identity(4);
    // Map x:[0..W] -> [-1..1], y:[0..H] -> [-1..1]
    ortho(0,0) =  2.0f / (float)this->Width;  ortho(0,3) = -1.0f;
    ortho(1,1) = -2.0f / (float)this->Height; ortho(1,3) =  1.0f;
    ortho(2,2) = 1.0f; ortho(3,3) = 1.0f;

    SDL_PushGPUVertexUniformData(this->cmdRender, 0, ortho.data.data(), sizeof(float) * ortho.data.size());
    SDL_BindGPUVertexBuffers(this->pass, 0, &uiMesh.VertexBuffer, 1);
    SDL_BindGPUIndexBuffer(this->pass, &uiMesh.IndexBuffer, SDL_GPU_INDEXELEMENTSIZE_32BIT);
    SDL_DrawGPUIndexedPrimitives(this->pass, uiMesh.faces * 6, 1, 0, 0, 0);

    // Restore 3D MVP will be set on next frame; current draw call order draws UI last
}
SDL_GPUShader* LoadShader(SDL_GPUDevice* device, const char* filename, Uint32 sampler_count,
                          Uint32 uniform_buffer_count, Uint32 storage_buffer_count,
                          Uint32 storage_texture_count) {
    SDL_GPUShaderStage stage;
    if (SDL_strstr(filename, ".vert")) {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    } else if (SDL_strstr(filename, ".frag")) {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    } else {
        SDL_Log("Unknown shader type: %s", filename);
        return NULL;
    }

    SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(device);
    SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
    char fullpath[256];
    const char* entrypoint;
    const char* basepath = SDL_GetBasePath();

    if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        SDL_snprintf(fullpath, sizeof(fullpath), "%sassets/shaders/%s.spv", basepath, filename);
        entrypoint = "main";
        format = SDL_GPU_SHADERFORMAT_SPIRV;
    } else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL) {
        SDL_snprintf(fullpath, sizeof(fullpath), "%sassets/shaders/%s.dxil", basepath, filename);
        entrypoint = "main";
        format = SDL_GPU_SHADERFORMAT_DXIL;
    } else if (backend_formats & SDL_GPU_SHADERFORMAT_MSL) {
        SDL_snprintf(fullpath, sizeof(fullpath), "%sassets/shaders/%s.msl", basepath, filename);
        entrypoint = "main0";
        format = SDL_GPU_SHADERFORMAT_MSL;
    } else {
        SDL_Log("No supported shader format found!");
        return NULL;
    }

    size_t code_size;
    void* code = SDL_LoadFile(fullpath, &code_size);
    if (!code) {
        SDL_Log("Couldn't load shader file: %s", SDL_GetError());
        return NULL;
    }

    SDL_GPUShaderCreateInfo shader_info = {};
    shader_info.code = (Uint8*)code;
    shader_info.code_size = code_size, shader_info.entrypoint = entrypoint;
    shader_info.format = format, shader_info.stage = stage;
    shader_info.num_samplers = sampler_count;
    shader_info.num_uniform_buffers = uniform_buffer_count;
    shader_info.num_storage_buffers = storage_buffer_count;
    shader_info.num_storage_textures = storage_texture_count;

    SDL_GPUShader* shader = SDL_CreateGPUShader(device, &shader_info);
    if (!shader) {
        SDL_Log("Couldn't create shader: %s", SDL_GetError());
        SDL_free(code);
        return NULL;
    }

    SDL_free(code);
    return shader;
}
SDL_GPUTexture* Renderer::CreateDepthTexture(Uint32 drawablew, Uint32 drawableh) {
    SDL_GPUTextureCreateInfo createinfo;
    SDL_GPUTexture* result;

    createinfo.type = SDL_GPU_TEXTURETYPE_2D;
    createinfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    createinfo.width = drawablew;
    createinfo.height = drawableh;
    createinfo.layer_count_or_depth = 1;
    createinfo.num_levels = 1;
    createinfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
    createinfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    createinfo.props = 0;

    result = SDL_CreateGPUTexture(this->GPU, &createinfo);
    if (!result) {
        SDL_Log("Failed to create depth texture: %s", SDL_GetError());
        return NULL;
    }

    return result;
}

/*
Renderer::Renderer(GameClient& gameClient) : gameClient(gameClient), chunkManager() {
    this->Width = 600;
    this->Height = 400;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        assert(false);
    } else {
        std::cout << "SDL initialized successfully." << std::endl;
    }

    this->window = SDL_CreateWindow("Bit Miner", this->Width, this->Height, SDL_WINDOW_RESIZABLE);
    if (this->window == nullptr) {
        std::cout << "Error creating window: " << SDL_GetError();
        SDL_Quit();
        assert(false);
    }

    this->GPU =
        SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);

    if (this->GPU == nullptr) {
        std::cerr << "SDL GPU creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(this->window);
        SDL_Quit();
        assert(false);
    }

    if (!SDL_ClaimWindowForGPUDevice(this->GPU, this->window)) {
        std::cerr << "Error claiming window for GPU device: " << SDL_GetError() << std::endl;
    }

    // Initialize SDL_ttf
    if (!TTF_Init()) {
        std::cerr << "TTF_Init failed: \n";
        SDL_Quit();
        assert(false);
    }
    this->event = {};
    // Show OS cursor so the user can see it in the window
    SDL_SetWindowRelativeMouseMode(window, false);

    float aspect = (float)this->Width / (float)this->Height;
    this->frustum = Frustum().createFrustumFromCamera(aspect, FOV, Znear, Zfar);
    std::cout << "Frustum created with aspect: " << aspect << " FOV: " << FOV << " Znear: " << Znear
              << " Zfar: " << Zfar << std::endl;
    // load the vertex shader code
    SDL_GPUShader* vertex_shader = LoadShader(this->GPU, "Shader.vert", 0, 1, 0, 0);
    if (!vertex_shader) {
        SDL_Log("Couldn't load vertex shader: %s", SDL_GetError());
    }
    SDL_GPUShader* fragment_shader = LoadShader(this->GPU, "Shader.frag", 0, 0, 0, 0);
    if (!fragment_shader) {
        SDL_Log("Couldn't load fragment shader: %s", SDL_GetError());
    }

    std::cout << "Vertex size: " << vertexSize << "\t Index Size: " << indexSize << std::endl;
    for (int i = 0; i < (RenderDistance + 1) * (RenderDistance + 1); i++) {
        Mesh mesh{};
        SDL_GPUBufferCreateInfo vertexInfo = {};
        vertexInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vertexInfo.size = vertexSize;
        vertexInfo.props = 0;
        mesh.VertexBuffer.buffer = SDL_CreateGPUBuffer(this->GPU, &vertexInfo);
        mesh.VertexBuffer.offset = 0;
        if (!mesh.VertexBuffer.buffer) {
            SDL_Log("Couldn't create vertex buffer: %s", SDL_GetError());
        }

        // Create index buffer
        SDL_GPUBufferCreateInfo indexInfo = {};
        indexInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        indexInfo.size = indexSize;
        indexInfo.props = 0;
        mesh.IndexBuffer.buffer = SDL_CreateGPUBuffer(this->GPU, &indexInfo);
        mesh.IndexBuffer.offset = 0;

        SDL_GPUTransferBufferCreateInfo VertextransferInfo{};
        VertextransferInfo.size = vertexSize;
        VertextransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        VertextransferInfo.props = 0;
        mesh.VertextransferBuffer = SDL_CreateGPUTransferBuffer(this->GPU, &VertextransferInfo);

        SDL_GPUTransferBufferCreateInfo IndextransferInfo{};
        IndextransferInfo.size = indexSize;
        IndextransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        IndextransferInfo.props = 0;
        mesh.IndextransferBuffer = SDL_CreateGPUTransferBuffer(this->GPU, &IndextransferInfo);

        this->Terrain.push_back(mesh);
    }

    // SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{}; // TODO: Use for pipeline configuration
    // pipelineInfo.props = 0;

    // bind shaders
    SDL_GPUColorTargetDescription color_target_desc;
    SDL_GPUGraphicsPipelineCreateInfo pipeline_desc;

    SDL_zero(color_target_desc);
    SDL_zero(pipeline_desc);

    color_target_desc.format = SDL_GetGPUSwapchainTextureFormat(this->GPU, window);

    pipeline_desc.target_info.num_color_targets = 1;
    pipeline_desc.target_info.color_target_descriptions = &color_target_desc;
    pipeline_desc.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    pipeline_desc.target_info.has_depth_stencil_target = true;

    pipeline_desc.depth_stencil_state.enable_depth_test = true;
    pipeline_desc.depth_stencil_state.enable_depth_write = true;
    pipeline_desc.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

    pipeline_desc.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_desc.vertex_shader = vertex_shader;
    pipeline_desc.fragment_shader = fragment_shader;

    SDL_GPUVertexBufferDescription vertex_buffer_desc;
    SDL_GPUVertexAttribute vertex_attributes[2];

    vertex_buffer_desc.slot = 0;
    vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertex_buffer_desc.instance_step_rate = 0;
    vertex_buffer_desc.pitch = sizeof(Vertex);

    vertex_attributes[0].buffer_slot = 0;
    vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes[0].location = 0;
    vertex_attributes[0].offset = 0;

    vertex_attributes[1].buffer_slot = 0;
    vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes[1].location = 1;
    vertex_attributes[1].offset = sizeof(float) * 3;

    pipeline_desc.vertex_input_state.num_vertex_buffers =
        (RenderDistance + 1) * (RenderDistance + 1);
    pipeline_desc.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_desc;
    pipeline_desc.vertex_input_state.num_vertex_attributes = 2;
    pipeline_desc.vertex_input_state.vertex_attributes =
        (SDL_GPUVertexAttribute*)&vertex_attributes;

    pipeline_desc.props = 0;

    this->graphicsPipeline = SDL_CreateGPUGraphicsPipeline(this->GPU, &pipeline_desc);
    if (!this->graphicsPipeline) {
        SDL_Log("Failed to create pipeline: %s", SDL_GetError());
    }

    SDL_ReleaseGPUShader(this->GPU, vertex_shader);
    SDL_ReleaseGPUShader(this->GPU, fragment_shader);
    this->depthTexture = CreateDepthTexture(this->Width, this->Height);
}
*/

Renderer::Renderer(GameClient& gameClient) : gameClient(gameClient), chunkManager() {
    std::cout << "Renderer constructor started..." << std::endl;

    this->Width = 600;
    this->Height = 400;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        assert(false);
    } else {
        std::cout << "SDL initialized successfully." << std::endl;
    }

    std::cout << "Creating window..." << std::endl;
    this->window = SDL_CreateWindow("Bit Miner", this->Width, this->Height, SDL_WINDOW_RESIZABLE);
    if (this->window == nullptr) {
        std::cout << "Error creating window: " << SDL_GetError();
        SDL_Quit();
        assert(false);
    }
    std::cout << "Window created successfully." << std::endl;

    std::cout << "Creating GPU device..." << std::endl;
    this->GPU =
        SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
    if (this->GPU == nullptr) {
        std::cerr << "SDL GPU creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(this->window);
        SDL_Quit();
        assert(false);
    }
    std::cout << "GPU device created successfully." << std::endl;

    std::cout << "Claiming window for GPU..." << std::endl;
    if (!SDL_ClaimWindowForGPUDevice(this->GPU, this->window)) {
        std::cerr << "Error claiming window for GPU device: " << SDL_GetError() << std::endl;
    }
    std::cout << "Window claimed for GPU successfully." << std::endl;

    std::cout << "Initializing TTF..." << std::endl;
    if (!TTF_Init()) {
        std::cerr << "TTF_Init failed: \n";
        SDL_Quit();
        assert(false);
    }
    std::cout << "TTF initialized successfully." << std::endl;

    this->event = {};
    SDL_SetWindowRelativeMouseMode(window, true);

    std::cout << "Creating frustum..." << std::endl;
    float aspect = (float)this->Width / (float)this->Height;
    this->frustum = Frustum().createFrustumFromCamera(aspect, FOV, Znear, Zfar);
    std::cout << "Frustum created with aspect: " << aspect << " FOV: " << FOV << " Znear: " << Znear
              << " Zfar: " << Zfar << std::endl;

    std::cout << "Loading shaders..." << std::endl;
    SDL_GPUShader* vertex_shader = LoadShader(this->GPU, "Shader.vert", 0, 1, 0, 0);
    if (!vertex_shader) {
        SDL_Log("Couldn't load vertex shader: %s", SDL_GetError());
    }
    SDL_GPUShader* fragment_shader = LoadShader(this->GPU, "Shader.frag", 0, 0, 0, 0);
    if (!fragment_shader) {
        SDL_Log("Couldn't load fragment shader: %s", SDL_GetError());
    }
    std::cout << "Shaders loaded successfully." << std::endl;

    std::cout << "Creating mesh buffers..." << std::endl;
    std::cout << "Vertex size: " << vertexSize << "\t Index Size: " << indexSize << std::endl;

    int numMeshes = (RenderDistance + 1) * (RenderDistance + 1);
    std::cout << "Creating " << numMeshes << " meshes..." << std::endl;

    for (int i = 0; i < numMeshes; i++) {
        std::cout << "Creating mesh " << i << "..." << std::endl;
        Mesh mesh{};

        SDL_GPUBufferCreateInfo vertexInfo = {};
        vertexInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vertexInfo.size = vertexSize;
        vertexInfo.props = 0;
        mesh.VertexBuffer.buffer = SDL_CreateGPUBuffer(this->GPU, &vertexInfo);
        mesh.VertexBuffer.offset = 0;
        if (!mesh.VertexBuffer.buffer) {
            SDL_Log("Couldn't create vertex buffer: %s", SDL_GetError());
        }

        // Create index buffer
        SDL_GPUBufferCreateInfo indexInfo = {};
        indexInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        indexInfo.size = indexSize;
        indexInfo.props = 0;
        mesh.IndexBuffer.buffer = SDL_CreateGPUBuffer(this->GPU, &indexInfo);
        mesh.IndexBuffer.offset = 0;
        if (!mesh.IndexBuffer.buffer) {
            SDL_Log("Couldn't create index buffer: %s", SDL_GetError());
        }

        SDL_GPUTransferBufferCreateInfo VertextransferInfo{};
        VertextransferInfo.size = vertexSize;
        VertextransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        VertextransferInfo.props = 0;
        mesh.VertextransferBuffer = SDL_CreateGPUTransferBuffer(this->GPU, &VertextransferInfo);
        if (!mesh.VertextransferBuffer) {
            SDL_Log("Couldn't create vertex transfer buffer: %s", SDL_GetError());
        }

        SDL_GPUTransferBufferCreateInfo IndextransferInfo{};
        IndextransferInfo.size = indexSize;
        IndextransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        IndextransferInfo.props = 0;
        mesh.IndextransferBuffer = SDL_CreateGPUTransferBuffer(this->GPU, &IndextransferInfo);
        if (!mesh.IndextransferBuffer) {
            SDL_Log("Couldn't create index transfer buffer: %s", SDL_GetError());
        }

        this->Terrain.push_back(mesh);
        std::cout << "Mesh " << i << " created successfully." << std::endl;
    }

    // Create UI mesh buffers
    {
        SDL_GPUBufferCreateInfo vertexInfo = {};
        vertexInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vertexInfo.size = vertexSize;
        vertexInfo.props = 0;
        uiMesh.VertexBuffer.buffer = SDL_CreateGPUBuffer(this->GPU, &vertexInfo);
        uiMesh.VertexBuffer.offset = 0;

        SDL_GPUBufferCreateInfo indexInfo = {};
        indexInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        indexInfo.size = indexSize;
        indexInfo.props = 0;
        uiMesh.IndexBuffer.buffer = SDL_CreateGPUBuffer(this->GPU, &indexInfo);
        uiMesh.IndexBuffer.offset = 0;

        SDL_GPUTransferBufferCreateInfo vti{}; vti.size = vertexSize; vti.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD; vti.props = 0;
        uiMesh.VertextransferBuffer = SDL_CreateGPUTransferBuffer(this->GPU, &vti);

        SDL_GPUTransferBufferCreateInfo iti{}; iti.size = indexSize; iti.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD; iti.props = 0;
        uiMesh.IndextransferBuffer = SDL_CreateGPUTransferBuffer(this->GPU, &iti);
    }

    std::cout << "Setting up graphics pipeline..." << std::endl;

    // bind shaders
    SDL_GPUColorTargetDescription color_target_desc;
    SDL_GPUGraphicsPipelineCreateInfo pipeline_desc;

    SDL_zero(color_target_desc);
    SDL_zero(pipeline_desc);

    color_target_desc.format = SDL_GetGPUSwapchainTextureFormat(this->GPU, window);

    pipeline_desc.target_info.num_color_targets = 1;
    pipeline_desc.target_info.color_target_descriptions = &color_target_desc;
    pipeline_desc.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    pipeline_desc.target_info.has_depth_stencil_target = true;

    pipeline_desc.depth_stencil_state.enable_depth_test = true;
    pipeline_desc.depth_stencil_state.enable_depth_write = true;
    pipeline_desc.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

    pipeline_desc.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_desc.vertex_shader = vertex_shader;
    pipeline_desc.fragment_shader = fragment_shader;

    SDL_GPUVertexBufferDescription vertex_buffer_desc;
    SDL_GPUVertexAttribute vertex_attributes[2];

    vertex_buffer_desc.slot = 0;
    vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertex_buffer_desc.instance_step_rate = 0;
    vertex_buffer_desc.pitch = sizeof(Vertex);

    vertex_attributes[0].buffer_slot = 0;
    vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes[0].location = 0;
    vertex_attributes[0].offset = 0;

    vertex_attributes[1].buffer_slot = 0;
    vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes[1].location = 1;
    vertex_attributes[1].offset = sizeof(float) * 3;

    // FIX: This line was wrong - should be 1, not number of chunks
    pipeline_desc.vertex_input_state.num_vertex_buffers = 1;  // Changed from numMeshes
    pipeline_desc.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_desc;
    pipeline_desc.vertex_input_state.num_vertex_attributes = 2;
    pipeline_desc.vertex_input_state.vertex_attributes = vertex_attributes;

    pipeline_desc.props = 0;

    this->graphicsPipeline = SDL_CreateGPUGraphicsPipeline(this->GPU, &pipeline_desc);
    if (!this->graphicsPipeline) {
        SDL_Log("Failed to create pipeline: %s", SDL_GetError());
    }
    std::cout << "Graphics pipeline created successfully." << std::endl;

    SDL_ReleaseGPUShader(this->GPU, vertex_shader);
    SDL_ReleaseGPUShader(this->GPU, fragment_shader);

    std::cout << "Creating depth texture..." << std::endl;
    this->depthTexture = CreateDepthTexture(this->Width, this->Height);
    if (!this->depthTexture) {
        SDL_Log("Failed to create depth texture: %s", SDL_GetError());
    }

    std::cout << "Renderer constructor completed successfully!" << std::endl;
}
