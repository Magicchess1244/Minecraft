#include "../../include/client/Renderer.hpp"
#include "../../include/client/GameClient.hpp"
#include <SDL3/SDL_gpu.h>
#include <iostream>
#include <ostream>

constexpr Uint32 vertexSize = sizeof(Vertex) * 4 * 10000;
constexpr Uint32 indexSize = sizeof(Uint32) * 6 * 10000;
const float FOV = tan((90 * (PI / 180)) / 2.0f);
const float Znear = 0.1f;
constexpr float Zfar = 500.0f;
constexpr int RenderDistance = 1;
const Vector3 Verts[6][4] = {
    {// Front (+Z) - looking at face from outside (positive Z direction)
     // Counter-clockwise: bottom-right, bottom-left, top-right, top-left
     {0.5, -0.5, 0.5},  // 0: bottom-right
     {-0.5, -0.5, 0.5}, // 1: bottom-left
     {0.5, 0.5, 0.5},   // 2: top-right
     {-0.5, 0.5, 0.5}}, // 3: top-left
    {// Back (-Z) - looking at face from outside (negative Z direction)
     // Counter-clockwise: bottom-left, bottom-right, top-left, top-right
     {-0.5, -0.5, -0.5}, // 0: bottom-left
     {0.5, -0.5, -0.5},  // 1: bottom-right
     {-0.5, 0.5, -0.5},  // 2: top-left
     {0.5, 0.5, -0.5}},  // 3: top-right
    {// Right (+X) - looking at face from outside (positive X direction)
     // Counter-clockwise when viewed from +X: front-bottom, back-bottom,
     // front-top, back-top
     {0.5, -0.5, 0.5},  // 0: front-bottom
     {0.5, -0.5, -0.5}, // 1: back-bottom
     {0.5, 0.5, 0.5},   // 2: front-top
     {0.5, 0.5, -0.5}}, // 3: back-top
    {// Left (-X) - looking at face from outside (negative X direction)
     // Counter-clockwise when viewed from -X: back-bottom, front-bottom,
     // back-top, front-top
     {-0.5, -0.5, -0.5}, // 0: back-bottom
     {-0.5, -0.5, 0.5},  // 1: front-bottom
     {-0.5, 0.5, -0.5},  // 2: back-top
     {-0.5, 0.5, 0.5}},  // 3: front-top
    {// Top (+Y) - looking at face from outside (positive Y direction)
     // Counter-clockwise: back-left, back-right, front-left, front-right
     {-0.5, 0.5, -0.5}, // 0: back-left
     {0.5, 0.5, -0.5},  // 1: back-right
     {-0.5, 0.5, 0.5},  // 2: front-left
     {0.5, 0.5, 0.5}},  // 3: front-right
    {// Bottom (-Y) - looking at face from outside (negative Y direction, from
     // below) Counter-clockwise when viewed from below: front-right,
     // back-right, front-left, back-left
     {0.5, -0.5, 0.5},     // 0: front-right
     {0.5, -0.5, -0.5},    // 1: back-right
     {-0.5, -0.5, 0.5},    // 2: front-left
     {-0.5, -0.5, -0.5}}}; // 3: back-left
