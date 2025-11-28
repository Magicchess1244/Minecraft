#include "../../include/client/Renderer.hpp"
#include "../../include/client/GameClient.hpp"
#include "../../include/common/Logger.hpp"
#include <SDL3/SDL.h>
#include <cmath>

// Constants
constexpr int RenderDistance = 8;
const float FOV = tan(70.0f * 3.14159f / 360.0f);
constexpr float Znear = 0.1f;
constexpr float Zfar = 1000.0f;
constexpr int vertexSize = 1024 * 1024 * 8; // 8MB
constexpr int indexSize = 1024 * 1024 * 2;  // 2MB

// Voxel face vertices
const Vector3 Verts[6][4] = {
    {{-0.5f, -0.5f, -0.5f},
     {0.5f, -0.5f, -0.5f},
     {-0.5f, 0.5f, -0.5f},
     {0.5f, 0.5f, -0.5f}}, // Front
    {{0.5f, -0.5f, 0.5f},
     {-0.5f, -0.5f, 0.5f},
     {0.5f, 0.5f, 0.5f},
     {-0.5f, 0.5f, 0.5f}}, // Back
    {{-0.5f, -0.5f, 0.5f},
     {-0.5f, -0.5f, -0.5f},
     {-0.5f, 0.5f, 0.5f},
     {-0.5f, 0.5f, -0.5f}}, // Left
    {{0.5f, -0.5f, -0.5f},
     {0.5f, -0.5f, 0.5f},
     {0.5f, 0.5f, -0.5f},
     {0.5f, 0.5f, 0.5f}}, // Right
    {{-0.5f, 0.5f, -0.5f},
     {0.5f, 0.5f, -0.5f},
     {-0.5f, 0.5f, 0.5f},
     {0.5f, 0.5f, 0.5f}}, // Top
    {{-0.5f, -0.5f, 0.5f},
     {0.5f, -0.5f, 0.5f},
     {-0.5f, -0.5f, -0.5f},
     {0.5f, -0.5f, -0.5f}} // Bottom
};

// Face shading colors
const Color Colors[3] = {
    {30, 30, 30}, // X faces
    {20, 20, 20}, // Y faces
    {10, 10, 10}  // Z faces
};

// Helper Functions
Matrix Perspective(float tanHalfFov, float aspect, float Near, float Far) {
  const float f = 1.0f / tanHalfFov;
  Matrix m(4, 4, 0.0f);

  m(0, 0) = f / aspect;
  m(1, 1) = f;
  m(2, 2) = (Far + Near) / (Near - Far);
  m(2, 3) = (2.0f * Far * Near) / (Near - Far);
  m(3, 2) = -1.0f;
  m(3, 3) = 0.0f;

  return m;
}

Matrix RotationX(float angleRad) {
  Matrix m = Matrix::Identity(4);
  float c = std::cos(angleRad);
  float s = std::sin(angleRad);
  m(1, 1) = c;
  m(1, 2) = -s;
  m(2, 1) = s;
  m(2, 2) = c;
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

Matrix RotationZ(float angleRad) {
  Matrix m = Matrix::Identity(4);
  float c = std::cos(angleRad);
  float s = std::sin(angleRad);
  m(0, 0) = c;
  m(0, 1) = -s;
  m(1, 0) = s;
  m(1, 1) = c;
  return m;
}

Matrix Rotation(float x, float y, float z) {
  return RotationY(y) * RotationX(x) * RotationZ(z);
}

Matrix Translation(float x, float y, float z) {
  Matrix m = Matrix::Identity(4);
  m(0, 3) = x;
  m(1, 3) = y;
  m(2, 3) = z;
  return m;
}

Matrix Scale(float x, float y, float z) {
  Matrix m = Matrix::Identity(4);
  m(0, 0) = x;
  m(1, 1) = y;
  m(2, 2) = z;
  return m;
}

Matrix LookAt(const Vector3 &Rotation, const Vector3 &Position) {
  Matrix rotation = RotationY(Rotation.y) * RotationX(Rotation.x);
  Matrix viewRotation = Matrix::Identity(4);
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      viewRotation(i, j) = rotation(j, i);
    }
  }
  Matrix translation = Translation(-Position.x, -Position.y, -Position.z);
  return viewRotation * translation;
}

// Renderer Implementation

