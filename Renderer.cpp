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

        Vector3 localPos = this->rotate(relToScreen, player.Rotation);

        // std::cout << " 3D:\t Px: " << Px << " Py: " << Py << " Pz: " << Pz << std::endl;

        float screenX = localPos.x;
        float screenY = localPos.y;

        if (localPos.z <= Znear) localPos.z = Znear;

        screenX = (localPos.x / (localPos.z * FOV)) * BlockPixelSize + ScreenSize.x / 2.0f;
        screenY = (localPos.y / (localPos.z * FOV)) * BlockPixelSize + ScreenSize.y / 2.0f;
        screenY = ScreenSize.y - screenY;

        // std::cout << " 2D:\t Px: " << screenX << " Py: " << screenY << std::endl;

        Color faceColor = (this->chunkManager.GetBlock(color).color + Colors[(int)(Side / 2)]);

        Vertex vertex = {{screenX, screenY}, faceColor.ToFloat()};
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
        if (!volume.isOnFrustum(this->frustum, local)) continue;
        // std::cout << Max.x << std::endl;
        Faces.push_back({face.blockPos, face.side, face.blockID, Max.z, face.blockID == 5});
    }
    std::sort(Faces.begin(), Faces.end(), [](const DrawnFace& a, const DrawnFace& b) {
        if (a.maxZ != b.maxZ) return a.maxZ > b.maxZ;
        if (a.Transparent != b.Transparent) return !a.Transparent && b.Transparent;
        return false;  // consider them equal if both fields match
    });

    Vertex* Vertexdata =
        (Vertex*)SDL_MapGPUTransferBuffer(this->GPU, mesh->VertextransferBuffer, false);
    Uint32* Indexdata =
        (Uint32*)SDL_MapGPUTransferBuffer(this->GPU, mesh->IndextransferBuffer, false);

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
    SDL_UploadToGPUBuffer(this->copyPass, &Vertexlocation, &Vertexregion, false);

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
    SDL_UploadToGPUBuffer(this->copyPass, &Indexlocation, &Indexregion, false);
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

    this->copyPass = SDL_BeginGPUCopyPass(this->cmdCopy);

    SDL_GPUColorTargetInfo colorInfo = {};
    colorInfo.clear_color = SDL_FColor{0.0f, 0.69f, 1.0f, 1.0f};
    colorInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorInfo.store_op = SDL_GPU_STOREOP_STORE;
    colorInfo.texture = swap_texture;

    // SDL_SetGPUViewport(pass, NULL);
    DrawTerrain(players[0]);

    // Stats(player);
    // DrawBG(renderer, players[0],{ (float)width, (float)height, 0}, texture);
    // ChunckManager::ShowInventor(renderer, width, height, std::ref(inventory), inventorySlot,
    // font); DrawPlayer(renderer, Range, std::ref(players)); SDL_Delay(1000 / 10);
    SDL_EndGPUCopyPass(this->copyPass);

    this->pass = SDL_BeginGPURenderPass(this->cmdRender, &colorInfo, 1, NULL);
    SDL_BindGPUGraphicsPipeline(this->pass, this->graphicsPipeline);

    int i = 0;
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
        SDL_DrawGPUIndexedPrimitives(this->pass, 42000, 1, 0, 0, 0);
    }

    SDL_EndGPURenderPass(this->pass);
    if (!SDL_SubmitGPUCommandBuffer(this->cmdRender)) {
        std::cout << "Heil el render de Puigdemont\n";
    }
    if (!SDL_SubmitGPUCommandBuffer(this->cmdCopy)) {
        std::cout << "Heil la memoria de Puigdemont\n";
    }
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

    this->window = SDL_CreateWindow("Bit Miner", 600, 400, SDL_WINDOW_RESIZABLE);
    if (this->window == nullptr) {
        std::cout << "Error creating window: " << SDL_GetError();
        SDL_Quit();
        assert(false);
    }

    // Pick GPU backend depending on OS
#ifdef _WIN32
    this->GPU = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL, false, "direct3d12");
#else
    this->GPU = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
