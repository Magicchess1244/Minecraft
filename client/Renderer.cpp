#include "Renderer.hpp"

#include <SDL3_ttf/SDL_ttf.h>

#include "GameClient.hpp"

// Cube face vertices (local space)
const Vector3 FACE_VERTS[6][4] = {
    // Front (-Z)
    {{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}},
    // Back (+Z)
    {{0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}},
    // Right (+X)
    {{0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, 0.5f}},
    // Left (-X)
    {{-0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, -0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, -0.5f}},
    // Top (+Y)
    {{-0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}},
    // Bottom (-Y)
    {{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}}};

const Vector3 FACE_NORMALS[6] = {
    {0, 0, -1},  // Front
    {0, 0, 1},   // Back
    {1, 0, 0},   // Right
    {-1, 0, 0},  // Left
    {0, 1, 0},   // Top
    {0, -1, 0}   // Bottom
};

const Color FACE_COLORS[3] = {
    {0, 0, 0},     // Front/Back
    {20, 20, 20},  // Right/Left
    {40, 40, 40}   // Top/Bottom
};

Renderer::Renderer() : chunkManager() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "Error initializing SDL: " << SDL_GetError() << std::endl;
        throw std::runtime_error("SDL initialization failed");
    }

    std::cout << "SDL initialized successfully." << std::endl;
    this->window = SDL_CreateWindow("Bit Miner", static_cast<int>(screenSize.x),
                                    static_cast<int>(screenSize.y), SDL_WINDOW_RESIZABLE);
    if (!this->window) {
        std::cerr << "Error creating window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        throw std::runtime_error("Window creation failed");
    }

#ifdef _WIN32
    this->GPU = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_DXIL, true, "direct3d12");
#else
    this->GPU = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, "vulkan");
#endif

    if (!this->GPU) {
        std::cerr << "SDL GPU creation failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(this->window);
        SDL_Quit();
        throw std::runtime_error("GPU device creation failed");
    }

    if (!SDL_ClaimWindowForGPUDevice(this->GPU, this->window)) {
        std::cerr << "Error claiming window for GPU device: " << SDL_GetError() << std::endl;
        throw std::runtime_error("GPU window claim failed");
    }

    // Initialize TTF
    if (!TTF_Init()) {
        std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
    }

    // Set up mouse capture
    SDL_SetWindowRelativeMouseMode(window, true);

    // Create frustum
    float aspect = screenSize.x / screenSize.y;
    this->frustum = Frustum().createFrustumFromCamera(aspect, FOV_RADIANS, Z_NEAR, Z_FAR);

    // Create terrain meshes
    int numChunks = (RENDER_DISTANCE * 2 + 1) * (RENDER_DISTANCE * 2 + 1);
    terrain.reserve(numChunks);

    for (int i = 0; i < numChunks; i++) {
        Mesh mesh{};

        // Create vertex buffer
        SDL_GPUBufferCreateInfo vertexInfo{};
        vertexInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        vertexInfo.size = VERTEX_SIZE;
        mesh.vertexBuffer.buffer = SDL_CreateGPUBuffer(this->GPU, &vertexInfo);
        mesh.vertexBuffer.offset = 0;

        if (!mesh.vertexBuffer.buffer) {
            std::cerr << "Failed to create vertex buffer: " << SDL_GetError() << std::endl;
            throw std::runtime_error("Vertex buffer creation failed");
        }

        // Create index buffer
        SDL_GPUBufferCreateInfo indexInfo{};
        indexInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
        indexInfo.size = INDEX_SIZE;
        mesh.indexBuffer.buffer = SDL_CreateGPUBuffer(this->GPU, &indexInfo);
        mesh.indexBuffer.offset = 0;

        if (!mesh.indexBuffer.buffer) {
            std::cerr << "Failed to create index buffer: " << SDL_GetError() << std::endl;
            SDL_ReleaseGPUBuffer(this->GPU, mesh.vertexBuffer.buffer);
            throw std::runtime_error("Index buffer creation failed");
        }

        // Create transfer buffers
        SDL_GPUTransferBufferCreateInfo vertexTransferInfo{};
        vertexTransferInfo.size = VERTEX_SIZE;
        vertexTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        mesh.vertexTransferBuffer = SDL_CreateGPUTransferBuffer(this->GPU, &vertexTransferInfo);

        if (!mesh.vertexTransferBuffer) {
            std::cerr << "Failed to create vertex transfer buffer: " << SDL_GetError() << std::endl;
            SDL_ReleaseGPUBuffer(this->GPU, mesh.vertexBuffer.buffer);
            SDL_ReleaseGPUBuffer(this->GPU, mesh.indexBuffer.buffer);
            throw std::runtime_error("Vertex transfer buffer creation failed");
        }

        SDL_GPUTransferBufferCreateInfo indexTransferInfo{};
        indexTransferInfo.size = INDEX_SIZE;
        indexTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        mesh.indexTransferBuffer = SDL_CreateGPUTransferBuffer(this->GPU, &indexTransferInfo);

        if (!mesh.indexTransferBuffer) {
            std::cerr << "Failed to create index transfer buffer: " << SDL_GetError() << std::endl;
            SDL_ReleaseGPUBuffer(this->GPU, mesh.vertexBuffer.buffer);
            SDL_ReleaseGPUBuffer(this->GPU, mesh.indexBuffer.buffer);
            SDL_ReleaseGPUTransferBuffer(this->GPU, mesh.vertexTransferBuffer);
            throw std::runtime_error("Index transfer buffer creation failed");
        }

        terrain.push_back(mesh);
    }

    // Create shaders and pipeline
    createGraphicsPipeline();

    std::cout << "Renderer initialized successfully." << std::endl;
}