Renderer::Renderer(GameClient &gameClient)
    : gameClient(gameClient), chunkManager(false) { // Disable auto-generate
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    LOG_ERROR(std::string("SDL_Init failed: ") + SDL_GetError());
    return;
  }
  LOG_INFO("SDL initialized successfully");

  if (!TTF_Init()) {
    LOG_ERROR(std::string("TTF_Init failed: ") + SDL_GetError());
    return;
  }
  LOG_INFO("SDL_TTF initialized successfully");

  LOG_DEBUG("Loading font from assets/fonts/LiberationMono-BoldItalic.ttf");
  this->font = TTF_OpenFont("assets/fonts/LiberationMono-BoldItalic.ttf", 24);
  if (!this->font) {
    LOG_ERROR(std::string("Failed to load font: ") + SDL_GetError());
  } else {
    LOG_INFO("Font loaded successfully");
  }

  this->GPU = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
  if (!this->GPU) {
    LOG_ERROR(std::string("SDL_CreateGPUDevice failed: ") + SDL_GetError());
    return;
  }
  LOG_INFO("GPU device created successfully");

  this->window =
      SDL_CreateWindow("Minecraft 3D", 1280, 720, SDL_WINDOW_RESIZABLE);
  if (!this->window) {
    LOG_ERROR(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    return;
  }
  LOG_INFO("Window created successfully");

  if (!SDL_ClaimWindowForGPUDevice(this->GPU, this->window)) {
    LOG_ERROR(std::string("SDL_ClaimWindowForGPUDevice failed: ") +
              SDL_GetError());
    return;
  }
  LOG_DEBUG("Window claimed for GPU device");

  // Load Shaders
  SDL_GPUShader *vertex_shader = nullptr;
  SDL_GPUShader *fragment_shader = nullptr;

  // Load SPIR-V shaders
  size_t codeSize;
  void *code = SDL_LoadFile("assets/shaders/Shader.vert.spv", &codeSize);
  if (!code) {
    LOG_ERROR("Failed to load vertex shader spv");
    return;
  }
  LOG_DEBUG("Loaded vertex shader");
  SDL_GPUShaderCreateInfo shaderInfo;
  shaderInfo.code_size = codeSize;
  shaderInfo.code = (const Uint8 *)code;
  shaderInfo.entrypoint = "main";
  shaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
  shaderInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
  shaderInfo.num_samplers = 0;
  shaderInfo.num_storage_textures = 0;
  shaderInfo.num_storage_buffers = 0;
  shaderInfo.num_uniform_buffers = 1;
  vertex_shader = SDL_CreateGPUShader(this->GPU, &shaderInfo);
  SDL_free(code);

  code = SDL_LoadFile("assets/shaders/Shader.frag.spv", &codeSize);
  if (!code) {
    LOG_ERROR("Failed to load fragment shader spv");
    return;
  }
  LOG_DEBUG("Loaded fragment shader");
  shaderInfo.code_size = codeSize;
  shaderInfo.code = (const Uint8 *)code;
  shaderInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
  shaderInfo.num_samplers = 1;
  fragment_shader = SDL_CreateGPUShader(this->GPU, &shaderInfo);
  SDL_free(code);

  if (!vertex_shader || !fragment_shader) {
    LOG_ERROR("Failed to create shaders");
    return;
  }
  LOG_INFO("Shaders created successfully");

  // Create Graphics Pipeline
  SDL_GPUColorTargetDescription colorTargetDesc = {};
  colorTargetDesc.format =
      SDL_GetGPUSwapchainTextureFormat(this->GPU, this->window);
  colorTargetDesc.blend_state.enable_blend = true;
  colorTargetDesc.blend_state.src_color_blendfactor =
      SDL_GPU_BLENDFACTOR_SRC_ALPHA;
  colorTargetDesc.blend_state.dst_color_blendfactor =
      SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  colorTargetDesc.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
  colorTargetDesc.blend_state.src_alpha_blendfactor =
      SDL_GPU_BLENDFACTOR_SRC_ALPHA;
  colorTargetDesc.blend_state.dst_alpha_blendfactor =
      SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  colorTargetDesc.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

  SDL_GPUVertexBufferDescription vertexBufferDesc = {};
  vertexBufferDesc.slot = 0;
  vertexBufferDesc.pitch = sizeof(Vertex);
  vertexBufferDesc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

  SDL_GPUVertexAttribute vertexAttributes[3] = {};
  vertexAttributes[0].location = 0;
  vertexAttributes[0].buffer_slot = 0;
  vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
  vertexAttributes[0].offset = 0;

  vertexAttributes[1].location = 1;
  vertexAttributes[1].buffer_slot = 0;
  vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
  vertexAttributes[1].offset = sizeof(Vector3);

  vertexAttributes[2].location = 2;
  vertexAttributes[2].buffer_slot = 0;
  vertexAttributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
  vertexAttributes[2].offset = sizeof(Vector3) * 2;

  SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.vertex_shader = vertex_shader;
  pipelineInfo.fragment_shader = fragment_shader;

  pipelineInfo.target_info.num_color_targets = 1;
  pipelineInfo.target_info.color_target_descriptions = &colorTargetDesc;
  pipelineInfo.target_info.has_depth_stencil_target = true;
  pipelineInfo.target_info.depth_stencil_format =
      SDL_GPU_TEXTUREFORMAT_D16_UNORM;

  pipelineInfo.depth_stencil_state.enable_depth_test = true;
  pipelineInfo.depth_stencil_state.enable_depth_write = true;
  pipelineInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

  pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
  pipelineInfo.vertex_input_state.vertex_buffer_descriptions =
      &vertexBufferDesc;
  pipelineInfo.vertex_input_state.num_vertex_attributes = 3;
  pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

  this->graphicsPipeline =
      SDL_CreateGPUGraphicsPipeline(this->GPU, &pipelineInfo);
  if (!this->graphicsPipeline) {
    LOG_ERROR(std::string("Failed to create graphics pipeline: ") +
              SDL_GetError());
    return;
  }
  LOG_INFO("Graphics pipeline created successfully");

  // UI Pipeline (No Depth Test)
  pipelineInfo.depth_stencil_state.enable_depth_test = false;
  pipelineInfo.depth_stencil_state.enable_depth_write = false;
  this->uiPipeline = SDL_CreateGPUGraphicsPipeline(this->GPU, &pipelineInfo);

  // Load Textures
  LOG_DEBUG("Loading texture atlas from assets/images/atlas.bmp");
  this->AtlasTexture = CreateTextureFromBMP("assets/images/atlas.bmp");
  if (this->AtlasTexture) {
    LOG_INFO("Atlas texture loaded successfully");
  }
  LOG_DEBUG("Creating Texture Sampler...");
  SDL_GPUSamplerCreateInfo samplerInfo = {};
  samplerInfo.min_filter = SDL_GPU_FILTER_NEAREST;
  samplerInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
  samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
  samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
  samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
  samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

  this->TextureSampler = SDL_CreateGPUSampler(this->GPU, &samplerInfo);
  if (!this->TextureSampler) {
    LOG_ERROR(std::string("Failed to create GPU sampler: ") + SDL_GetError());
    return;
  }
  LOG_DEBUG("Waiting for GPU after sampler creation...");
  SDL_WaitForGPUIdle(this->GPU);
  LOG_DEBUG("Texture sampler created successfully");

  // Create White Texture for UI
  LOG_DEBUG("Creating White Texture...");
  SDL_Surface *whiteSurf = SDL_CreateSurface(1, 1, SDL_PIXELFORMAT_RGBA8888);
  SDL_FillSurfaceRect(whiteSurf, NULL,
                      SDL_MapRGBA(SDL_GetPixelFormatDetails(whiteSurf->format),
                                  NULL, 255, 255, 255, 255));

  LOG_DEBUG("Acquiring Command Buffer for White Texture...");
  this->cmdCopy = SDL_AcquireGPUCommandBuffer(this->GPU);
  LOG_DEBUG("Beginning Copy Pass...");
  this->copyPass = SDL_BeginGPUCopyPass(this->cmdCopy);
  LOG_DEBUG("Creating Texture from Surface...");
  this->whiteTexture = CreateTextureFromSurface(this->copyPass, whiteSurf);
  LOG_DEBUG("Ending Copy Pass...");
  SDL_EndGPUCopyPass(this->copyPass);
  LOG_DEBUG("Submitting Command Buffer...");
  SDL_SubmitGPUCommandBuffer(this->cmdCopy);
  LOG_DEBUG("Waiting for GPU to finish...");
  SDL_WaitForGPUIdle(this->GPU);
  LOG_DEBUG("Destroying Surface...");
  SDL_DestroySurface(whiteSurf);
  LOG_INFO("White Texture created successfully");

  // Initialize Meshes
  int numChunks = (RenderDistance * 2 + 1) * (RenderDistance * 2 + 1);
  this->Terrain.resize(numChunks);
  // Lazy allocation: Buffers are created in RenderChunk when needed

  // UI Mesh
  LOG_DEBUG("Creating UI mesh buffers...");
  SDL_GPUTransferBufferCreateInfo transferInfo = {};
  transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  transferInfo.size = vertexSize;
  uiMesh.VertextransferBuffer =
      SDL_CreateGPUTransferBuffer(this->GPU, &transferInfo);
  if (!uiMesh.VertextransferBuffer) {
    LOG_ERROR(std::string("Failed to create UI vertex transfer buffer: ") +
              SDL_GetError());
    return;
  }

  transferInfo.size = indexSize;
  uiMesh.IndextransferBuffer =
      SDL_CreateGPUTransferBuffer(this->GPU, &transferInfo);
  if (!uiMesh.IndextransferBuffer) {
    LOG_ERROR(std::string("Failed to create UI index transfer buffer: ") +
              SDL_GetError());
    return;
  }

  SDL_GPUBufferCreateInfo bufferInfo = {};
  bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  bufferInfo.size = vertexSize;
  uiMesh.VertexBuffer.buffer = SDL_CreateGPUBuffer(this->GPU, &bufferInfo);
  if (!uiMesh.VertexBuffer.buffer) {
    LOG_ERROR(std::string("Failed to create UI vertex buffer: ") +
              SDL_GetError());
    return;
  }

  bufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
  bufferInfo.size = indexSize;
  uiMesh.IndexBuffer.buffer = SDL_CreateGPUBuffer(this->GPU, &bufferInfo);
  if (!uiMesh.IndexBuffer.buffer) {
    LOG_ERROR(std::string("Failed to create UI index buffer: ") +
              SDL_GetError());
    return;
  }

  LOG_DEBUG("Releasing shaders...");
  SDL_ReleaseGPUShader(this->GPU, vertex_shader);
  SDL_ReleaseGPUShader(this->GPU, fragment_shader);

  LOG_DEBUG("Syncing GPU after buffer creation...");
  SDL_WaitForGPUIdle(this->GPU);
  LOG_INFO("UI mesh buffers created successfully");

  UpdateViewportAndProjection();
  LOG_INFO("Renderer initialization complete");
}

