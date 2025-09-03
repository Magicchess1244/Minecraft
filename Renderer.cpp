#include "Renderer.hpp"
#include "GameClient.hpp"


constexpr Uint32 vertexSize = sizeof(Vertex) * 4 * 7000;
constexpr Uint32 indexSize = sizeof(Uint32) * 6 * 7000;
const float FOV = tan((45 * (PI / 180)) / 2.0f);
const float Znear = 1 / FOV;
constexpr float Zfar = 1000.0f;
constexpr int RenderDistance = 0;
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
    {0, 0, -1},  // Front
    {0, 0, 1},   // Back
    {1, 0, 0},   // Right
    {-1, 0, 0},  // Left
    {0, 1, 0},   // Top
    {0, -1, 0}   // Bottom
};
const Color Colors[3] = {
    {0, 0, 0},     // Front / Back
    {5, 5, 5},     // Right / Left
    {10, 10, 10},  // Top / Bottom
};

Matrix Perspective(float fovRadians, float aspect, float Near, float Far) {
    float f = 1.0f / std::tan(fovRadians / 2.0f);
    Matrix m(4, 4, 0.0f);
    m(0, 0) = f / aspect;
    m(1, 1) = f;
    m(2, 2) = (Far + Near) / (Near - Far);
    m(2, 3) = (2.0f * Far * Near) / (Near - Far);
    m(3, 2) = 1.0f;
    return m;
}
Matrix Translation(float x, float y, float z) {
    Matrix m = Matrix::Identity(4);
    m(0, 3) = x;
    m(1, 3) = y;
    m(2, 3) = z;
    return m;
}
Matrix RotationY(float angleRad) {
    Matrix m = Matrix::Identity(4);
    float c = std::cos(angleRad);
    float s = std::sin(angleRad);
    m(0, 0) = c;
    m(0, 2) = s;
    m(2, 0) = -s;
    m(2, 2) = c;
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
Vector3 Renderer::rotate(const Vector3 pos, Vector3 Angle) {
    Vector3 angleRadians = Angle.AngleToRadians();
    float cx = cos(angleRadians.x), sx = sin(angleRadians.x);
    float cy = cos(angleRadians.y), sy = sin(angleRadians.y);
    float cz = cos(angleRadians.z), sz = sin(angleRadians.z);

    Vector3 result = {0};

    result.x = pos.x * (cy * cz) + pos.y * (-cy * sz) + pos.z * (sy);

    result.y =
        pos.x * (sx * sy * cz + cx * sz) + pos.y * (-sx * sy * sz + cx * cz) + pos.z * (-sx * cy);

    result.z =
        pos.x * (-cx * sy * cz + sx * sz) + pos.y * (cx * sy * sz + sx * cz) + pos.z * (cx * cy);

    return result;
}
void Renderer::DrawFace(Player& player, Vector3 blocks, int color, int Side, Mesh* mesh,
                        Vertex* Vertexdata, Uint32* Indexdata) {
    const Vector3* verts = Verts[Side];
    const int baseVertex = mesh->faces * 4;
    const int baseIndex = mesh->faces * 6;

    // std::cout << "\n \n";
    for (int i = 0; i < 4; i++) {
        Vector3 relToScreen = ((verts[i] + blocks) - player.Position);

        Color faceColor = (this->chunkManager.GetBlock(color).color + Colors[(int)(Side / 2)]);

        Vertex vertex = {relToScreen, {1, 1, 1}};
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
    Vector3 Radiants = player.Rotation.AngleToRadians();
    auto* mesh = &this->Terrain[NumChunk];
    mesh->faces = 0;

    Vector3 CameraDir = {
        cosf(Radiants.x) * sinf(Radiants.y),  // X
        sinf(Radiants.x),                     // Y
        cosf(Radiants.x) * cosf(Radiants.y)   // Z
    };

    //    int chunkX = (int)floor((player.Position.x) / 32);
    //    int chunkZ = (int)floor((player.Position.z) / 32);
    std::vector<DrawnFace> Faces;
    //    int index = 0;

    for (const auto& face : chunk.allFaces) {
        // std::cout << face << std::endl;
        if (CameraDir.Dot(Direction[face.side]) >= 0.3) continue;
        Vector3 Max, Min;
        Vector3 local[4];
        for (int j = 0; j < 4; j++) {
            Vector3 worldFacePos = face.blockPos + Verts[face.side][j];
            local[j] = rotate((worldFacePos - player.Position), player.Rotation);
            if (j == 0) {
                Max = Min = local[j];
            } else {
                Max = Max.Max(local[j]);
                Min = Min.Min(local[j]);
            }
        }
        AABB volume(Min, Max);
        if (Max.z < Znear) continue;
        //if (!volume.isOnFrustum(this->frustum, local)) continue;
        // std::cout << Max.x << std::endl;
        Faces.push_back({face.blockPos, face.side, face.blockID, Max.z, face.blockID == 5});
    }
    std::sort(Faces.begin(), Faces.end(), [](const DrawnFace& a, const DrawnFace& b) {
        if (a.maxZ != b.maxZ) return a.maxZ > b.maxZ;
        if (a.Transparent != b.Transparent) return !a.Transparent && b.Transparent;
        return false;  // consider them equal if both fields match
    });

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
    std::vector<std::thread> threads;
    std::cout << "Nigga" << std::endl;

    int threadIndex = 0;
    Vector3 PlayerChunk = (player.Position / 32).Truncate();
    for (int i = -RenderDistance; i <= RenderDistance; i++) {
        for (int j = -RenderDistance; j <= RenderDistance; j++) {
            Vector3 Chunk = {(float)i, 0, (float)j};
            Chunk += PlayerChunk;

            threads.emplace_back(&Renderer::RenderChunk, this,
                                 std::ref(chunkManager.get_chunk(Chunk)), std::ref(player), 0);

            threadIndex++;
        }
    }

    for (auto& t : threads) {
        if (t.joinable()) t.join();
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

    SDL_Color White = {200, 200, 200, 1};

    if (!this->font) {
        std::cerr << "Font not loaded!\n";
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(this->font, text.c_str(), text.length(), White);
    if (!surface) {
        std::cerr << "TTF_RenderText_Blended failed: \n";
        return;
    }

    // SDL_Texture* texture = SDL_CreateTextureFromSurface(this->renderer, surface);
    if (!texture) {
        std::cerr << "SDL_CreateTextureFromSurface failed: " << SDL_GetError() << "\n";
        SDL_DestroySurface(surface);
        return;
    }

    //    SDL_FRect dstRect{10, 10, (float)surface->w, (float)surface->h};
    // SDL_RenderTexture(this->renderer, texture, NULL, &dstRect);

    // SDL_DestroyTexture(texture);
    // SDL_DestroySurface(surface);
}
void Renderer::MainRenderLoop(std::vector<Slot>& inventory, int inventorySlot,
                              std::vector<Player>& players) {
    while (SDL_PollEvent(&this->event)) {
        switch (this->event.type) {
            case SDL_EVENT_QUIT:
                // Handle quitting the game
                this->gameClient.Quit();
                break;

            case SDL_EVENT_WINDOW_RESIZED:
                // Update screen size
                this->ScreenSize.x = (int)this->event.window.data1;
                this->ScreenSize.y = (int)this->event.window.data2;

                // Optionally update chunk manager here
                // ChunckManager::Size(...);
                break;

            case SDL_EVENT_KEY_DOWN: {
                SDL_Keycode key = this->event.key.scancode;

                if (key == SDL_SCANCODE_ESCAPE) {
                    this->gameClient.Quit();
                    break;
                } else if (key == SDL_SCANCODE_F11) {
                    // Toggle fullscreen
                    this->fullScreen = !this->fullScreen;
                    SDL_SetWindowFullscreen(this->window, this->fullScreen);
                }
                break;
            }

            case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                // Handle mouse input
                if (this->event.button.button == SDL_BUTTON_LEFT) {
                    // Handle block breaking
                    //                    short Type = 0;
                    if (false /*ChunckManager::PlaceBlock(...)*/) {
                        // short Slot = FindSlot(inventory, Type);
                        // inventory[Slot].Amount++;
                        // inventory[Slot].Type = Type;
                        // ChunckManager::SimulateWater(...);
                    }
                } else if (this->event.button.button == SDL_BUTTON_RIGHT &&
                           inventory[inventorySlot].Amount > 0) {
                    // Handle block placing
                    //                   short Type = 0;
                    if (false /*ChunckManager::PlaceBlock(...)*/) {
                        inventory[inventorySlot].Amount--;

                        if (inventory[inventorySlot].Amount == 0) inventory[inventorySlot].Type = 0;

                        // ChunckManager::SimulateWater(...);
                    }
                }
                break;
            }
        }
    }

    this->cmdRender = SDL_AcquireGPUCommandBuffer(this->GPU);
    this->cmdCopy = SDL_AcquireGPUCommandBuffer(this->GPU);

    // 2. Get the current window framebuffer
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

    // SDL_SetGPUViewport(pass, NULL);
    DrawTerrain(players[0]);
    /*
    Vertex* Vertexdata = (Vertex*)SDL_MapGPUTransferBuffer(this->GPU, this->Terrain[0].VertextransferBuffer, true);
    Uint32* Indexdata = (Uint32*)SDL_MapGPUTransferBuffer(this->GPU, this->Terrain[0].IndextransferBuffer, true);

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

    int i = 0;
    Matrix model = RotationY(30.0f * 3.141592 / 180.0f);

    // Camera at (3,0,5), looking at origin, up = Y axis
    std::vector<float> eye = {3.0f, 0.0f, 5.0f};
    std::vector<float> target = {0.0f, 0.0f, -1.0f};
    std::vector<float> up = {0.0f, 1.0f, 0.0f};
    Matrix view = LookAt(eye, target, up);

    // Projection
    Matrix proj = Perspective(FOV, 16.0f / 9.0f, 0.1f, 100.0f);

    // Final MVP matrix
    Matrix mvp = proj * view * model;

    SDL_PushGPUVertexUniformData(this->cmdRender, 0, mvp.data.data(), sizeof(float) * mvp.data.size());
    std::cout << "MVP matrix:\n";
    mvp.print();

    for (auto& mesh : this->Terrain) {
        std::cout << "Mesh pointer: " << &mesh << "\tVertexBuffer pointer: " << &mesh.VertexBuffer
                  << "\t Index Buffer pointer: " << &mesh.IndexBuffer << std::endl;
        std::cout << "Pass pointer: " << this->pass
                  << "\t Index size: " << SDL_GPU_INDEXELEMENTSIZE_32BIT
                  << "\t Amount of faces: " << mesh.faces << std::endl;
        std::cout << "Terrain size: " << this->Terrain.size() << "Drawing Chunk[" << i++ << "]\n"
                  << std::endl;

        SDL_BindGPUVertexBuffers(this->pass, 0, &mesh.VertexBuffer, 1);
        SDL_BindGPUIndexBuffer(this->pass, &mesh.IndexBuffer, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_DrawGPUIndexedPrimitives(this->pass, mesh.faces * 6, 1, 0, 0, 0);
    }

    SDL_EndGPURenderPass(this->pass);
    
    if (!SDL_SubmitGPUCommandBuffer(this->cmdRender)) {
        std::cout << "Heil el render de Puigdemont\n";
    }
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
        SDL_snprintf(fullpath, sizeof(fullpath), "%sshaders/%s.spv", basepath, filename);
        entrypoint = "main";
        format = SDL_GPU_SHADERFORMAT_SPIRV;
    } else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL) {
        SDL_snprintf(fullpath, sizeof(fullpath), "%sshaders/%s.dxil", basepath, filename);
        entrypoint = "main";
        format = SDL_GPU_SHADERFORMAT_DXIL;
    } else if (backend_formats & SDL_GPU_SHADERFORMAT_MSL) {
        SDL_snprintf(fullpath, sizeof(fullpath), "%sshaders/%s.msl", basepath, filename);
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

    this->GPU = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);

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
    /*
    this->font =
    TTF_OpenFont("C:\\Users\\pumu\\source\\repos\\2Dminecraft\\x64\\Release\\Quantico-Bold.ttf",
    14); if (!this->font) { std::cerr << "Font is null!" << std::endl;
    }

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_FRect rect = { 0,0,100,100 };
    SDL_RenderFillRect(renderer, &rect);

    SDL_Color White = { 200, 200, 200 };
    SDL_Surface* surface = TTF_RenderText_Blended(font, "HelloWorld SDL3 TTF",sizeof("HelloWorld
    SDL3 TTF"), White); SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    SDL_FRect dstRect{ 100, 100, 200, 80 };
    SDL_RenderTexture(renderer, texture, NULL, &dstRect);
    SDL_DestroyTexture(texture);

    SDL_RenderPresent(renderer);

    SDL_Surface* surface =
    SDL_LoadBMP("C:\\Users\\pumu\\source\\repos\\2Dminecraft\\x64\\Release\\Textures.bmp"); if
    (!surface) { std::cerr << "SDL_LoadBMP failed: " << SDL_GetError() << std::endl;
    }

    //this->texture = SDL_CreateTextureFromSurface(this->renderer, surface);
    SDL_DestroySurface(surface);
    if (!texture) {
            std::cerr << "SDL_CreateTextureFromSurface failed: " << SDL_GetError() << std::endl;
    }
    */
    this->event = {};
    SDL_SetWindowRelativeMouseMode(window, true);

    float aspect = this->ScreenSize.x / this->ScreenSize.y;
    this->frustum = Frustum().createFrustumFromCamera(aspect, FOV, Znear, Zfar);

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

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.props = 0;

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

    pipeline_desc.vertex_input_state.num_vertex_buffers = (RenderDistance + 1) * (RenderDistance + 1);
    pipeline_desc.vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_desc;
    pipeline_desc.vertex_input_state.num_vertex_attributes = 2;
    pipeline_desc.vertex_input_state.vertex_attributes = (SDL_GPUVertexAttribute*)&vertex_attributes;

    pipeline_desc.props = 0;

    this->graphicsPipeline = SDL_CreateGPUGraphicsPipeline(this->GPU, &pipeline_desc);
    if (!this->graphicsPipeline) {
        SDL_Log("Failed to create pipeline: %s", SDL_GetError());
    }

    // we don't need to store the shaders after creating the pipeline
    SDL_ReleaseGPUShader(this->GPU, vertex_shader);
    SDL_ReleaseGPUShader(this->GPU, fragment_shader);
    this->depthTexture = CreateDepthTexture(this->Width, this->Height);
}