#endif

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

    std::cout << "Vertex size: " << vertexSize << "\t Index Size: " << indexSize << std::endl;
    for (int i = 0; i < (RenderDistance + 1) * (RenderDistance + 1); i++) {
        Mesh mesh{};
        SDL_GPUBufferCreateInfo vertexInfo = {};
        vertexInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vertexInfo.size = vertexSize;
        vertexInfo.props = 0;
        mesh.VertexBuffer.buffer = SDL_CreateGPUBuffer(this->GPU, &vertexInfo);
        mesh.VertexBuffer.offset = 0;

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

    // load the vertex shader code
    size_t vertexCodeSize;
    void* vertexCode = SDL_LoadFile("VertexShader.cso", &vertexCodeSize);

    // create the vertex shader
    SDL_GPUShaderCreateInfo vertexInfo{};
    vertexInfo.code = (Uint8*)vertexCode;  // convert to an array of bytes
    vertexInfo.code_size = vertexCodeSize;
    vertexInfo.entrypoint = "VSMain";
    vertexInfo.format = SDL_GPU_SHADERFORMAT_DXIL;  // loading .spv shaders
    vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;  // vertex shader
    vertexInfo.num_samplers = 0;
    vertexInfo.num_storage_buffers = 0;
    vertexInfo.num_storage_textures = 0;
    vertexInfo.num_uniform_buffers = 0;
    SDL_GPUShader* vertexShader = SDL_CreateGPUShader(this->GPU, &vertexInfo);

    SDL_free(vertexCode);

    if (vertexShader == NULL) {
        std::cout << "Error creating vertex shader because " << SDL_GetError();
    }
    // create the fragment shader
    size_t fragmentCodeSize;
    void* fragmentCode = SDL_LoadFile("PixelShader.cso", &fragmentCodeSize);

    // create the fragment shader
    SDL_GPUShaderCreateInfo fragmentInfo{};
    fragmentInfo.code = (Uint8*)fragmentCode;
    fragmentInfo.code_size = fragmentCodeSize;
    fragmentInfo.entrypoint = "PSMain";
    fragmentInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
    fragmentInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;  // fragment shader
    fragmentInfo.num_samplers = 0;
    fragmentInfo.num_storage_buffers = 0;
    fragmentInfo.num_storage_textures = 0;
    fragmentInfo.num_uniform_buffers = 0;

    SDL_GPUShader* fragmentShader = SDL_CreateGPUShader(this->GPU, &fragmentInfo);

    // free the file
    SDL_free(fragmentCode);

    if (fragmentShader == NULL) {
        std::cout << "Error creating fragment shader because " << SDL_GetError();
    }

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.props = 0;

    // bind shaders
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;

    // draw triangles
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    // describe the vertex buffers
    SDL_GPUVertexBufferDescription vertexBufferDesctiptions = {};
    vertexBufferDesctiptions.slot = 0;
    vertexBufferDesctiptions.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDesctiptions.instance_step_rate = 0;
    vertexBufferDesctiptions.pitch = sizeof(Vertex);

    pipelineInfo.vertex_input_state.num_vertex_buffers =
        (RenderDistance + 1) * (RenderDistance + 1);
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = &vertexBufferDesctiptions;

    // describe the vertex attribute
    SDL_GPUVertexAttribute vertexAttributes[2] = {};

    // a_position
    vertexAttributes[0].buffer_slot = 0;  // fetch data from the buffer at slot 0
    vertexAttributes[0].location = 0;     // layout (location = 0) in shader
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;  // vec3
    vertexAttributes[0].offset = 0;  // start from the first byte from current buffer position

    // a_color
    vertexAttributes[1].buffer_slot = 0;  // use buffer at slot 0
    vertexAttributes[1].location = 1;     // layout (location = 1) in shader
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;  // vec4
    vertexAttributes[1].offset = sizeof(Vector3);  // 4th float from current buffer position

    pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
    pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

    // describe the color target
    SDL_GPUColorTargetDescription colorTargetDescriptions[1] = {};
    colorTargetDescriptions[0] = {};
    colorTargetDescriptions[0].format = SDL_GetGPUSwapchainTextureFormat(this->GPU, window);
    SDL_GPUColorTargetBlendState blend = {};
    blend.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    blend.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.color_blend_op = SDL_GPU_BLENDOP_ADD;
    blend.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    blend.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    blend.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    blend.color_write_mask = SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G |
                            SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;
    blend.enable_blend = true; 
    blend.enable_color_write_mask = true;

    colorTargetDescriptions[0].blend_state = blend;

    if (colorTargetDescriptions[0].format == SDL_GPU_TEXTUREFORMAT_INVALID) {
        std::cout << "SDL_GPU_TEXTUREFORMAT_INVALID";
    }
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;

    // create the pipeline
    this->graphicsPipeline = SDL_CreateGPUGraphicsPipeline(this->GPU, &pipelineInfo);
    if (this->graphicsPipeline == NULL) {
        std::cout << "Creating graphicPipeline failed because: " << SDL_GetError() << std::endl;
    }

    // we don't need to store the shaders after creating the pipeline
    SDL_ReleaseGPUShader(this->GPU, vertexShader);
    SDL_ReleaseGPUShader(this->GPU, fragmentShader);
}