SDL_GPUTexture *Renderer::CreateTextureFromBMP(const char *filename) {
  LOG_DEBUG(std::string("Loading BMP texture: ") + filename);
  SDL_Surface *surface = SDL_LoadBMP(filename);
  if (!surface) {
    LOG_ERROR(std::string("Failed to load BMP: ") + filename);
    return nullptr;
  }
  LOG_DEBUG(std::string("BMP loaded: ") + std::to_string(surface->w) + "x" +
            std::to_string(surface->h));

  LOG_DEBUG("Acquiring command buffer for texture upload...");
  this->cmdCopy = SDL_AcquireGPUCommandBuffer(this->GPU);
  LOG_DEBUG("Beginning copy pass...");
  this->copyPass = SDL_BeginGPUCopyPass(this->cmdCopy);
  LOG_DEBUG("Creating GPU texture from surface...");
  SDL_GPUTexture *texture = CreateTextureFromSurface(this->copyPass, surface);
  LOG_DEBUG("Ending copy pass...");
  SDL_EndGPUCopyPass(this->copyPass);
  LOG_DEBUG("Submitting command buffer...");
  SDL_SubmitGPUCommandBuffer(this->cmdCopy);
  LOG_DEBUG("Waiting for GPU to finish...");
  SDL_WaitForGPUIdle(this->GPU);
  LOG_DEBUG("Destroying surface...");
  SDL_DestroySurface(surface);
  LOG_DEBUG(std::string("Texture created successfully: ") + filename);
  return texture;
}