void Renderer::createGraphicsPipeline() {
    // Load vertex shader
    size_t vertexCodeSize;
    void* vertexCode = nullptr;

#ifdef _WIN32
    vertexCode = SDL_LoadFile("VertexShader.cso", &vertexCodeSize);
#else
    vertexCode = SDL_LoadFile("VertexShader.spv", &vertexCodeSize);
#endif

    if (!vertexCode) {
        std::cerr << "Failed to load vertex shader" << std::endl;
        throw std::runtime_error("Vertex shader load failed");
    }

    SDL_GPUShaderCreateInfo vertexShaderInfo{};
    vertexShaderInfo.code = static_cast<Uint8*>(vertexCode);
    vertexShaderInfo.code_size = vertexCodeSize;
    vertexShaderInfo.entrypoint = "main";
#ifdef _WIN32
    vertexShaderInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
#else
    vertexShaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
#endif
    vertexShaderInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;

    SDL_GPUShader* vertexShader = SDL_CreateGPUShader(this->GPU, &vertexShaderInfo);
    SDL_free(vertexCode);

    if (!vertexShader) {
        std::cerr << "Failed to create vertex shader: " << SDL_GetError() << std::endl;
        throw std::runtime_error("Vertex shader creation failed");
    }

    // Load fragment shader
    size_t fragmentCodeSize;
    void* fragmentCode = nullptr;

#ifdef _WIN32
    fragmentCode = SDL_LoadFile("FragmentShader.cso", &fragmentCodeSize);
#else
    fragmentCode = SDL_LoadFile("FragmentShader.spv", &fragmentCodeSize);
#endif

    if (!fragmentCode) {
        std::cerr << "Failed to load fragment shader" << std::endl;
        SDL_ReleaseGPUShader(this->GPU, vertexShader);
        throw std::runtime_error("Fragment shader load failed");
    }

    SDL_GPUShaderCreateInfo fragmentShaderInfo{};
    fragmentShaderInfo.code = static_cast<Uint8*>(fragmentCode);
    fragmentShaderInfo.code_size = fragmentCodeSize;
    fragmentShaderInfo.entrypoint = "main";
#ifdef _WIN32
    fragmentShaderInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
#else
    fragmentShaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
#endif
    fragmentShaderInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;

    SDL_GPUShader* fragmentShader = SDL_CreateGPUShader(this->GPU, &fragmentShaderInfo);
    SDL_free(fragmentCode);

    if (!fragmentShader) {
        std::cerr << "Failed to create fragment shader: " << SDL_GetError() << std::endl;
        SDL_ReleaseGPUShader(this->GPU, vertexShader);
        throw std::runtime_error("Fragment shader creation failed");
    }

    // Create graphics pipeline
    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    // Vertex input description
    SDL_GPUVertexBufferDescription vertexBufferDesc{};
    vertexBufferDesc.slot = 0;
    vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDesc.instance_step_rate = 0;
    vertexBufferDesc.pitch = sizeof(Vertex);

    SDL_GPUVertexAttribute vertexAttributes[2];
    // Position attribute
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].location = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[0].offset = 0;

    // Color attribute
    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].location = 1;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[1].offset = sizeof(float) * 3;

    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = &vertexBufferDesc;
    pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
    pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

    // Rasterizer state
    pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

    // Color target
    SDL_GPUColorTargetDescription colorTarget{};
    colorTarget.format = SDL_GetGPUSwapchainTextureFormat(this->GPU, window);

    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTarget;

    this->graphicsPipeline = SDL_CreateGPUGraphicsPipeline(this->GPU, &pipelineInfo);

    if (!this->graphicsPipeline) {
        std::cerr << "Failed to create graphics pipeline: " << SDL_GetError() << std::endl;
        SDL_ReleaseGPUShader(this->GPU, vertexShader);
        SDL_ReleaseGPUShader(this->GPU, fragmentShader);
        throw std::runtime_error("Graphics pipeline creation failed");
    }

    // Clean up shaders
    SDL_ReleaseGPUShader(this->GPU, vertexShader);
    SDL_ReleaseGPUShader(this->GPU, fragmentShader);
}