const Vector3 Direction[6] = {
    {0, 0, -1}, // Front
    {0, 0, 1},  // Back
    {-1, 0, 0}, // Right
    {1, 0, 0},  // Left
    {0, -1, 0}, // Top
    {0, 1, 0}   // Bottom
};
const Color Colors[3] = {
    {0, 0, 0},    // Front / Back
    {5, 5, 5},    // Right / Left
    {10, 10, 10}, // Top / Bottom
};
Matrix Perspective(float tanHalfFov, float aspect, float Near, float Far) {
  Matrix m(4, 4, 0.0f);
  float f = 1 / tanHalfFov;
  m(0, 0) = (f / aspect); // Negated to flip x-axis
  m(1, 1) = f;
  m(2, 2) = (Near + Far) / (Near - Far);
  m(3, 2) = 1.0f;
  m(2, 3) = (2.0f * Near * Far) / (Near - Far);
  m(3, 3) = 0.0f;
  return m;
}
// Rotation matrices look correct
Matrix RotationX(float angleRad) {
  Matrix m = Matrix::Identity(4);
  float c = std::cos(angleRad);
  float s = std::sin(angleRad);
  m(0, 0) = 1;
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
  m(1, 1) = 1;
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

Matrix Rotation(Vector3 Rotation) {
  return RotationZ(Rotation.AngleToRadians().z) *
         RotationY(Rotation.AngleToRadians().y) *
         RotationX(Rotation.AngleToRadians().x);
}
Matrix Translation(float x, float y, float z) {
  Matrix m = Matrix::Identity(4);
  m(0, 3) = x;
  m(1, 3) = y;
  m(2, 3) = z;
  return m;
}
Matrix LookAt(const Vector3 &Rotation, const Vector3 &Position) {
  Matrix rotation = RotationY(Rotation.AngleToRadians().y * 1) *
                    RotationX(Rotation.AngleToRadians().x * 1);
  Matrix viewRotation = Matrix::Identity(4);
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) {
      viewRotation(i, j) = rotation(j, i);
    }
  }
  Matrix translation = Translation(-Position.x, -Position.y, -Position.z);
  return viewRotation * translation;
}
bool isFaceInFrustum(const Frustum &frustum, const Vector3 faceVerts[4]) {
  const Plane planes[6] = {frustum.nearFace,  frustum.farFace,
                           frustum.rightFace, frustum.leftFace,
                           frustum.topFace,   frustum.bottomFace};

  // For each plane, check if all 4 vertices are outside
  for (const auto &plane : planes) {
    int outsideCount = 0;
    for (int i = 0; i < 4; ++i) {
      if (plane.getSignedDistanceToPlane(faceVerts[i]) < 0.0f) {
        ++outsideCount;
      }
    }
    if (outsideCount == 4) {
      return false; // entire face is outside this plane
    }
  }

  return true; // at least partially inside all planes
}
SDL_FPoint Renderer::getUV(int tileIndex, int cornerX, int cornerY) {
  const int tileSize = 16;
  const int atlasSize = 64;
  const float pixelNudge = 0.25f; // or 0.25f if needed

  int tilesPerRow = atlasSize / tileSize;
  int tileX = tileIndex % tilesPerRow;
  int tileY = tileIndex / tilesPerRow;

  float u =
      (tileX * tileSize + cornerX * (tileSize - pixelNudge * 2) + pixelNudge) /
      (float)atlasSize;
  float v =
      (tileY * tileSize + cornerY * (tileSize - pixelNudge * 2) + pixelNudge) /
      (float)atlasSize;

  SDL_FPoint point = {(float)u, (float)v};
  return point;
}
Vector3 Renderer::rotate(const Vector3 &pos, const Vector3 &Angle) {
  Vector3 angleRadians = Angle.AngleToRadians();
  float cx = cos(angleRadians.x), sx = sin(angleRadians.x);
  float cy = cos(angleRadians.y), sy = sin(angleRadians.y);
  float cz = cos(angleRadians.z), sz = sin(angleRadians.z);

  Vector3 result;

  // Row 1
  result.x = pos.x * (cy * cz) + pos.y * (-cy * sz) + pos.z * (sy);

  // Row 2
  result.y = pos.x * (sx * sy * cz + cx * sz) +
             pos.y * (-sx * sy * sz + cx * cz) + pos.z * (-sx * cy);

  // Row 3
  result.z = pos.x * (-cx * sy * cz + sx * sz) +
             pos.y * (cx * sy * sz + sx * cz) + pos.z * (cx * cy);

  return result;
}
void Renderer::DrawFace(Player &player, Vector3 blocks, int blockID, int Side,
                        Mesh *mesh, Vertex *Vertexdata, Uint32 *Indexdata) {
  const Vector3 *verts = Verts[Side];

  for (int i = 0; i < 4; i++) {
    Vector3 worldPos = (verts[i] + blocks);

    Vector3 Color =
        BlockDef[blockID].color.ToFloat() - Colors[(int)(Side / 2)].ToFloat();
    Vertex vertex = {worldPos, Color};
    Vertexdata[mesh->BaseVertex++] = vertex;
  }

  Indexdata[mesh->BaseIndex++] = mesh->BaseVertex - 4 + 0;
  Indexdata[mesh->BaseIndex++] = mesh->BaseVertex - 4 + 2;
  Indexdata[mesh->BaseIndex++] = mesh->BaseVertex - 4 + 1;

  Indexdata[mesh->BaseIndex++] = mesh->BaseVertex - 4 + 1;
  Indexdata[mesh->BaseIndex++] = mesh->BaseVertex - 4 + 2;
  Indexdata[mesh->BaseIndex++] = mesh->BaseVertex - 4 + 3;
}
void Renderer::RenderChunk(ChunkPrefab &chunk, Player &player, int chunkIndex,
                           int bufferIndex, int bufferOffset,
                           const Frustum &worldFrustum) {
  auto *mesh = &this->Terrain[bufferIndex];

  for (auto &face : chunk.allFaces) {
    // Skip transparent faces in this simplified call if we handle them
    // elsewhere But for now, let's just fix the indices in the main loop
    const Vector3 *verts = Verts[face.side];

    // Write vertices
    for (int i = 0; i < 4; i++) {
      Vector3 worldPos = (verts[i] + face.blockPos);
      Vector3 Color = BlockDef[face.blockID].color.ToFloat() -
                      Colors[(int)(face.side / 2)].ToFloat();
      Vertex vertex = {worldPos, Color};
      mesh->mappedVertexData[mesh->BaseVertex + i] = vertex;
    }

    // Write indices (CCW)
    mesh->mappedIndexData[mesh->BaseIndex + 0] = mesh->BaseVertex + 0;
    mesh->mappedIndexData[mesh->BaseIndex + 1] = mesh->BaseVertex + 2;
    mesh->mappedIndexData[mesh->BaseIndex + 2] = mesh->BaseVertex + 1;

    mesh->mappedIndexData[mesh->BaseIndex + 3] = mesh->BaseVertex + 1;
    mesh->mappedIndexData[mesh->BaseIndex + 4] = mesh->BaseVertex + 2;
    mesh->mappedIndexData[mesh->BaseIndex + 5] = mesh->BaseVertex + 3;

    mesh->BaseVertex += 4;
    mesh->BaseIndex += 6;
  }
}
void Renderer::DrawTerrain(Player &player) {
  SpiralIterator spiral(RenderDistance * 2 + 1);
  Vector3 PlayerChunk = (player.Position / 16).Truncate();

  for (int bufferIdx = 0; bufferIdx < this->totalBuffers; bufferIdx++) {
    auto *mesh = &this->Terrain[bufferIdx];
    mesh->BaseVertex = 0;
    mesh->BaseIndex = 0;
    mesh->OpaqueIndexCount = 0;
    mesh->TransparentIndexCount = 0;

    Vertex *Vertexdata = (Vertex *)SDL_MapGPUTransferBuffer(
        this->basicInitVars.GPU, mesh->VertextransferBuffer, true);
    Uint32 *Indexdata = (Uint32 *)SDL_MapGPUTransferBuffer(
        this->basicInitVars.GPU, mesh->IndextransferBuffer, true);

    mesh->mappedVertexData = Vertexdata;
    mesh->mappedIndexData = Indexdata;

    int startChunkIdx = bufferIdx * chunksPerBuffer;
    int endChunkIdx =
        std::min(startChunkIdx + chunksPerBuffer,
                 (RenderDistance * 2 + 1) * (RenderDistance * 2 + 1));

    std::vector<ChunkPrefab *> chunks;
    for (int i = startChunkIdx; i < endChunkIdx; i++) {
      std::pair<int, int> Pos = spiral.next();
      Vector3 ChunkPos = {(float)Pos.first, 0, (float)Pos.second};
      ChunkPos += PlayerChunk;
      chunks.push_back(&chunkManager.get_chunk(ChunkPos));
    }

    // First pass: Opaque
    for (auto *chunk : chunks) {
      for (auto &face : chunk->allFaces) {
        if (face.Transparent)
          continue;
        const Vector3 *verts = Verts[face.side];
        for (int i = 0; i < 4; i++) {
          Vertexdata[mesh->BaseVertex + i] = {
              verts[i] + face.blockPos,
              BlockDef[face.blockID].color.ToFloat() -
                  Colors[(int)(face.side / 2)].ToFloat()};
        }
        Indexdata[mesh->BaseIndex + 0] = mesh->BaseVertex + 0;
        Indexdata[mesh->BaseIndex + 1] = mesh->BaseVertex + 2;
        Indexdata[mesh->BaseIndex + 2] = mesh->BaseVertex + 1;
        Indexdata[mesh->BaseIndex + 3] = mesh->BaseVertex + 1;
        Indexdata[mesh->BaseIndex + 4] = mesh->BaseVertex + 2;
        Indexdata[mesh->BaseIndex + 5] = mesh->BaseVertex + 3;
        mesh->BaseVertex += 4;
        mesh->BaseIndex += 6;
      }
    }
    mesh->OpaqueIndexCount = mesh->BaseIndex;

    // Second pass: Transparent
    for (auto *chunk : chunks) {
      for (auto &face : chunk->allFaces) {
        if (!face.Transparent)
          continue;
        const Vector3 *verts = Verts[face.side];
        for (int i = 0; i < 4; i++) {
          Vertexdata[mesh->BaseVertex + i] = {
              verts[i] + face.blockPos,
              BlockDef[face.blockID].color.ToFloat() -
                  Colors[(int)(face.side / 2)].ToFloat()};
        }
        Indexdata[mesh->BaseIndex + 0] = mesh->BaseVertex + 0;
        Indexdata[mesh->BaseIndex + 1] = mesh->BaseVertex + 2;
        Indexdata[mesh->BaseIndex + 2] = mesh->BaseVertex + 1;
        Indexdata[mesh->BaseIndex + 3] = mesh->BaseVertex + 1;
        Indexdata[mesh->BaseIndex + 4] = mesh->BaseVertex + 2;
        Indexdata[mesh->BaseIndex + 5] = mesh->BaseVertex + 3;
        mesh->BaseVertex += 4;
        mesh->BaseIndex += 6;
      }
    }
    mesh->TransparentIndexCount = mesh->BaseIndex - mesh->OpaqueIndexCount;

    SDL_UnmapGPUTransferBuffer(this->basicInitVars.GPU,
                               mesh->VertextransferBuffer);
    SDL_UnmapGPUTransferBuffer(this->basicInitVars.GPU,
                               mesh->IndextransferBuffer);

    SDL_GPUTransferBufferLocation vLoc{mesh->VertextransferBuffer, 0};
    SDL_GPUBufferRegion vReg{mesh->VertexBuffer.buffer, 0,
                             (Uint32)(mesh->BaseVertex * sizeof(Vertex))};
    SDL_UploadToGPUBuffer(this->runTimeRenderVars.copyPass, &vLoc, &vReg, true);

    SDL_GPUTransferBufferLocation iLoc{mesh->IndextransferBuffer, 0};
    SDL_GPUBufferRegion iReg{mesh->IndexBuffer.buffer, 0,
                             (Uint32)(mesh->BaseIndex * sizeof(Uint32))};
    SDL_UploadToGPUBuffer(this->runTimeRenderVars.copyPass, &iLoc, &iReg, true);
  }
}
/*
void Renderer::DrawPlayer(SDL_Renderer* Renderer, Vector3 Range,
std::vector<Player>& PlayerPos)
{
        //Other player
        for (const Player& Position : PlayerPos)
        {
                int dx = (int)(Position.Position.x - PlayerPos[0].Position.x);
                int dy = (int)(Position.Position.y - PlayerPos[0].Position.y);

                bool InsideX = std::abs(dx) <= (Range.x / 2);
                if (InsideX) {
                        int RelativeX = (int)((Range.x / 2 - 1 + dx) *
BlockPixelSize); int RelativeY = (int)((Range.y / 2 - 2 - dy) * BlockPixelSize);

                        SDL_FRect OtherPlayerRect = {
                                (float)RelativeX,
                                (float)RelativeY,
                                (float)BlockPixelSize,
                                (float)BlockPixelSize * 2
                        };
                        SDL_SetRenderDrawColor(Renderer,
(Uint8)SDL_clamp(PlayerPos[0].color.r, 0, 255),
(Uint8)SDL_clamp(PlayerPos[0].color.g, 0, 255),
(Uint8)SDL_clamp(PlayerPos[0].color.b, 0, 255), 255);
                        SDL_RenderFillRect(Renderer, &OtherPlayerRect);
                }
        }

        // Your player
        SDL_SetRenderDrawColor(Renderer, SDL_clamp(PlayerPos[0].color.r, 0,
255), SDL_clamp(PlayerPos[0].color.g, 0, 255), SDL_clamp(PlayerPos[0].color.b,
0, 255), 255); SDL_FRect PlayerRect = { (float)(Range.x / 2 - 1) *
BlockPixelSize, (float)(Range.y / 2 - 2) * this->BlockPixelSize,
                (float)this->BlockPixelSize,
                (float)this->BlockPixelSize * 2
        };
        SDL_RenderFillRect(Renderer, &PlayerRect);

        SDL_SetRenderDrawColor(Renderer, SDL_clamp(PlayerPos[0].color.r + 90, 0,
255), SDL_clamp(PlayerPos[0].color.g + 90, 0, 255),
SDL_clamp(PlayerPos[0].color.b + 90, 0, 255), 255); SDL_FRect InsidePlayerRect =
{ (float)(Range.x / 2 - 1) * this->BlockPixelSize + (this->BlockPixelSize *
0.1f), (float)(Range.y / 2 - 2) * this->BlockPixelSize + (this->BlockPixelSize *
0.1f), (float)(this->BlockPixelSize * 0.8f), (float)(this->BlockPixelSize *
0.9f) * 2
        };
        SDL_RenderFillRect(Renderer, &InsidePlayerRect);

}
*/
/*
void Renderer::Stats(Player& player)
{
        std::string text = std::to_string(player.Position.x) + ", " +
std::to_string(player.Position.y) + ", " + std::to_string(player.Position.z);
        SDL_Color White = { 200, 200, 200 };

        if (!this->font) {
                std::cerr << "Font not loaded!\n";
                return;
        }

        if (text.empty()) return;

        SDL_Surface* surface = TTF_RenderText_Blended(this->font, text.c_str(),
text.length(), White); if (!surface) { std::cerr << "TTF_RenderText_Blended
failed: \n"; return;
        }

        //SDL_Texture* texture = SDL_CreateTextureFromSurface(this->renderer,
surface); if (!texture) { std::cerr << "SDL_CreateTextureFromSurface failed: "
<< SDL_GetError() << "\n"; SDL_DestroySurface(surface); return;
        }

        SDL_FRect dstRect{ 10, 10, (float)surface->w, (float)surface->h };
        //SDL_RenderTexture(this->renderer, texture, NULL, &dstRect);

        //SDL_DestroyTexture(texture);
        //SDL_DestroySurface(surface);
}
*/
void Renderer::UpdateViewportAndProjection() {
  // Get the actual drawable size (important for high-DPI displays)
  int drawableWidth, drawableHeight;
  SDL_GetWindowSizeInPixels(this->basicInitVars.window, &drawableWidth,
                            &drawableHeight);

  this->basicInitVars.Width = drawableWidth;
  this->basicInitVars.Height = drawableHeight;

  std::cout << "Updating viewport to: " << drawableWidth << "x"
            << drawableHeight << std::endl;

  // Recreate depth texture with new size
  if (this->DepthTexture) {
    SDL_ReleaseGPUTexture(this->basicInitVars.GPU, this->DepthTexture);
  }
  this->DepthTexture = CreateDepthTexture(drawableWidth, drawableHeight);

  // Update frustum for new aspect ratio
  float aspect = (float)drawableWidth / (float)drawableHeight;
  this->frustum = Frustum().createFrustumFromCamera(aspect, FOV, Znear, Zfar);
}
void Renderer::EventManager(Player &player) {
  while (SDL_PollEvent(&this->basicInitVars.event)) {
    switch (this->basicInitVars.event.type) {
    case SDL_EVENT_QUIT:
      // Handle quitting the game
      this->gameClient.Quit();
      break;

    case SDL_EVENT_WINDOW_RESIZED: {
      UpdateViewportAndProjection();
      break;
    }
    case SDL_EVENT_KEY_DOWN: {
      SDL_Keycode key = this->basicInitVars.event.key.scancode;

      if (key == SDL_SCANCODE_ESCAPE) {
        this->gameClient.Quit();
        break;
      } else if (key == SDL_SCANCODE_F11) {

        this->fullScreen = !this->fullScreen;
        std::cout << "Toggling fullscreen: "
                  << (this->fullScreen ? "ON" : "OFF") << std::endl;

        SDL_SetWindowFullscreen(this->basicInitVars.window, this->fullScreen);

        // Update screen size immediately
        UpdateViewportAndProjection();
      }
      break;
    }
    }
  }
}
void Renderer::MainRenderLoop(std::vector<Slot> &inventory, int inventorySlot,
                              std::vector<Player> &players) {
  EventManager(players[0]);
  this->runTimeRenderVars.cmdCopy =
      SDL_AcquireGPUCommandBuffer(this->basicInitVars.GPU);

  this->runTimeRenderVars.copyPass =
      SDL_BeginGPUCopyPass(this->runTimeRenderVars.cmdCopy);
  DrawTerrain(players[0]);

  SDL_EndGPUCopyPass(this->runTimeRenderVars.copyPass);
  if (!SDL_SubmitGPUCommandBuffer(this->runTimeRenderVars.cmdCopy)) {
    std::cout << "Heil la memoria de Puigdemont\n";
    return;
  }

  this->runTimeRenderVars.cmdRender =
      SDL_AcquireGPUCommandBuffer(this->basicInitVars.GPU);

  SDL_GPUTexture *swap_texture;
  SDL_WaitAndAcquireGPUSwapchainTexture(
      this->runTimeRenderVars.cmdRender, this->basicInitVars.window,
      &swap_texture, &this->basicInitVars.Width, &this->basicInitVars.Height);

  if (swap_texture == NULL) {
    std::cout << "La swap_texture no s'ha fet be\n";
    SDL_SubmitGPUCommandBuffer(this->runTimeRenderVars.cmdRender);
    return; // CRITICAL: Must return here!
  }
  SDL_GPUColorTargetInfo color_target_info;
  SDL_zero(color_target_info);
  color_target_info.clear_color = SDL_FColor{0.1f, 0.79f, 1.0f, 1.0f};
  color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
  color_target_info.store_op = SDL_GPU_STOREOP_STORE;
  color_target_info.texture = swap_texture;

  // Setup depth buffer for proper depth testing
  SDL_GPUDepthStencilTargetInfo depth_target_info;
  SDL_zero(depth_target_info);
  depth_target_info.texture = this->DepthTexture;
  depth_target_info.clear_depth = 1.0f;
  depth_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
  depth_target_info.store_op = SDL_GPU_STOREOP_DONT_CARE;
  depth_target_info.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
  depth_target_info.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
  depth_target_info.cycle = false;

  Player player = players[0];
  Matrix model = Rotation({0, 0, 0});

  Matrix view = LookAt(player.Rotation, player.Position);

  float aspect =
      (float)this->basicInitVars.Width / (float)this->basicInitVars.Height;
  Matrix proj = Perspective(FOV, aspect, Znear, Zfar);

  // Final MVP matrix
  Matrix mvp = proj * view * model;

  // mvp.print();
  SDL_PushGPUVertexUniformData(this->runTimeRenderVars.cmdRender, 0,
                               mvp.getColumnMajorData().data(),
                               sizeof(float) * mvp.getColumnMajorData().size());

  this->runTimeRenderVars.pass =
      SDL_BeginGPURenderPass(this->runTimeRenderVars.cmdRender,
                             &color_target_info, 1, &depth_target_info);
  SDL_BindGPUGraphicsPipeline(this->runTimeRenderVars.pass,
                              this->pipelineInitVars.graphicsPipeline);

  // Draw the tesseract from first mesh
  for (auto &mesh : this->Terrain) {
    SDL_BindGPUVertexBuffers(this->runTimeRenderVars.pass, 0,
                             &mesh.VertexBuffer, 1);
    SDL_BindGPUIndexBuffer(this->runTimeRenderVars.pass, &mesh.IndexBuffer,
                           SDL_GPU_INDEXELEMENTSIZE_32BIT);

    if (mesh.BaseVertex > 0) {
      SDL_DrawGPUIndexedPrimitives(this->runTimeRenderVars.pass, mesh.BaseIndex,
                                   1, 0, 0, 0);
    }
    mesh.BaseIndex = 0;
    mesh.BaseVertex = 0;
  }

  SDL_EndGPURenderPass(this->runTimeRenderVars.pass);

  if (!SDL_SubmitGPUCommandBuffer(this->runTimeRenderVars.cmdRender)) {
    std::cout << "Failed to submit render command buffer\n";
    return;
  }
}
SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *filename,
                          Uint32 sampler_count, Uint32 uniform_buffer_count,
                          Uint32 storage_buffer_count,
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
  const char *entrypoint;
  const char *basepath = SDL_GetBasePath();

  if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV) {
    SDL_snprintf(fullpath, sizeof(fullpath), "%sassets/shaders/%s.spv",
                 basepath, filename);
    entrypoint = "main";
    format = SDL_GPU_SHADERFORMAT_SPIRV;
  } else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL) {
    SDL_snprintf(fullpath, sizeof(fullpath), "%sshaders/%s.dxil", basepath,
                 filename);
    entrypoint = "main";
    format = SDL_GPU_SHADERFORMAT_DXIL;
  } else if (backend_formats & SDL_GPU_SHADERFORMAT_MSL) {
    SDL_snprintf(fullpath, sizeof(fullpath), "%sshaders/%s.msl", basepath,
                 filename);
    entrypoint = "main0";
    format = SDL_GPU_SHADERFORMAT_MSL;
  } else {
    SDL_Log("No supported shader format found!");
    return NULL;
  }

  size_t code_size;
  void *code = SDL_LoadFile(fullpath, &code_size);
  if (!code) {
    SDL_Log("Couldn't load shader file: %s", SDL_GetError());
    return NULL;
  }

  SDL_GPUShaderCreateInfo shader_info = {
      .code_size = code_size,
      .code = (const unsigned char *)code,
      .entrypoint = entrypoint,
      .format = format,
      .stage = stage,
      .num_samplers = sampler_count,
      .num_storage_textures = storage_texture_count,
      .num_storage_buffers = storage_buffer_count,
      .num_uniform_buffers = uniform_buffer_count,

  };

  SDL_GPUShader *shader = SDL_CreateGPUShader(device, &shader_info);
  if (!shader) {
    SDL_Log("Couldn't create shader: %s", SDL_GetError());
    SDL_free(code);
    return NULL;
  }

  SDL_free(code);
  return shader;
}
SDL_GPUTexture *Renderer::CreateDepthTexture(Uint32 drawablew,
                                             Uint32 drawableh) {
  SDL_GPUTextureCreateInfo createinfo;
  SDL_GPUTexture *result;

  createinfo.type = SDL_GPU_TEXTURETYPE_2D;
  createinfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
  createinfo.width = drawablew;
  createinfo.height = drawableh;
  createinfo.layer_count_or_depth = 1;
  createinfo.num_levels = 1;
  createinfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
  createinfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
  createinfo.props = 0;

  result = SDL_CreateGPUTexture(this->basicInitVars.GPU, &createinfo);
  if (!result) {
    SDL_Log("Failed to create depth texture: %s", SDL_GetError());
    return NULL;
  }

  return result;
}
void Renderer::Init() {
  this->basicInitVars.Width = 600;
  this->basicInitVars.Height = 400;

  this->basicInitVars.window =
      SDL_CreateWindow("Bit Miner", this->basicInitVars.Width,
                       this->basicInitVars.Height, SDL_WINDOW_RESIZABLE);
  if (this->basicInitVars.window == nullptr) {
    std::cout << "Error creating basicInitVars.window: " << SDL_GetError();
    SDL_Quit();
    assert(false);
  }

  this->basicInitVars.GPU =
      SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, NULL);
  if (this->basicInitVars.GPU == nullptr) {
    std::cout << "SDL basicInitVars.GPU creation failed: \n" << SDL_GetError();
  }
  if (!SDL_ClaimWindowForGPUDevice(this->basicInitVars.GPU,
                                   this->basicInitVars.window)) {
    std::cout
        << "Error claiming basicInitVars.window for basicInitVars.GPU device: "
        << SDL_GetError() << std::endl;
  }
  this->basicInitVars.event = {};
  SDL_SetWindowRelativeMouseMode(basicInitVars.window, true);
}
void Renderer::GenerateBuffer() {
  // Calculate buffer packing
  int totalChunks = (RenderDistance * 2 + 1) * (RenderDistance * 2 + 1);
  this->chunksPerBuffer = 3; // Pack 3 chunks per buffer
  this->totalBuffers =
      (totalChunks + chunksPerBuffer - 1) / chunksPerBuffer; // Ceiling division

  // Each buffer now holds multiple chunks
  constexpr Uint32 singleChunkVertexSize = sizeof(Vertex) * 4 * 10000;
  constexpr Uint32 singleChunkIndexSize = sizeof(Uint32) * 6 * 10000;
  const Uint32 packedVertexSize = singleChunkVertexSize * chunksPerBuffer;
  const Uint32 packedIndexSize = singleChunkIndexSize * chunksPerBuffer;

  std::cout << "Vertex size per buffer: " << packedVertexSize
            << "\t Index Size per buffer: " << packedIndexSize
            << "\n Total buffers: " << this->totalBuffers
            << "\t Chunks per buffer: " << this->chunksPerBuffer << std::endl;

  for (int i = 0; i < this->totalBuffers; i++) {
    Mesh mesh{};
    SDL_GPUBufferCreateInfo vertexInfo = {};
    vertexInfo.size = packedVertexSize;
    vertexInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    mesh.VertexBuffer.buffer =
        SDL_CreateGPUBuffer(this->basicInitVars.GPU, &vertexInfo);
    mesh.VertexBuffer.offset = 0;

    SDL_GPUBufferCreateInfo indexInfo = {};
    indexInfo.size = packedIndexSize;
    indexInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    mesh.IndexBuffer.buffer =
        SDL_CreateGPUBuffer(this->basicInitVars.GPU, &indexInfo);
    mesh.IndexBuffer.offset = 0;

    SDL_GPUTransferBufferCreateInfo VertextransferInfo{};
    VertextransferInfo.size = packedVertexSize;
    VertextransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    mesh.VertextransferBuffer = SDL_CreateGPUTransferBuffer(
        this->basicInitVars.GPU, &VertextransferInfo);

    SDL_GPUTransferBufferCreateInfo IndextransferInfo{};
    IndextransferInfo.size = packedIndexSize;
    IndextransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    mesh.IndextransferBuffer = SDL_CreateGPUTransferBuffer(
        this->basicInitVars.GPU, &IndextransferInfo);

    this->Terrain.push_back(mesh);
  }
}
void Renderer::VertexGPUInit() {
  // describe the vertex attribute
  this->pipelineInitVars.vertex_buffer_desc.slot = 0;
  this->pipelineInitVars.vertex_buffer_desc.input_rate =
      SDL_GPU_VERTEXINPUTRATE_VERTEX;
  this->pipelineInitVars.vertex_buffer_desc.instance_step_rate = 0;
  this->pipelineInitVars.vertex_buffer_desc.pitch = sizeof(Vertex);

  this->pipelineInitVars.vertex_attributes[0].buffer_slot = 0;
  this->pipelineInitVars.vertex_attributes[0].location = 0;
  this->pipelineInitVars.vertex_attributes[0].format =
      SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
  this->pipelineInitVars.vertex_attributes[0].offset = 0;

  this->pipelineInitVars.vertex_attributes[1].buffer_slot = 0;
  this->pipelineInitVars.vertex_attributes[1].location = 1;
  this->pipelineInitVars.vertex_attributes[1].format =
      SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
  this->pipelineInitVars.vertex_attributes[1].offset = sizeof(float) * 3;
}
void Renderer::ColorTargetDes() {
  this->pipelineInitVars.colorTargetDescriptions[0] = {};
  this->pipelineInitVars.colorTargetDescriptions[0].blend_state.enable_blend =
      true;
  this->pipelineInitVars.colorTargetDescriptions[0].blend_state.color_blend_op =
      SDL_GPU_BLENDOP_ADD;
  this->pipelineInitVars.colorTargetDescriptions[0].blend_state.alpha_blend_op =
      SDL_GPU_BLENDOP_ADD;
  this->pipelineInitVars.colorTargetDescriptions[0]
      .blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
  this->pipelineInitVars.colorTargetDescriptions[0]
      .blend_state.dst_color_blendfactor =
      SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  this->pipelineInitVars.colorTargetDescriptions[0]
      .blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
  this->pipelineInitVars.colorTargetDescriptions[0]
      .blend_state.dst_alpha_blendfactor =
      SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
  this->pipelineInitVars.colorTargetDescriptions[0].format =
      SDL_GetGPUSwapchainTextureFormat(this->basicInitVars.GPU,
                                       this->basicInitVars.window);
}
void Renderer::PipelineInit() {
  this->pipelineInitVars.vertex_shader =
      LoadShader(this->basicInitVars.GPU, "Shader.vert", 0, 1, 0, 0);
  if (!this->pipelineInitVars.vertex_shader) {
    SDL_Log("Couldn't load vertex shader: %s", SDL_GetError());
  }
  this->pipelineInitVars.fragment_shader =
      LoadShader(this->basicInitVars.GPU, "Shader.frag", 0, 0, 0, 0);
  if (!this->pipelineInitVars.fragment_shader) {
    SDL_Log("Couldn't load fragment shader: %s", SDL_GetError());
  }

  SDL_zero(this->pipelineInitVars.pipeline_desc);

  // Setup depth stencil for proper depth testing
  this->pipelineInitVars.pipeline_desc.depth_stencil_state.enable_depth_test =
      true;
  this->pipelineInitVars.pipeline_desc.depth_stencil_state.enable_depth_write =
      true;
  this->pipelineInitVars.pipeline_desc.depth_stencil_state.compare_op =
      SDL_GPU_COMPAREOP_LESS;
  this->pipelineInitVars.pipeline_desc.depth_stencil_state.enable_stencil_test =
      false;

  this->pipelineInitVars.pipeline_desc.target_info.num_color_targets = 1;
  this->pipelineInitVars.pipeline_desc.target_info.color_target_descriptions =
      this->pipelineInitVars.colorTargetDescriptions;
  this->pipelineInitVars.pipeline_desc.target_info.has_depth_stencil_target =
      true;
  this->pipelineInitVars.pipeline_desc.target_info.depth_stencil_format =
      SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

  this->pipelineInitVars.pipeline_desc.primitive_type =
      SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
  this->pipelineInitVars.pipeline_desc.vertex_shader =
      pipelineInitVars.vertex_shader;
  this->pipelineInitVars.pipeline_desc.fragment_shader =
      pipelineInitVars.fragment_shader;

  this->pipelineInitVars.pipeline_desc.rasterizer_state = {
      .fill_mode = SDL_GPU_FILLMODE_FILL,
      .cull_mode = SDL_GPU_CULLMODE_BACK, // Enable back-face culling
      .front_face = SDL_GPU_FRONTFACE_CLOCKWISE};

  VertexGPUInit();

  pipelineInitVars.pipeline_desc.vertex_input_state.num_vertex_buffers =
      1; // We only bind one buffer at a time
  pipelineInitVars.pipeline_desc.vertex_input_state.vertex_buffer_descriptions =
      &this->pipelineInitVars.vertex_buffer_desc;
  pipelineInitVars.pipeline_desc.vertex_input_state.num_vertex_attributes = 2;
  pipelineInitVars.pipeline_desc.vertex_input_state.vertex_attributes =
      this->pipelineInitVars.vertex_attributes;

  pipelineInitVars.pipeline_desc.props = 0;

  this->pipelineInitVars.graphicsPipeline = SDL_CreateGPUGraphicsPipeline(
      this->basicInitVars.GPU, &this->pipelineInitVars.pipeline_desc);

  // Transparent pipeline (same but with depth write disabled)
  this->pipelineInitVars.pipeline_desc.depth_stencil_state.enable_depth_write =
      false;
  this->pipelineInitVars.transparentPipeline = SDL_CreateGPUGraphicsPipeline(
      this->basicInitVars.GPU, &this->pipelineInitVars.pipeline_desc);
}
Renderer::Renderer(GameClient &gameClient)
    : gameClient(gameClient), chunkManager() {

  Init();
  GenerateBuffer();
  ColorTargetDes();
  PipelineInit();

  if (!this->pipelineInitVars.graphicsPipeline ||
      !this->pipelineInitVars.transparentPipeline) {
    SDL_Log("Failed to create pipelines: %s", SDL_GetError());
  }

  SDL_ReleaseGPUShader(this->basicInitVars.GPU,
                       this->pipelineInitVars.vertex_shader);
  SDL_ReleaseGPUShader(this->basicInitVars.GPU,
                       this->pipelineInitVars.fragment_shader);

  UpdateViewportAndProjection();
}