SDL_GPUTexture *Renderer::CreateTextureFromSurface(SDL_GPUCopyPass *copyPass,
                                                   SDL_Surface *surface) {
  if (!surface) {
    LOG_ERROR("CreateTextureFromSurface: surface is null");
    return nullptr;
  }

  LOG_DEBUG(std::string("Creating GPU texture (") + std::to_string(surface->w) +
            "x" + std::to_string(surface->h) + ")...");

  SDL_GPUTextureCreateInfo createInfo = {};
  createInfo.type = SDL_GPU_TEXTURETYPE_2D;
  createInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
  createInfo.width = surface->w;
  createInfo.height = surface->h;
  createInfo.layer_count_or_depth = 1;
  createInfo.num_levels = 1;
  createInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

  SDL_GPUTexture *texture = SDL_CreateGPUTexture(this->GPU, &createInfo);
  if (!texture) {
    LOG_ERROR(std::string("Failed to create GPU texture: ") + SDL_GetError());
    return nullptr;
  }

  LOG_DEBUG("Creating transfer buffer...");
  SDL_GPUTransferBufferCreateInfo transferInfo = {};
  transferInfo.size = surface->w * surface->h * 4;
  transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  SDL_GPUTransferBuffer *transferBuffer =
      SDL_CreateGPUTransferBuffer(this->GPU, &transferInfo);

  LOG_DEBUG("Mapping transfer buffer...");
  Uint8 *data =
      (Uint8 *)SDL_MapGPUTransferBuffer(this->GPU, transferBuffer, false);
  LOG_DEBUG("Converting surface to ABGR8888...");
  SDL_Surface *converted =
      SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ABGR8888);
  if (converted) {
    memcpy(data, converted->pixels, converted->w * converted->h * 4);
    SDL_DestroySurface(converted);
  } else {
    LOG_WARN("Surface conversion failed, using raw pixels");
    memcpy(data, surface->pixels, surface->pitch * surface->h);
  }
  SDL_UnmapGPUTransferBuffer(this->GPU, transferBuffer);

  LOG_DEBUG("Uploading to GPU texture...");
  SDL_GPUTextureTransferInfo transferLocation = {};
  transferLocation.transfer_buffer = transferBuffer;
  transferLocation.pixels_per_row = surface->w;
  transferLocation.rows_per_layer = surface->h;

  SDL_GPUTextureRegion textureRegion = {};
  textureRegion.texture = texture;
  textureRegion.w = surface->w;
  textureRegion.h = surface->h;
  textureRegion.d = 1;

  SDL_UploadToGPUTexture(copyPass, &transferLocation, &textureRegion, false);
  SDL_ReleaseGPUTransferBuffer(this->GPU, transferBuffer);

  LOG_DEBUG("GPU texture created");
  return texture;
}

SDL_GPUTexture *Renderer::CreateDepthTexture(Uint32 w, Uint32 h) {
  SDL_GPUTextureCreateInfo createInfo = {};
  createInfo.type = SDL_GPU_TEXTURETYPE_2D;
  createInfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
  createInfo.width = w;
  createInfo.height = h;
  createInfo.layer_count_or_depth = 1;
  createInfo.num_levels = 1;
  createInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
  return SDL_CreateGPUTexture(this->GPU, &createInfo);
}