Renderer::~Renderer() {
    // Clean up terrain meshes
    for (auto& mesh : terrain) {
        if (mesh.indexBuffer.buffer) {
            SDL_ReleaseGPUBuffer(this->GPU, mesh.indexBuffer.buffer);
        }
        if (mesh.vertexBuffer.buffer) {
            SDL_ReleaseGPUBuffer(this->GPU, mesh.vertexBuffer.buffer);
        }
        if (mesh.indexTransferBuffer) {
            SDL_ReleaseGPUTransferBuffer(this->GPU, mesh.indexTransferBuffer);
        }
        if (mesh.vertexTransferBuffer) {
            SDL_ReleaseGPUTransferBuffer(this->GPU, mesh.vertexTransferBuffer);
        }
    }

    if (this->graphicsPipeline) {
        SDL_ReleaseGPUGraphicsPipeline(this->GPU, this->graphicsPipeline);
    }

    if (this->GPU) {
        SDL_DestroyGPUDevice(this->GPU);
    }

    if (this->window) {
        SDL_DestroyWindow(this->window);
    }

    if (this->font) {
        TTF_CloseFont(this->font);
    }

    TTF_Quit();
    SDL_Quit();
}

Vector3 Renderer::rotate(const Vector3& pos, const Vector3& angle) {
    Vector3 angleRadians = {static_cast<float>(angle.x * (M_PI / 180.0)),
                            static_cast<float>(angle.y * (M_PI / 180.0)),
                            static_cast<float>(angle.z * (M_PI / 180.0))};

    float cx = cos(angleRadians.x), sx = sin(angleRadians.x);
    float cy = cos(angleRadians.y), sy = sin(angleRadians.y);
    float cz = cos(angleRadians.z), sz = sin(angleRadians.z);

    Vector3 result;
    result.x = pos.x * (cy * cz) + pos.y * (-cy * sz) + pos.z * (sy);
    result.y =
        pos.x * (sx * sy * cz + cx * sz) + pos.y * (-sx * sy * sz + cx * cz) + pos.z * (-sx * cy);
    result.z =
        pos.x * (-cx * sy * cz + sx * sz) + pos.y * (cx * sy * sz + sx * cz) + pos.z * (cx * cy);

    return result;
}