void Renderer::UpdateViewportAndProjection() {
  int w, h;
  SDL_GetWindowSizeInPixels(this->window, &w, &h);
  this->Width = w;
  this->Height = h;

  if (this->DepthTexture)
    SDL_ReleaseGPUTexture(this->GPU, this->DepthTexture);
  this->DepthTexture = CreateDepthTexture(w, h);

  float aspect = (float)w / (float)h;
  this->frustum = Frustum().createFrustumFromCamera(aspect, FOV, Znear, Zfar);
}

// Reuse existing logic for DrawRect, UploadUI, DrawUIPass, getUV, rotate,
// DrawFace
// ... (I will include them here to ensure they are present)

SDL_FPoint Renderer::getUV(int tileIndex, int cornerX, int cornerY) {
  const int tileSize = 16;
  const int atlasSize = 64;
  const float pixelNudge = 0.25f;
  int tilesPerRow = atlasSize / tileSize;
  int tileX = tileIndex % tilesPerRow;
  int tileY = tileIndex / tilesPerRow;
  float u =
      (tileX * tileSize + cornerX * (tileSize - pixelNudge * 2) + pixelNudge) /
      (float)atlasSize;
  float v =
      (tileY * tileSize + cornerY * (tileSize - pixelNudge * 2) + pixelNudge) /
      (float)atlasSize;
  return {u, v};
}

Vector3 Renderer::rotate(const Vector3 &pos, const Vector3 &Angle) {
  Vector3 angleRadians = Angle;
  float cx = cos(angleRadians.x), sx = sin(angleRadians.x);
  float cy = cos(angleRadians.y), sy = sin(angleRadians.y);
  float cz = cos(angleRadians.z), sz = sin(angleRadians.z);
  Vector3 result;
  result.x = pos.x * (cy * cz) + pos.y * (-cy * sz) + pos.z * (sy);
  result.y = pos.x * (sx * sy * cz + cx * sz) +
             pos.y * (-sx * sy * sz + cx * cz) + pos.z * (-sx * cy);
  result.z = pos.x * (-cx * sy * cz + sx * sz) +
             pos.y * (cx * sy * sz + sx * cz) + pos.z * (cx * cy);
  return result;
}

void Renderer::DrawRect(float x, float y, float w, float h, Color color,
                        Vertex *vertices, Uint32 *indices, int &vCount,
                        int &iCount) {
  vertices[vCount + 0].Position = {x, y, 0};
  vertices[vCount + 0].Color = color.ToFloat();
  vertices[vCount + 0].UV = {0, 0};
  vertices[vCount + 1].Position = {x + w, y, 0};
  vertices[vCount + 1].Color = color.ToFloat();
  vertices[vCount + 1].UV = {1, 0};
  vertices[vCount + 2].Position = {x, y + h, 0};
  vertices[vCount + 2].Color = color.ToFloat();
  vertices[vCount + 2].UV = {0, 1};
  vertices[vCount + 3].Position = {x + w, y + h, 0};
  vertices[vCount + 3].Color = color.ToFloat();
  vertices[vCount + 3].UV = {1, 1};
  indices[iCount + 0] = vCount + 0;
  indices[iCount + 1] = vCount + 1;
  indices[iCount + 2] = vCount + 2;
  indices[iCount + 3] = vCount + 2;
  indices[iCount + 4] = vCount + 1;
  indices[iCount + 5] = vCount + 3;
  vCount += 4;
  iCount += 6;
}

void Renderer::UploadUI(SDL_GPUCopyPass *copyPass, Player &player,
                        int inventorySlot, float fps) {
  std::string fpsText = "FPS: " + std::to_string((int)fps);
  std::string coordsText = "X: " + std::to_string((int)player.Position.x) +
                           " Y: " + std::to_string((int)player.Position.y) +
                           " Z: " + std::to_string((int)player.Position.z);
  SDL_Color white = {255, 255, 255, 255};
  SDL_Surface *fpsSurf =
      TTF_RenderText_Blended(font, fpsText.c_str(), fpsText.length(), white);
  SDL_Surface *coordsSurf = TTF_RenderText_Blended(font, coordsText.c_str(),
                                                   coordsText.length(), white);

  if (fpsTexture)
    SDL_ReleaseGPUTexture(this->GPU, fpsTexture);
  if (coordsTexture)
    SDL_ReleaseGPUTexture(this->GPU, coordsTexture);

  fpsTexture = CreateTextureFromSurface(copyPass, fpsSurf);
  coordsTexture = CreateTextureFromSurface(copyPass, coordsSurf);

  Vertex *vertices = (Vertex *)SDL_MapGPUTransferBuffer(
      this->GPU, uiMesh.VertextransferBuffer, false);
  Uint32 *indices = (Uint32 *)SDL_MapGPUTransferBuffer(
      this->GPU, uiMesh.IndextransferBuffer, false);

  int vCount = 0;
  int iCount = 0;

  float hotbarW = 400;
  float hotbarH = 50;
  float startX = (this->Width - hotbarW) / 2;
  float startY = this->Height - hotbarH - 10;
  float slotSize = hotbarW / 9;

  hotbarIndexStart = iCount;
  for (int i = 0; i < 9; ++i) {
    Color c =
        (i == inventorySlot) ? Color{200, 200, 200} : Color{100, 100, 100};
    DrawRect(startX + i * slotSize, startY, slotSize - 2, hotbarH, c, vertices,
             indices, vCount, iCount);
  }
  hotbarIndexCount = iCount - hotbarIndexStart;

  fpsIndexStart = iCount;
  if (fpsTexture && fpsSurf)
    DrawRect(10, 10, fpsSurf->w, fpsSurf->h, {255, 255, 255}, vertices, indices,
             vCount, iCount);
  fpsIndexCount = iCount - fpsIndexStart;

  coordsIndexStart = iCount;
  if (coordsTexture && coordsSurf)
    DrawRect(10, 40, coordsSurf->w, coordsSurf->h, {255, 255, 255}, vertices,
             indices, vCount, iCount);
  coordsIndexCount = iCount - coordsIndexStart;

  SDL_UnmapGPUTransferBuffer(this->GPU, uiMesh.VertextransferBuffer);
  SDL_UnmapGPUTransferBuffer(this->GPU, uiMesh.IndextransferBuffer);

  SDL_GPUTransferBufferLocation vLoc = {uiMesh.VertextransferBuffer, 0};
  SDL_GPUBufferRegion vReg = {uiMesh.VertexBuffer.buffer, 0, vertexSize};
  SDL_UploadToGPUBuffer(copyPass, &vLoc, &vReg, true);

  SDL_GPUTransferBufferLocation iLoc = {uiMesh.IndextransferBuffer, 0};
  SDL_GPUBufferRegion iReg = {uiMesh.IndexBuffer.buffer, 0, indexSize};
  SDL_UploadToGPUBuffer(copyPass, &iLoc, &iReg, true);

  if (fpsSurf)
    SDL_DestroySurface(fpsSurf);
  if (coordsSurf)
    SDL_DestroySurface(coordsSurf);
}

void Renderer::DrawUIPass(SDL_GPURenderPass *pass) {
  SDL_BindGPUGraphicsPipeline(pass, this->uiPipeline);
  Matrix ortho = Matrix::Identity(4);
  ortho(0, 0) = 2.0f / this->Width;
  ortho(1, 1) = -2.0f / this->Height;
  ortho(0, 3) = -1.0f;
  ortho(1, 3) = 1.0f;
  SDL_PushGPUVertexUniformData(this->cmdRender, 0,
                               ortho.getColumnMajorData().data(),
                               sizeof(float) * 16);
  SDL_BindGPUVertexBuffers(pass, 0, &uiMesh.VertexBuffer, 1);
  SDL_BindGPUIndexBuffer(pass, &uiMesh.IndexBuffer,
                         SDL_GPU_INDEXELEMENTSIZE_32BIT);

  if (this->whiteTexture) {
    SDL_GPUTextureSamplerBinding sb = {this->whiteTexture,
                                       this->TextureSampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &sb, 1);
    SDL_DrawGPUIndexedPrimitives(pass, hotbarIndexCount, 1, hotbarIndexStart, 0,
                                 0);
  }
  if (fpsTexture) {
    SDL_GPUTextureSamplerBinding sb = {fpsTexture, this->TextureSampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &sb, 1);
    SDL_DrawGPUIndexedPrimitives(pass, fpsIndexCount, 1, fpsIndexStart, 0, 0);
  }
  if (coordsTexture) {
    SDL_GPUTextureSamplerBinding sb = {coordsTexture, this->TextureSampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &sb, 1);
    SDL_DrawGPUIndexedPrimitives(pass, coordsIndexCount, 1, coordsIndexStart, 0,
                                 0);
  }
}

void Renderer::DrawFace(Player &player, Vector3 blocks, int blockID, int Side,
                        Mesh *mesh, Vertex *Vertexdata, Uint32 *Indexdata) {
  const int baseVertex = mesh->faces * 4;
  const int baseIndex = mesh->faces * 6;

  // Bounds check
  if ((baseVertex + 4) * sizeof(Vertex) > vertexSize ||
      (baseIndex + 6) * sizeof(Uint32) > indexSize) {
    // LOG_WARN("Chunk mesh buffer overflow!");
    return;
  }

  const Vector3 *verts = Verts[Side];
  int tileIndex = 0;
  if (blockID == 1) { // Grass
    if (Side == 4)
      tileIndex = 0; // Top
    else if (Side == 5)
      tileIndex = 2; // Bottom
    else
      tileIndex = 1; // Side
  } else if (blockID == 2)
    tileIndex = 2; // Dirt
  else if (blockID == 3)
    tileIndex = 3; // Stone
  else if (blockID == 4)
    tileIndex = 4; // Wood
  else
    tileIndex = blockID - 1;

  int corners[4][2] = {{0, 1}, {1, 1}, {0, 0}, {1, 0}};
  for (int i = 0; i < 4; i++) {
    Vector3 worldPos = (verts[i] + blocks);
    Vector3 Color =
        BlockDef[blockID].color.ToFloat() - Colors[(int)(Side / 2)].ToFloat();
    SDL_FPoint uv = getUV(tileIndex, corners[i][0], corners[i][1]);
    Vertexdata[baseVertex + i] = {worldPos, Color, {uv.x, uv.y}, {0, 0}};
  }
  Indexdata[baseIndex] = baseVertex + 0;
  Indexdata[baseIndex + 1] = baseVertex + 1;
  Indexdata[baseIndex + 2] = baseVertex + 2;
  Indexdata[baseIndex + 3] = baseVertex + 2;
  Indexdata[baseIndex + 4] = baseVertex + 1;
  Indexdata[baseIndex + 5] = baseVertex + 3;
  mesh->faces++;
}