void Renderer::drawFace(Player& player, const Vector3& blockPos, int color, int side, Mesh* mesh,
                        Vertex* vertexData, Uint32* indexData) {
    if (mesh->faces >= 7000) {
        return;  // Prevent buffer overflow
    }

    const Vector3* verts = FACE_VERTS[side];
    const int baseVertex = mesh->faces * 4;
    const int baseIndex = mesh->faces * 6;

    float tanHalfFov = tan(FOV_RADIANS * 0.5f);

    for (int i = 0; i < 4; i++) {
        Vector3 worldPos = verts[i] + blockPos;
        Vector3 relativePos = worldPos - player.Position;
        Vector3 localPos = rotate(relativePos, player.Rotation);

        // Ensure we don't divide by zero or negative Z
        if (localPos.z <= Z_NEAR) {
            localPos.z = Z_NEAR + 0.01f;
        }

        // Project to screen space
        float screenX = (localPos.x / (localPos.z * tanHalfFov)) * (screenSize.x * 0.5f) +
                        (screenSize.x * 0.5f);
        float screenY = (localPos.y / (localPos.z * tanHalfFov)) * (screenSize.y * 0.5f) +
                        (screenSize.y * 0.5f);
        screenY = screenSize.y - screenY;  // Flip Y coordinate

        // Get face color
        Color faceColor = chunkManager.GetBlock(color).color + FACE_COLORS[side / 2];
        Vector3 colorFloat = faceColor.ToFloat();

        Vertex vertex;
        vertex.position = {screenX, screenY, localPos.z};
        vertex.color = colorFloat;
        vertexData[baseVertex + i] = vertex;
    }

    // Set up indices for two triangles
    indexData[baseIndex + 0] = baseVertex + 0;
    indexData[baseIndex + 1] = baseVertex + 1;
    indexData[baseIndex + 2] = baseVertex + 2;

    indexData[baseIndex + 3] = baseVertex + 2;
    indexData[baseIndex + 4] = baseVertex + 1;
    indexData[baseIndex + 5] = baseVertex + 3;

    mesh->faces++;
}

void Renderer::renderChunk(const ChunkPrefab& chunk, Player& player, int numChunk) {
    if (numChunk >= static_cast<int>(terrain.size())) {
        return;
    }

    auto& mesh = terrain[numChunk];
    mesh.faces = 0;

    Vector3 radiants = {static_cast<float>(player.Rotation.x * (M_PI / 180.0)),
                        static_cast<float>(player.Rotation.y * (M_PI / 180.0)),
                        static_cast<float>(player.Rotation.z * (M_PI / 180.0))};

    Vector3 cameraDir = {static_cast<float>(cos(radiants.x) * sin(radiants.y)),
                         static_cast<float>(sin(radiants.x)),
                         static_cast<float>(cos(radiants.x) * cos(radiants.y))};

    std::vector<DrawnFace> faces;

    for (const auto& face : chunk.allFaces) {
        // Backface culling
        if (cameraDir.Dot(FACE_NORMALS[face.side]) >= 0.1f) {
            continue;
        }

        Vector3 faceCenter = face.blockPos;
        Vector3 relativePos = faceCenter - player.Position;
        Vector3 localPos = rotate(relativePos, player.Rotation);

        // Frustum culling (simplified - just check if behind camera)
        if (localPos.z <= Z_NEAR) {
            continue;
        }

        faces.push_back({face.blockPos, face.side, face.blockID, localPos.z, face.blockID == 5});
    }

    // Sort faces by distance (far to near for proper alpha blending)
    std::sort(faces.begin(), faces.end(), [](const DrawnFace& a, const DrawnFace& b) {
        if (a.maxZ != b.maxZ) return a.maxZ > b.maxZ;
        if (a.Transparent != b.Transparent) return !a.Transparent && b.Transparent;
        return false;
    });

    // Map transfer buffers
    Vertex* vertexData =
        static_cast<Vertex*>(SDL_MapGPUTransferBuffer(this->GPU, mesh.vertexTransferBuffer, false));
    Uint32* indexData =
        static_cast<Uint32*>(SDL_MapGPUTransferBuffer(this->GPU, mesh.indexTransferBuffer, false));

    if (!vertexData || !indexData) {
        std::cerr << "Failed to map transfer buffers" << std::endl;
        return;
    }

    // FIXME: continue from here
    // Draw faces
    for (const auto& face : faces) {
        if (mesh.faces >= 7000) break;  // Prevent buffer overflow
        drawFace(player, face.blockPos, face.blockID, face.side, mesh, vertexData, indexData);
    }
}