void Renderer::InitMeshBuffers(Mesh &mesh) {
  if (mesh.VertexBuffer.buffer != nullptr)
    return;

  SDL_GPUTransferBufferCreateInfo transferInfo = {};
  transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  transferInfo.size = vertexSize;
  mesh.VertextransferBuffer =
      SDL_CreateGPUTransferBuffer(this->GPU, &transferInfo);

  transferInfo.size = indexSize;
  mesh.IndextransferBuffer =
      SDL_CreateGPUTransferBuffer(this->GPU, &transferInfo);

  SDL_GPUBufferCreateInfo bufferInfo = {};
  bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  bufferInfo.size = vertexSize;
  mesh.VertexBuffer.buffer = SDL_CreateGPUBuffer(this->GPU, &bufferInfo);

  bufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
  bufferInfo.size = indexSize;
  mesh.IndexBuffer.buffer = SDL_CreateGPUBuffer(this->GPU, &bufferInfo);
}

void Renderer::RenderChunk(ChunkPrefab &chunk, Player &player, int NumChunk) {
  auto *mesh = &this->Terrain[NumChunk];

  if (mesh->x != chunk.xPos || mesh->z != chunk.zPos) {
    mesh->dirty = true;
    mesh->x = chunk.xPos;
    mesh->z = chunk.zPos;
  }

  if (!mesh->dirty)
    return;

  // LOG_DEBUG("Rendering Chunk: " + std::to_string(chunk.xPos) + ", " +
  // std::to_string(chunk.zPos));

  InitMeshBuffers(*mesh);

  mesh->faces = 0;
  std::vector<DrawnFace> Faces;
  for (int i = 0; i < chunk.allFaces.size(); i++) {
    auto *face = &chunk.allFaces[i];
    Faces.push_back(
        {face->blockPos, face->side, face->blockID, 1, face->blockID == 5});
  }

  Vertex *Vertexdata = (Vertex *)SDL_MapGPUTransferBuffer(
      this->GPU, mesh->VertextransferBuffer, false);
  Uint32 *Indexdata = (Uint32 *)SDL_MapGPUTransferBuffer(
      this->GPU, mesh->IndextransferBuffer, false);

  for (auto &Face : Faces) {
    DrawFace(player, Face.blockPos, Face.blockID, Face.side, mesh, Vertexdata,
             Indexdata);
  }

  SDL_UnmapGPUTransferBuffer(this->GPU, mesh->VertextransferBuffer);
  SDL_GPUTransferBufferLocation vLoc = {mesh->VertextransferBuffer, 0};
  SDL_GPUBufferRegion vReg = {mesh->VertexBuffer.buffer, 0, vertexSize};
  SDL_UploadToGPUBuffer(this->copyPass, &vLoc, &vReg, true);

  SDL_UnmapGPUTransferBuffer(this->GPU, mesh->IndextransferBuffer);
  SDL_GPUTransferBufferLocation iLoc = {mesh->IndextransferBuffer, 0};
  SDL_GPUBufferRegion iReg = {mesh->IndexBuffer.buffer, 0, indexSize};
  SDL_UploadToGPUBuffer(this->copyPass, &iLoc, &iReg, true);

  mesh->dirty = false;
}

void Renderer::DrawTerrain(Player &player) {
  int chunkIndex = 0;
  Vector3 playerChunk = {(float)floor(player.Position.x / 32), 0,
                         (float)floor(player.Position.z / 32)};

  // LOG_DEBUG("Drawing Terrain around: " + std::to_string(playerChunk.x) + ", "
  // + std::to_string(playerChunk.z));

  for (int i = -RenderDistance; i <= RenderDistance; i++) {
    for (int j = -RenderDistance; j <= RenderDistance; j++) {
      if (chunkIndex >= this->Terrain.size())
        break;

      int chunkX = (int)playerChunk.x + i;
      int chunkZ = (int)playerChunk.z + j;
      Vector3 ChunkPos = {(float)chunkX, 0, (float)chunkZ};

      ChunkPrefab &chunk = chunkManager.get_chunk(ChunkPos);

      Vector3 min = {(float)chunk.xPos, 0, (float)chunk.zPos};
      Vector3 max = {(float)chunk.xPos + 32, 64, (float)chunk.zPos + 32};

      if (this->frustum.IsBoxVisible(min, max)) {
        if (!chunk.isLoaded) {
          // Request from server if not loaded
          // LOG_DEBUG("Requesting chunk: " + std::to_string(chunkX) + ", " +
          // std::to_string(chunkZ));
          gameClient.RequestChunk(chunkX, chunkZ);
        } else {
          // Render if loaded
          RenderChunk(chunk, player, chunkIndex);

          // Actually draw the mesh if it has faces
          if (this->Terrain[chunkIndex].faces > 0) {
            // Bind and Draw logic should be here or called here?
            // Wait, RenderChunk just updates buffers. Where is the Draw call?
            // Ah, I see DrawTerrain just calls RenderChunk which updates
            // buffers. The actual SDL_DrawGPUIndexedPrimitives call seems
            // missing in DrawTerrain! It must be in the MainRenderLoop or
            // DrawTerrain needs to do it. Let's check MainRenderLoop.
          }
        }
      }
      chunkIndex++;
    }
  }
}

void Renderer::MainRenderLoop(std::vector<Slot> &inventory, int inventorySlot,
                              std::vector<Player> &players) {
  while (SDL_PollEvent(&this->event)) {
    if (this->event.type == SDL_EVENT_QUIT)
      this->gameClient.Quit();
    if (this->event.type == SDL_EVENT_WINDOW_RESIZED)
      UpdateViewportAndProjection();
    if (this->event.type == SDL_EVENT_KEY_DOWN &&
        this->event.key.scancode == SDL_SCANCODE_ESCAPE)
      this->gameClient.Quit();
  }

  // Receive Chunks
  gameClient.ReceiveChunks(this->chunkManager);

  this->cmdCopy = SDL_AcquireGPUCommandBuffer(this->GPU);
  this->copyPass = SDL_BeginGPUCopyPass(this->cmdCopy);

  DrawTerrain(players[0]);

  // FPS
  static Uint32 lastTime = 0;
  static int frameCount = 0;
  static float fps = 0;
  frameCount++;
  Uint32 currentTime = SDL_GetTicks();
  if (currentTime - lastTime >= 1000) {
    fps = frameCount;
    frameCount = 0;
    lastTime = currentTime;
  }

  UploadUI(this->copyPass, players[0], inventorySlot, fps);

  SDL_EndGPUCopyPass(this->copyPass);
  SDL_SubmitGPUCommandBuffer(this->cmdCopy);

  this->cmdRender = SDL_AcquireGPUCommandBuffer(this->GPU);
  SDL_GPUTexture *swap_texture;
  SDL_WaitAndAcquireGPUSwapchainTexture(this->cmdRender, this->window,
                                        &swap_texture, &this->Width,
                                        &this->Height);

  if (swap_texture) {
    SDL_GPUColorTargetInfo color_target_info = {};
    color_target_info.texture = swap_texture;
    color_target_info.clear_color = SDL_FColor{0.1f, 0.79f, 1.0f, 1.0f};
    color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    color_target_info.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depth_target_info = {};
    depth_target_info.texture = this->DepthTexture;
    depth_target_info.clear_depth = 1.0f;
    depth_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
    depth_target_info.store_op = SDL_GPU_STOREOP_STORE;

    this->pass = SDL_BeginGPURenderPass(this->cmdRender, &color_target_info, 1,
                                        &depth_target_info);

    // Terrain Pass
    SDL_BindGPUGraphicsPipeline(this->pass, this->graphicsPipeline);

    Player player = players[0];
    Matrix view = LookAt(player.Rotation, player.Position);
    float aspect = (float)this->Width / (float)this->Height;
    Matrix proj = Perspective(FOV, aspect, Znear, Zfar);
    Matrix mvp =
        proj * view; // Model is Identity for chunks (vertices are world space)

    SDL_PushGPUVertexUniformData(this->cmdRender, 0,
                                 mvp.getColumnMajorData().data(),
                                 sizeof(float) * 16);

    SDL_GPUTextureSamplerBinding sb = {this->AtlasTexture,
                                       this->TextureSampler};
    SDL_BindGPUFragmentSamplers(this->pass, 0, &sb, 1);

    for (int i = 0; i < this->Terrain.size(); i++) {
      auto *mesh = &this->Terrain[i];
      if (mesh->faces > 0) {
        SDL_BindGPUVertexBuffers(this->pass, 0, &mesh->VertexBuffer, 1);
        SDL_BindGPUIndexBuffer(this->pass, &mesh->IndexBuffer,
                               SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_DrawGPUIndexedPrimitives(this->pass, mesh->faces * 6, 1, 0, 0, 0);
      }
    }
    SDL_EndGPURenderPass(this->pass);

    // UI Pass
    SDL_GPUColorTargetInfo ui_target_info = color_target_info;
    ui_target_info.load_op = SDL_GPU_LOADOP_LOAD;
    this->pass =
        SDL_BeginGPURenderPass(this->cmdRender, &ui_target_info, 1, NULL);
    DrawUIPass(this->pass);
    SDL_EndGPURenderPass(this->pass);
  }

  SDL_SubmitGPUCommandBuffer(this->cmdRender);
}
