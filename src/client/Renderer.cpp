#include "../../include/client/Renderer.hpp"
#include "../../include/client/GameClient.hpp"
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <cmath>
#include <iostream>
#include <ostream>
#include <vector>

const float FOV = 90.0f;
const float Znear = 0.1f;
constexpr float Zfar = 500.0f;
constexpr int RenderDistance = 10;
constexpr float PlayerRad = 0.4;
const Vector3 PlayerModel[6][4] = {
    {                                // Front (+Z)
     {PlayerRad, -1.62, PlayerRad},  // bottom-right (feet level)
     {-PlayerRad, -1.62, PlayerRad}, // bottom-left
     {PlayerRad, 0.18, PlayerRad},   // top-right (head level)
     {-PlayerRad, 0.18, PlayerRad}}, // top-left

    {                                 // Back (-Z)
     {-PlayerRad, -1.62, -PlayerRad}, // bottom-left
     {PlayerRad, -1.62, -PlayerRad},  // bottom-right
     {-PlayerRad, 0.18, -PlayerRad},  // top-left
     {PlayerRad, 0.18, -PlayerRad}},  // top-right

    {                                 // Right (+X)
     {PlayerRad, 0.18, PlayerRad},    // front-top
     {PlayerRad, 0.18, -PlayerRad},   // back-top
     {PlayerRad, -1.62, PlayerRad},   // front-bottom
     {PlayerRad, -1.62, -PlayerRad}}, // back-bottom

    {                                 // Left (-X)
     {-PlayerRad, 0.18, -PlayerRad},  // back-top
     {-PlayerRad, 0.18, PlayerRad},   // front-top
     {-PlayerRad, -1.62, -PlayerRad}, // back-bottom
     {-PlayerRad, -1.62, PlayerRad}}, // front-bottom

    {                                // Top (+Y)
     {-PlayerRad, 0.18, -PlayerRad}, // back-left
     {PlayerRad, 0.18, -PlayerRad},  // back-right
     {-PlayerRad, 0.18, PlayerRad},  // front-left
     {PlayerRad, 0.18, PlayerRad}},  // front-right

    {                                 // Bottom (-Y)
     {PlayerRad, -1.62, PlayerRad},   // front-right
     {PlayerRad, -1.62, -PlayerRad},  // back-right
     {-PlayerRad, -1.62, PlayerRad},  // front-left
     {-PlayerRad, -1.62, -PlayerRad}} // back-left
};
const Vector3 Verts[6][4] = {
    {// Front (+Z) - looking at face from outside (positive Z direction)
     // Counter-clockwise: bottom-right, bottom-left, top-right, top-left
     {1.0, 0.0, 1.0},  // 0: bottom-right
     {0.0, 0.0, 1.0},  // 1: bottom-left
     {1.0, 1.0, 1.0},  // 2: top-right
     {0.0, 1.0, 1.0}}, // 3: top-left
    {// Back (-Z) - looking at face from outside (negative Z direction)
     // Counter-clockwise: bottom-left, bottom-right, top-left, top-right
     {0.0, 0.0, 0.0},  // 0: bottom-left
     {1.0, 0.0, 0.0},  // 1: bottom-right
     {0.0, 1.0, 0.0},  // 2: top-left
     {1.0, 1.0, 0.0}}, // 3: top-right
    {// Right (+X) - looking at face from outside (positive X direction)
     // Counter-clockwise when viewed from +X: front-bottom, back-bottom,
     // front-top, back-top
     {1.0, 1.0, 1.0},  // 2: front-top
     {1.0, 1.0, 0.0},  // 3: back-top
     {1.0, 0.0, 1.0},  // 0: front-bottom
     {1.0, 0.0, 0.0}}, // 1: back-bottom
    {// Left (-X) - looking at face from outside (negative X direction)
     // Counter-clockwise when viewed from -X: back-bottom, front-bottom,
     // back-top, front-top
     {0.0, 1.0, 0.0},  // 2: back-top
     {0.0, 1.0, 1.0},  // 3: front-top
     {0.0, 0.0, 0.0},  // 0: back-bottom
     {0.0, 0.0, 1.0}}, // 1: front-bottom:
    {// Top (+Y) - looking at face from outside (positive Y direction)
     // Counter-clockwise: back-left, back-right, front-left, front-right
     {0.0, 1.0, 0.0},  // 0: back-left
     {1.0, 1.0, 0.0},  // 1: back-right
     {0.0, 1.0, 1.0},  // 2: front-left
     {1.0, 1.0, 1.0}}, // 3: front-right
    {// Bottom (-Y) - looking at face from outside (negative Y direction, from
     // below) Counter-clockwise when viewed from below: front-right,
     // back-right, front-left, back-left
     {1.0, 0.0, 1.0},   // 0: front-right
     {1.0, 0.0, 0.0},   // 1: back-right
     {0.0, 0.0, 1.0},   // 2: front-left
     {0.0, 0.0, 0.0}}}; // 3: back-left
const Vector3 Direction[6] = {
    {0, 0, 1},  // Front
    {0, 0, -1}, // Back
    {1, 0, 0},  // Right
    {-1, 0, 0}, // Left
    {0, 1, 0},  // Top
    {0, -1, 0}  // Bottom
};
const Color Colors[6] = {
    {20, 20, 20}, // Front (+Z)
    {20, 20, 20}, // Back (-Z)
    {40, 40, 40}, // Right (+X)
    {40, 40, 40}, // Left (-X)
    {0, 0, 0},    // Top (+Y)
    {70, 70, 70}, // Bottom (-Y)
};

Matrix Perspective(float Fov, float Aspect, float Near, float Far) {
  Matrix m(4, 4, 0.0f);
  float tanHalfFov = std::tan(Fov * PI / 360.0f);
  m(0, 0) = 1.0f / (Aspect * tanHalfFov);
  m(1, 1) = 1.0f / tanHalfFov;
  m(2, 2) = Far / (Far - Near);
  m(3, 2) = 1.0f;
  m(2, 3) = -(Far * Near) / (Far - Near);
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
SDL_FPoint getUV(int tileIndex, float cornerX, float cornerY) {
  const int tileSize = 16;
  const int atlasSize = 64;
  const float pixelNudge = 0.5f;

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
auto Renderer::AddRect(float x, float y, float w, float h, Vector3 color,
                       float blockID) {
  this->uiVars.uiVertices.push_back(
      {{x, y, 0.0f}, color, {0.0f, 1.0f}, blockID});
  this->uiVars.uiVertices.push_back(
      {{x + w, y, 0.0f}, color, {1.0f, 1.0f}, blockID});
  this->uiVars.uiVertices.push_back(
      {{x, y + h, 0.0f}, color, {0.0f, 0.0f}, blockID});
  this->uiVars.uiVertices.push_back(
      {{x + w, y, 0.0f}, color, {1.0f, 1.0f}, blockID});
  this->uiVars.uiVertices.push_back(
      {{x + w, y + h, 0.0f}, color, {1.0f, 0.0f}, blockID});
  this->uiVars.uiVertices.push_back(
      {{x, y + h, 0.0f}, color, {0.0f, 0.0f}, blockID});
}
void Renderer::UICrossHair() {
  // Crosshair size
  float crossSize = 0.02f;
  float sizeX = crossSize / this->runTimeRenderVars.aspect;
  float sizeY = crossSize;

  // Crosshair lines
  float thickness = 0.002f;
  // Horizontal
  AddRect(-sizeX, -thickness, sizeX * 2, thickness * 2, {1, 1, 1});
  // Vertical
  AddRect(-thickness / this->runTimeRenderVars.aspect, -sizeY,
          thickness * 2 / this->runTimeRenderVars.aspect, sizeY * 2, {1, 1, 1});
}
void Renderer::UIInventory(const std::vector<Slot> &inventory,
                           int inventorySlot) {
  // 2. Inventory / Hotbar
  float hotbarHeight = 0.12f;
  float slotCount = 8;
  float slotSpacing = 0.01f;
  // Logical width (same as height for square slots)
  float logicalSlotSize = hotbarHeight - 2 * slotSpacing;
  float logicalTotalWidth =
      slotCount * logicalSlotSize + (slotCount + 1) * slotSpacing;

  float hotbarWidthNDC = logicalTotalWidth / this->runTimeRenderVars.aspect;
  float hotbarX = -hotbarWidthNDC / 2.0f;
  float hotbarY = -0.95f;

  // Background
  AddRect(hotbarX, hotbarY, hotbarWidthNDC, hotbarHeight, {0.1f, 0.1f, 0.1f});

  for (int i = 0; i < slotCount; i++) {
    float xNDC = hotbarX + (slotSpacing + i * (logicalSlotSize + slotSpacing)) /
                               this->runTimeRenderVars.aspect;
    float yNDC = hotbarY + slotSpacing;
    float wNDC = logicalSlotSize / this->runTimeRenderVars.aspect;
    float hNDC = logicalSlotSize;

    // Slot background
    Vector3 bgColor = (i == inventorySlot) ? Vector3{0.4f, 0.4f, 0.4f}
                                           : Vector3{0.2f, 0.2f, 0.2f};
    AddRect(xNDC, yNDC, wNDC, hNDC, bgColor);

    // Border highlight
    if (i == inventorySlot) {
      float b = 0.005f;
      float bX = b / this->runTimeRenderVars.aspect;
      float bY = b;
      // top
      AddRect(xNDC - bX, yNDC + hNDC, wNDC + 2 * bX, bY, {1.0f, 1.0f, 1.0f});
      // bottom
      AddRect(xNDC - bX, yNDC - bY, wNDC + 2 * bX, bY, {1.0f, 1.0f, 1.0f});
      // left
      AddRect(xNDC - bX, yNDC, bX, hNDC, {1.0f, 1.0f, 1.0f});
      // right
      AddRect(xNDC + wNDC, yNDC, bX, hNDC, {1.0f, 1.0f, 1.0f});
    }

    // Item icon
    if (i < inventory.size() && inventory[i].Type != 0) {
      float iconPadding = 0.015f;
      float pX = iconPadding / this->runTimeRenderVars.aspect;
      float pY = iconPadding;
      AddRect(xNDC + pX, yNDC + pY, wNDC - 2 * pX, hNDC - 2 * pY, {1, 1, 1},
              (float)inventory[i].Type);
    }
  }
}
void Renderer::DrawUI(SDL_GPUCommandBuffer *cmd,
                      const std::vector<Slot> &inventory, int inventorySlot) {

  UICrossHair();
  UIInventory(inventory, inventorySlot);

  size_t Length = uiVars.uiVertices.size();

  // Upload vertex data to GPU
  void *mapData = SDL_MapGPUTransferBuffer(
      this->basicInitVars.GPU, this->uiVars.UIVertexTransferBuffer, true);
  SDL_memcpy(mapData, uiVars.uiVertices.data(),
             uiVars.uiVertices.size() * sizeof(Vertex));
  SDL_UnmapGPUTransferBuffer(this->basicInitVars.GPU,
                             this->uiVars.UIVertexTransferBuffer);

  // Copy data to GPU buffer
  SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(cmd);
  SDL_GPUTransferBufferLocation src = {this->uiVars.UIVertexTransferBuffer, 0};
  SDL_GPUBufferRegion dst = {
      this->uiVars.UIVertexBuffer, 0,
      (Uint32)(this->uiVars.uiVertices.size() * sizeof(Vertex))};
  SDL_UploadToGPUBuffer(copyPass, &src, &dst, true);
  SDL_EndGPUCopyPass(copyPass);

  // Setup color target to load existing content
  SDL_GPUColorTargetInfo color_target_info;
  SDL_zero(color_target_info);
  color_target_info.texture = this->runTimeRenderVars.swap_texture;
  color_target_info.load_op = SDL_GPU_LOADOP_LOAD;
  color_target_info.store_op = SDL_GPU_STOREOP_STORE;

  // Begin UI render pass
  SDL_GPURenderPass *uiPass =
      SDL_BeginGPURenderPass(cmd, &color_target_info, 1, nullptr);

  // Bind UI pipeline and textures
  SDL_BindGPUGraphicsPipeline(uiPass, this->pipelineInitVars.uiPipeline);

  // Bind Texture Atlas and Sampler for UI
  if (this->TextureAtlas && this->Sampler) {
    SDL_GPUTextureSamplerBinding samplerBinding = {this->TextureAtlas,
                                                   this->Sampler};
    SDL_BindGPUFragmentSamplers(uiPass, 0, &samplerBinding, 1);
  }

  SDL_GPUBufferBinding vBinding = {this->uiVars.UIVertexBuffer, 0};
  SDL_BindGPUVertexBuffers(uiPass, 0, &vBinding, 1);
  SDL_DrawGPUPrimitives(uiPass, (Uint32)this->uiVars.uiVertices.size(), 1, 0,
                        0);
  SDL_EndGPURenderPass(uiPass);

  this->uiVars.uiVertices.clear();
}
std::vector<ChunkDistance> Renderer::SortChunks(Player &player) {
  Vector3 PlayerChunk = (player.Position / 16).Truncate();
  PlayerChunk.y = 0;

  SpiralIterator spiral(RenderDistance * 2 + 1);
  std::vector<ChunkDistance> visibleChunkList;

  int maxChunks = (RenderDistance * 2 + 1) * (RenderDistance * 2 + 1);
  visibleChunkList.reserve(maxChunks);

  while (spiral.hasNext()) {
    std::pair<int, int> Pos = spiral.next();

    Vector3 ChunkPos = {(float)Pos.first, 0, (float)Pos.second};
    ChunkPos += PlayerChunk;

    Vector3 Min = {ChunkPos.x * ChunkPrefab::xSize, 0,
                   ChunkPos.z * ChunkPrefab::zSize};
    Vector3 Max = {(ChunkPos.x + 1) * ChunkPrefab::xSize, ChunkPrefab::ySize,
                   (ChunkPos.z + 1) * ChunkPrefab::zSize};

    if (!frustum.isChunkInFrustum(Min, Max))
      continue;

    ChunkPrefab &chunk = chunkManager.get_chunk(ChunkPos);

    Vector3 chunkCenter = ChunkPos * 16.0f + Vector3(8, 0, 8);
    float d2 = (chunkCenter - player.Position).LengthSquared();

    visibleChunkList.push_back({&chunk, d2});
  }

  return visibleChunkList;
}

void Renderer::DrawTerrain(Player &player) {
  // 1. Collect visible chunks
  std::vector<ChunkDistance> visibleChunks = SortChunks(player);

  // 2. Prepare Opaque list (stable sort for buffer consistency)
  std::vector<ChunkPrefab *> opaqueChunks;
  opaqueChunks.reserve(visibleChunks.size());
  for (auto &cd : visibleChunks) {
    opaqueChunks.push_back(cd.chunk);
  }

  // Stable sort by coordinates to keep chunks in the same buffers
  std::sort(opaqueChunks.begin(), opaqueChunks.end(),
            [](ChunkPrefab *a, ChunkPrefab *b) {
              if (a->xPos != b->xPos)
                return a->xPos < b->xPos;
              return a->zPos < b->zPos;
            });

  // 3. Opaque Pass - Fill buffers 0 to totalBuffers-2 using stable chunks
  int currentOpaqueIdx = 0;
  for (int bufferIdx = 0; bufferIdx < this->totalBuffers - 1; bufferIdx++) {
    auto *mesh = &this->Terrain[bufferIdx];

    std::vector<ChunkPrefab *> newChunks;
    bool anyChunkDirty = false;

    for (int i = 0;
         i < chunksPerBuffer && currentOpaqueIdx < (int)opaqueChunks.size();
         i++) {
      ChunkPrefab *c = opaqueChunks[currentOpaqueIdx++];
      newChunks.push_back(c);
      if (c->needsMeshUpdate)
        anyChunkDirty = true;
    }

    // Check if buffer needs re-upload
    bool setChanged = (newChunks != mesh->currentChunks);
    if (!setChanged && !anyChunkDirty && !mesh->needsUpdate) {
      // Buffer is up to date, but we still need to clear transparency from it
      // if it had any from previous versions of the code
      mesh->TransparentIndexCount = 0;
      continue;
    }

    mesh->currentChunks = newChunks;
    mesh->needsUpdate = false;
    mesh->BaseVertex = 0;
    mesh->BaseIndex = 0;
    mesh->TransparentIndexCount = 0;

    if (newChunks.empty()) {
      mesh->OpaqueIndexCount = 0;
      continue;
    }

    Vertex *Vertexdata = (Vertex *)SDL_MapGPUTransferBuffer(
        this->basicInitVars.GPU, mesh->VertextransferBuffer, true);
    Uint32 *Indexdata = (Uint32 *)SDL_MapGPUTransferBuffer(
        this->basicInitVars.GPU, mesh->IndextransferBuffer, true);

    if (!Vertexdata || !Indexdata)
      continue;

    size_t currentVertexOffset = 0;
    size_t currentIndexOffset = 0;
    const size_t maxVertices = chunksPerBuffer * 4 * 2100;
    const size_t maxIndices = chunksPerBuffer * 6 * 2100;

    for (auto *chunk : newChunks) {
      Vector3 chunkPosKey = {(float)chunk->xPos / 16, 0,
                             (float)chunk->zPos / 16};

      if (chunk->needsMeshUpdate ||
          opaqueMeshCache.find(chunkPosKey) == opaqueMeshCache.end()) {
        auto &cache = opaqueMeshCache[chunkPosKey];
        cache.vertices.clear();
        cache.indices.clear();
        cache.vertices.reserve(chunk->allFaces.size() * 4);
        cache.indices.reserve(chunk->allFaces.size() * 6);

        Vector3 chunkWorldPos{(float)chunk->xPos, 0, (float)chunk->zPos};

        for (const auto &face : chunk->allFaces) {
          if (face.Transparent)
            continue;

          Vector3 worldPos = face.blockPos + chunkWorldPos;
          Vector3 faceColor = BlockDef[face.blockID].color.ToFloat() -
                              Colors[(int)(face.side)].ToFloat();

          Uint32 baseV = (Uint32)cache.vertices.size();
          float fw = (float)face.w;
          float fh = (float)face.h;

          for (int j = 0; j < 4; j++) {
            Vector3 v = Verts[face.side][j];
            SDL_FPoint uv;
            if (face.side < 2) {
              v.x *= fw;
              v.y *= fh;
              uv = {Verts[face.side][j].x * fw, Verts[face.side][j].y * fh};
            } else if (face.side < 4) {
              v.z *= fw;
              v.y *= fh;
              uv = {Verts[face.side][j].z * fw, Verts[face.side][j].y * fh};
            } else {
              v.x *= fw;
              v.z *= fh;
              uv = {Verts[face.side][j].x * fw, Verts[face.side][j].z * fh};
            }
            cache.vertices.push_back(
                {v + worldPos, faceColor, uv,
                 (float)BlockDef[face.blockID].Textures[face.side],
                 (float)face.LightLevel});
          }
          cache.indices.push_back(baseV + 0);
          cache.indices.push_back(baseV + 2);
          cache.indices.push_back(baseV + 1);
          cache.indices.push_back(baseV + 1);
          cache.indices.push_back(baseV + 2);
          cache.indices.push_back(baseV + 3);
        }
        chunk->needsMeshUpdate = false;
      }

      auto &cache = opaqueMeshCache[chunkPosKey];
      if (cache.vertices.empty())
        continue;

      if (currentVertexOffset + cache.vertices.size() > maxVertices ||
          currentIndexOffset + cache.indices.size() > maxIndices)
        break;

      memcpy(&Vertexdata[currentVertexOffset], cache.vertices.data(),
             cache.vertices.size() * sizeof(Vertex));

      Uint32 vertexBase = (Uint32)currentVertexOffset;
      for (size_t i = 0; i < cache.indices.size(); i++) {
        Indexdata[currentIndexOffset + i] = cache.indices[i] + vertexBase;
      }

      currentVertexOffset += cache.vertices.size();
      currentIndexOffset += cache.indices.size();
    }

    mesh->OpaqueIndexCount = (int)currentIndexOffset;
    mesh->BaseVertex = (int)currentVertexOffset;
    mesh->BaseIndex = (int)currentIndexOffset;

    SDL_UnmapGPUTransferBuffer(this->basicInitVars.GPU,
                               mesh->VertextransferBuffer);
    SDL_UnmapGPUTransferBuffer(this->basicInitVars.GPU,
                               mesh->IndextransferBuffer);

    SDL_GPUTransferBufferLocation vLoc{mesh->VertextransferBuffer, 0};
    SDL_GPUBufferRegion vReg{mesh->VertexBuffer.buffer, 0,
                             (Uint32)(currentVertexOffset * sizeof(Vertex))};
    SDL_UploadToGPUBuffer(this->runTimeRenderVars.copyPass, &vLoc, &vReg, true);

    SDL_GPUTransferBufferLocation iLoc{mesh->IndextransferBuffer, 0};
    SDL_GPUBufferRegion iReg{mesh->IndexBuffer.buffer, 0,
                             (Uint32)(currentIndexOffset * sizeof(Uint32))};
    SDL_UploadToGPUBuffer(this->runTimeRenderVars.copyPass, &iLoc, &iReg, true);
  }

  // 4. Transparent Pass - Dedicated buffer (totalBuffers-1)
  std::vector<TransparentDrawnFace> transparentFaces;
  transparentFaces.reserve(500);

  for (auto &cd : visibleChunks) {
    ChunkPrefab *chunk = cd.chunk;
    Vector3 chunkWorldPos{(float)chunk->xPos, 0, (float)chunk->zPos};
    for (auto &face : chunk->allFaces) {
      if (face.Transparent) {
        transparentFaces.push_back({face.blockPos + chunkWorldPos,
                                    (int)face.side, (int)face.blockID,
                                    (int)face.w, (int)face.h, face.LightLevel});
      }
    }
  }

  Mesh *tMesh = &this->Terrain[this->totalBuffers - 1];
  tMesh->OpaqueIndexCount = 0;

  if (transparentFaces.empty()) {
    tMesh->TransparentIndexCount = 0;
    return;
  }

  std::sort(
      transparentFaces.begin(), transparentFaces.end(),
      [&player](const TransparentDrawnFace &a, const TransparentDrawnFace &b) {
        return (a.blockPos - player.Position).LengthSquared() >
               (b.blockPos - player.Position).LengthSquared();
      });

  Vertex *vData = (Vertex *)SDL_MapGPUTransferBuffer(
      this->basicInitVars.GPU, tMesh->VertextransferBuffer, true);
  Uint32 *iData = (Uint32 *)SDL_MapGPUTransferBuffer(
      this->basicInitVars.GPU, tMesh->IndextransferBuffer, true);

  if (vData && iData) {
    size_t vOffset = 0;
    size_t iOffset = 0;
    const size_t maxV = chunksPerBuffer * 4 * 2100;
    const size_t maxI = chunksPerBuffer * 6 * 2100;

    for (auto &face : transparentFaces) {
      if (vOffset + 4 > maxV || iOffset + 12 > maxI)
        break;

      Vector3 faceColor = BlockDef[face.blockID].color.ToFloat() -
                          Colors[(int)(face.side)].ToFloat();

      for (int j = 0; j < 4; j++) {
        Vector3 v = Verts[face.side][j];
        SDL_FPoint uv;
        float fw = (float)face.w;
        float fh = (float)face.h;
        if (face.side < 2) {
          v.x *= fw;
          v.y *= fh;
          uv = {Verts[face.side][j].x * fw, Verts[face.side][j].y * fh};
        } else if (face.side < 4) {
          v.z *= fw;
          v.y *= fh;
          uv = {Verts[face.side][j].z * fw, Verts[face.side][j].y * fh};
        } else {
          v.x *= fw;
          v.z *= fh;
          uv = {Verts[face.side][j].x * fw, Verts[face.side][j].z * fh};
        }
        vData[vOffset + j] = {v + face.blockPos, faceColor, uv,
                              (float)BlockDef[face.blockID].Textures[face.side],
                              (float)face.light};
      }

      if (face.blockID == 5) { // Water (double sided)
        iData[iOffset + 0] = (Uint32)vOffset + 0;
        iData[iOffset + 1] = (Uint32)vOffset + 2;
        iData[iOffset + 2] = (Uint32)vOffset + 1;
        iData[iOffset + 3] = (Uint32)vOffset + 1;
        iData[iOffset + 4] = (Uint32)vOffset + 2;
        iData[iOffset + 5] = (Uint32)vOffset + 3;
        iData[iOffset + 6] = (Uint32)vOffset + 0;
        iData[iOffset + 7] = (Uint32)vOffset + 1;
        iData[iOffset + 8] = (Uint32)vOffset + 2;
        iData[iOffset + 9] = (Uint32)vOffset + 1;
        iData[iOffset + 10] = (Uint32)vOffset + 3;
        iData[iOffset + 11] = (Uint32)vOffset + 2;
        iOffset += 12;
      } else {
        iData[iOffset + 0] = (Uint32)vOffset + 0;
        iData[iOffset + 1] = (Uint32)vOffset + 2;
        iData[iOffset + 2] = (Uint32)vOffset + 1;
        iData[iOffset + 3] = (Uint32)vOffset + 1;
        iData[iOffset + 4] = (Uint32)vOffset + 2;
        iData[iOffset + 5] = (Uint32)vOffset + 3;
        iOffset += 6;
      }
      vOffset += 4;
    }

    tMesh->TransparentIndexCount = (int)iOffset;
    tMesh->BaseVertex = (int)vOffset;

    SDL_UnmapGPUTransferBuffer(this->basicInitVars.GPU,
                               tMesh->VertextransferBuffer);
    SDL_UnmapGPUTransferBuffer(this->basicInitVars.GPU,
                               tMesh->IndextransferBuffer);

    SDL_GPUTransferBufferLocation vLoc{tMesh->VertextransferBuffer, 0};
    SDL_GPUBufferRegion vReg{tMesh->VertexBuffer.buffer, 0,
                             (Uint32)(vOffset * sizeof(Vertex))};
    SDL_UploadToGPUBuffer(this->runTimeRenderVars.copyPass, &vLoc, &vReg, true);

    SDL_GPUTransferBufferLocation iLoc{tMesh->IndextransferBuffer, 0};
    SDL_GPUBufferRegion iReg{tMesh->IndexBuffer.buffer, 0,
                             (Uint32)(iOffset * sizeof(Uint32))};
    SDL_UploadToGPUBuffer(this->runTimeRenderVars.copyPass, &iLoc, &iReg, true);
  }
}
void Renderer::DrawBg(std::vector<Player> &players) {
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

  SDL_WaitAndAcquireGPUSwapchainTexture(
      this->runTimeRenderVars.cmdRender, this->basicInitVars.window,
      &this->runTimeRenderVars.swap_texture, &this->basicInitVars.Width,
      &this->basicInitVars.Height);

  if (this->runTimeRenderVars.swap_texture == NULL) {
    std::cout << "La swap_texture no s'ha fet be\n";
    SDL_SubmitGPUCommandBuffer(this->runTimeRenderVars.cmdRender);
    return; // CRITICAL: Must return here!
  }
  SDL_GPUColorTargetInfo color_target_info;
  SDL_zero(color_target_info);
  color_target_info.clear_color = SDL_FColor{0.1f, 0.79f, 1.0f, 1.0f};
  color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
  color_target_info.store_op = SDL_GPU_STOREOP_STORE;
  color_target_info.texture = this->runTimeRenderVars.swap_texture;

  // Setup depth buffer for proper depth testing
  SDL_GPUDepthStencilTargetInfo depth_target_info;
  SDL_zero(depth_target_info);
  depth_target_info.texture = this->DepthTexture;
  depth_target_info.clear_depth = 1.0f;
  depth_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
  depth_target_info.store_op = SDL_GPU_STOREOP_DONT_CARE;
  depth_target_info.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
  depth_target_info.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
  depth_target_info.cycle = true;

  Player player = players[0];

  Matrix view = LookAt(player.Rotation, player.Position);

  float aspect =
      (float)this->basicInitVars.Width / (float)this->basicInitVars.Height;
  Matrix proj = Perspective(FOV, aspect, Znear, Zfar);

  // mvp.print();
  SDL_PushGPUVertexUniformData(
      this->runTimeRenderVars.cmdRender, 0, proj.getColumnMajorData().data(),
      sizeof(float) * proj.getColumnMajorData().size());

  SDL_PushGPUVertexUniformData(
      this->runTimeRenderVars.cmdRender, 1, view.getColumnMajorData().data(),
      sizeof(float) * view.getColumnMajorData().size());

  Uint32 water = (this->chunkManager.GetBlockID(player.Position) == 5) ? 1 : 0;

  SDL_PushGPUFragmentUniformData(this->runTimeRenderVars.cmdRender, 0, &water,
                                 sizeof(Uint32));

  this->runTimeRenderVars.pass =
      SDL_BeginGPURenderPass(this->runTimeRenderVars.cmdRender,
                             &color_target_info, 1, &depth_target_info);

  // Bind Texture Atlas and Sampler
  if (this->TextureAtlas && this->Sampler) {
    SDL_GPUTextureSamplerBinding samplerBinding = {this->TextureAtlas,
                                                   this->Sampler};
    SDL_BindGPUFragmentSamplers(this->runTimeRenderVars.pass, 0,
                                &samplerBinding, 1);
  }

  // Pass 1: Draw Opaque everything
  SDL_BindGPUGraphicsPipeline(this->runTimeRenderVars.pass,
                              this->pipelineInitVars.graphicsPipeline);

  for (auto &mesh : this->Terrain) {
    if (mesh.OpaqueIndexCount > 0) {
      SDL_BindGPUVertexBuffers(this->runTimeRenderVars.pass, 0,
                               &mesh.VertexBuffer, 1);
      SDL_BindGPUIndexBuffer(this->runTimeRenderVars.pass, &mesh.IndexBuffer,
                             SDL_GPU_INDEXELEMENTSIZE_32BIT);
      SDL_DrawGPUIndexedPrimitives(this->runTimeRenderVars.pass,
                                   mesh.OpaqueIndexCount, 1, 0, 0, 0);
    }
  }

  // Pass 2: Draw Transparent everything
  SDL_BindGPUGraphicsPipeline(this->runTimeRenderVars.pass,
                              this->pipelineInitVars.transparentPipeline);

  for (auto &mesh : this->Terrain) {
    if (mesh.TransparentIndexCount > 0) {
      SDL_BindGPUVertexBuffers(this->runTimeRenderVars.pass, 0,
                               &mesh.VertexBuffer, 1);
      SDL_BindGPUIndexBuffer(this->runTimeRenderVars.pass, &mesh.IndexBuffer,
                             SDL_GPU_INDEXELEMENTSIZE_32BIT);
      SDL_DrawGPUIndexedPrimitives(this->runTimeRenderVars.pass,
                                   mesh.TransparentIndexCount, 1,
                                   mesh.OpaqueIndexCount, 0, 0);
    }
  }

  DrawPlayers(players);

  SDL_EndGPURenderPass(this->runTimeRenderVars.pass);
}
void Renderer::DrawPlayers(std::vector<Player> &players) {
  if (players.size() <= 1)
    return;

  std::vector<Vertex> verts;
  std::vector<Uint32> indices;

  int myId = gameClient.get_my_id();

  {
    std::lock_guard<std::mutex> lock(gameClient.get_mutex());
    for (const auto &p : players) {
      if (p.id == myId)
        continue;

      Vector3 pos = p.Position;
      Vector3 color = p.color.ToFloat();

      for (int side = 0; side < 6; side++) {
        Uint32 base = verts.size();
        for (int i = 0; i < 4; i++) {
          verts.push_back({PlayerModel[side][i] + pos, color, {0, 0}, 1.0f});
        }
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
      }
    }
  }

  if (verts.empty())
    return;

  // Upload to GPU
  void *vData =
      SDL_MapGPUTransferBuffer(basicInitVars.GPU, EntityTransferBuffer, true);
  SDL_memcpy(vData, verts.data(), verts.size() * sizeof(Vertex));
  SDL_UnmapGPUTransferBuffer(basicInitVars.GPU, EntityTransferBuffer);

  void *iData = SDL_MapGPUTransferBuffer(basicInitVars.GPU,
                                         EntityIndexTransferBuffer, true);
  SDL_memcpy(iData, indices.data(), indices.size() * sizeof(Uint32));
  SDL_UnmapGPUTransferBuffer(basicInitVars.GPU, EntityIndexTransferBuffer);

  SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(basicInitVars.GPU);
  SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
  SDL_GPUTransferBufferLocation vSrc = {EntityTransferBuffer, 0};
  SDL_GPUBufferRegion vDst = {EntityBuffer, 0,
                              (Uint32)(verts.size() * sizeof(Vertex))};
  SDL_UploadToGPUBuffer(copy, &vSrc, &vDst, true);
  SDL_GPUTransferBufferLocation iSrc = {EntityIndexTransferBuffer, 0};
  SDL_GPUBufferRegion iDst = {EntityIndexBuffer, 0,
                              (Uint32)(indices.size() * sizeof(Uint32))};
  SDL_UploadToGPUBuffer(copy, &iSrc, &iDst, true);
  SDL_EndGPUCopyPass(copy);
  SDL_SubmitGPUCommandBuffer(cmd);

  // Draw
  SDL_BindGPUGraphicsPipeline(runTimeRenderVars.pass,
                              pipelineInitVars.graphicsPipeline);
  SDL_GPUBufferBinding vBinding = {EntityBuffer, 0};
  SDL_BindGPUVertexBuffers(runTimeRenderVars.pass, 0, &vBinding, 1);
  SDL_GPUBufferBinding iBinding = {EntityIndexBuffer, 0};
  SDL_BindGPUIndexBuffer(runTimeRenderVars.pass, &iBinding,
                         SDL_GPU_INDEXELEMENTSIZE_32BIT);
  SDL_DrawGPUIndexedPrimitives(runTimeRenderVars.pass, (Uint32)indices.size(),
                               1, 0, 0, 0);
}
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
  float tanHalfFov = tan(FOV * PI / 360.0f);
  this->frustum =
      Frustum().createFrustumFromCamera(aspect, tanHalfFov, Znear, Zfar);
}
void Renderer::EventManager(Player &player, int &inventorySlot) {
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
    case SDL_EVENT_MOUSE_WHEEL: {
      if (this->basicInitVars.event.wheel.y > 0) {
        inventorySlot = (inventorySlot + 7) % 8; // Previous
      } else if (this->basicInitVars.event.wheel.y < 0) {
        inventorySlot = (inventorySlot + 1) % 8; // Next
      }
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
void Renderer::MainRenderLoop(std::vector<Slot> &inventory, int &inventorySlot,
                              std::vector<Player> &players) {
  EventManager(players[0], inventorySlot);

  // Transform frustum to world space based on player position and rotation
  // Create a fresh frustum in camera space
  this->runTimeRenderVars.aspect =
      (float)this->basicInitVars.Width / (float)this->basicInitVars.Height;
  float tanHalfFov = tan(FOV * PI / 360.0f);
  Frustum worldFrustum = Frustum::createFrustumFromCamera(
      this->runTimeRenderVars.aspect, tanHalfFov, Znear, Zfar);

  // Transform to world space using player's position and rotation
  worldFrustum.transformToWorldSpace(players[0].Position, players[0].Rotation);

  // Store the world-space frustum for use in DrawTerrain
  this->frustum = worldFrustum;

  DrawBg(players);

  DrawUI(this->runTimeRenderVars.cmdRender, inventory, inventorySlot);

  if (!SDL_SubmitGPUCommandBuffer(this->runTimeRenderVars.cmdRender)) {
    std::cout << "Failed to submit render command buffer\n";
    return;
  }
}

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *filename,
                          Uint32 sampler_count, Uint32 uniform_buffer_count,
                          Uint32 storage_buffer_count,
                          Uint32 storage_texture_count) {
  if (!device) {
    return NULL;
  }
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
  const char *entrypoint;
  const char *basepath = SDL_GetBasePath();
  char fullpath[512];
  if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV) {
    if (basepath) {
      SDL_snprintf(fullpath, sizeof(fullpath), "%sassets/shaders/%s.spv",
                   basepath, filename);
    } else {
      SDL_snprintf(fullpath, sizeof(fullpath), "assets/shaders/%s.spv",
                   filename);
    }
    entrypoint = "main";
    format = SDL_GPU_SHADERFORMAT_SPIRV;
  } else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL) {
    if (basepath) {
      SDL_snprintf(fullpath, sizeof(fullpath), "%sshaders/%s.dxil", basepath,
                   filename);
    } else {
      SDL_snprintf(fullpath, sizeof(fullpath), "shaders/%s.dxil", filename);
    }
    entrypoint = "main";
    format = SDL_GPU_SHADERFORMAT_DXIL;
  } else if (backend_formats & SDL_GPU_SHADERFORMAT_MSL) {
    if (basepath) {
      SDL_snprintf(fullpath, sizeof(fullpath), "%sshaders/%s.msl", basepath,
                   filename);
    } else {
      SDL_snprintf(fullpath, sizeof(fullpath), "shaders/%s.msl", filename);
    }
    entrypoint = "main0";
    format = SDL_GPU_SHADERFORMAT_MSL;
  } else {
    SDL_Log("No supported shader format found!");
    return NULL;
  }

  size_t code_size;
  void *code = SDL_LoadFile(fullpath, &code_size);
  if (!code) {
    SDL_Log("Couldn't load shader file %s: %s", fullpath, SDL_GetError());
    return NULL;
  }

  SDL_GPUShaderCreateInfo shader_info = {};
  shader_info.code_size = code_size;
  shader_info.code = (const unsigned char *)code;
  shader_info.entrypoint = entrypoint;
  shader_info.format = format;
  shader_info.stage = stage;
  shader_info.num_samplers = sampler_count;
  shader_info.num_storage_textures = storage_texture_count;
  shader_info.num_storage_buffers = storage_buffer_count;
  shader_info.num_uniform_buffers = uniform_buffer_count;

  SDL_GPUShader *shader = SDL_CreateGPUShader(device, &shader_info);
  if (!shader) {
    SDL_Log("Couldn't create shader %s: %s", filename, SDL_GetError());
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

  this->basicInitVars.GPU =
      SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
  if (this->basicInitVars.GPU) {
    SDL_Log("GPU Driver: %s", SDL_GetGPUDeviceDriver(this->basicInitVars.GPU));
  } else {
    SDL_Log("Failed to create GPU device: %s", SDL_GetError());
    return;
  }

  this->basicInitVars.window =
      SDL_CreateWindow("Bit Miner", this->basicInitVars.Width,
                       this->basicInitVars.Height, SDL_WINDOW_RESIZABLE);
  if (this->basicInitVars.window == nullptr) {
    std::cout << "Error creating basicInitVars.window: " << SDL_GetError();
    SDL_Quit();
    assert(false);
  }

  if (!SDL_ClaimWindowForGPUDevice(this->basicInitVars.GPU,
                                   this->basicInitVars.window)) {
    SDL_Log("Error claiming window for GPU device: %s", SDL_GetError());
  }
  this->basicInitVars.event = {};
  SDL_SetWindowRelativeMouseMode(basicInitVars.window, true);
}
void Renderer::GenerateBuffer() {
  // Calculate buffer packing
  int totalChunks = (RenderDistance * 2 + 1) * (RenderDistance * 2 + 1);
  for (int i = std::sqrt(totalChunks) + 1; i > 0; i--) {
    if (totalChunks % i) {
      chunksPerBuffer = i;
      break;
    }
  }
  this->totalBuffers = (totalChunks + chunksPerBuffer) / chunksPerBuffer +
                       1; // +1 for dedicated transparency

  // Each buffer now holds multiple chunks
  constexpr Uint32 singleChunkVertexSize = sizeof(Vertex) * 4 * 2100;
  constexpr Uint32 singleChunkIndexSize = sizeof(Uint32) * 6 * 2100;
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

  this->pipelineInitVars.vertex_attributes[2].buffer_slot = 0;
  this->pipelineInitVars.vertex_attributes[2].location = 2;
  this->pipelineInitVars.vertex_attributes[2].format =
      SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
  this->pipelineInitVars.vertex_attributes[2].offset = sizeof(float) * 6;

  this->pipelineInitVars.vertex_attributes[3].buffer_slot = 0;
  this->pipelineInitVars.vertex_attributes[3].location = 3;
  this->pipelineInitVars.vertex_attributes[3].format =
      SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
  this->pipelineInitVars.vertex_attributes[3].offset = sizeof(float) * 8;

  // ADD THIS - Attribute 4 for light
  this->pipelineInitVars.vertex_attributes[4].buffer_slot = 0;
  this->pipelineInitVars.vertex_attributes[4].location = 4;
  this->pipelineInitVars.vertex_attributes[4].format =
      SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
  this->pipelineInitVars.vertex_attributes[4].offset = sizeof(float) * 9;
}
void Renderer::LoadTexture() {
  if (!this->basicInitVars.GPU) {
    SDL_Log("Cannot load texture: GPU device not initialized");
    return;
  }

  const char *basePath = SDL_GetBasePath();
  char fullPath[512];
  if (basePath) {
    SDL_snprintf(fullPath, sizeof(fullPath),
                 "%sassets/Textures/TextureAtlas.png", basePath);
  } else {
    SDL_strlcpy(fullPath, "assets/Textures/TextureAtlas.png", sizeof(fullPath));
  }

  SDL_Surface *surface = SDL_LoadPNG(fullPath);
  if (!surface) {
    SDL_Log("Failed to load texture at %s: %s", fullPath, SDL_GetError());
    return;
  }

  SDL_Log("Loaded texture: %s (%dx%d)", fullPath, surface->w, surface->h);

  // Convert to a guaranteed RGBA8 per-pixel format
  SDL_Surface *rgbaSurface =
      SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
  SDL_DestroySurface(surface);
  if (!rgbaSurface) {
    SDL_Log("Failed to convert surface: %s", SDL_GetError());
    return;
  }
  surface = rgbaSurface;

  SDL_GPUTextureCreateInfo textureInfo = {};
  textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
  textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
  textureInfo.width = (Uint32)surface->w;
  textureInfo.height = (Uint32)surface->h;
  textureInfo.layer_count_or_depth = 1;
  textureInfo.num_levels = 1;
  textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

  this->TextureAtlas =
      SDL_CreateGPUTexture(this->basicInitVars.GPU, &textureInfo);
  if (!this->TextureAtlas) {
    SDL_Log("Failed to create GPU texture: %s", SDL_GetError());
    SDL_DestroySurface(surface);
    return;
  }

  // Upload data
  Uint32 textureSize = (Uint32)(surface->w * surface->h * 4);
  SDL_GPUTransferBufferCreateInfo transferInfo = {};
  transferInfo.size = textureSize;
  transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  SDL_GPUTransferBuffer *transferBuffer =
      SDL_CreateGPUTransferBuffer(this->basicInitVars.GPU, &transferInfo);

  if (!transferBuffer) {
    SDL_Log("Failed to create transfer buffer: %s", SDL_GetError());
    SDL_DestroySurface(surface);
    return;
  }

  void *data =
      SDL_MapGPUTransferBuffer(this->basicInitVars.GPU, transferBuffer, false);
  if (data) {
    // Copy row by row to handle potential pitch differences
    for (int y = 0; y < surface->h; y++) {
      SDL_memcpy((Uint8 *)data + (y * surface->w * 4),
                 (Uint8 *)surface->pixels + (y * surface->pitch),
                 surface->w * 4);
    }
    SDL_UnmapGPUTransferBuffer(this->basicInitVars.GPU, transferBuffer);
  }

  SDL_GPUCommandBuffer *cmd =
      SDL_AcquireGPUCommandBuffer(this->basicInitVars.GPU);
  if (cmd) {
    SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTextureTransferInfo texTransfer = {};
    texTransfer.transfer_buffer = transferBuffer;
    texTransfer.offset = 0;
    texTransfer.pixels_per_row = (Uint32)surface->w;
    texTransfer.rows_per_layer = (Uint32)surface->h;

    SDL_GPUTextureRegion texRegion = {};
    texRegion.texture = this->TextureAtlas;
    texRegion.w = (Uint32)surface->w;
    texRegion.h = (Uint32)surface->h;
    texRegion.d = 1;

    SDL_UploadToGPUTexture(copyPass, &texTransfer, &texRegion, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);
  }

  SDL_ReleaseGPUTransferBuffer(this->basicInitVars.GPU, transferBuffer);
  SDL_DestroySurface(surface);

  // Create sampler
  SDL_GPUSamplerCreateInfo samplerInfo = {};
  samplerInfo.min_filter = SDL_GPU_FILTER_NEAREST;
  samplerInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
  samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
  samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
  samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
  samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;

  this->Sampler = SDL_CreateGPUSampler(this->basicInitVars.GPU, &samplerInfo);
  if (this->Sampler) {
    SDL_Log("Sampler created successfully");
  } else {
    SDL_Log("Failed to create sampler: %s", SDL_GetError());
  }
  if (!this->Sampler) {
    SDL_Log("Failed to create sampler: %s", SDL_GetError());
  }
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
      LoadShader(this->basicInitVars.GPU, "Shader.vert", 0, 2, 0, 0);
  if (!this->pipelineInitVars.vertex_shader) {
    SDL_Log("Couldn't load vertex shader: %s", SDL_GetError());
  }
  this->pipelineInitVars.fragment_shader =
      LoadShader(this->basicInitVars.GPU, "Shader.frag", 1, 1, 0, 0);
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
  this->pipelineInitVars.pipeline_desc.rasterizer_state.fill_mode =
      SDL_GPU_FILLMODE_FILL;
  this->pipelineInitVars.pipeline_desc.rasterizer_state.cull_mode =
      SDL_GPU_CULLMODE_BACK;
  this->pipelineInitVars.pipeline_desc.rasterizer_state.front_face =
      SDL_GPU_FRONTFACE_CLOCKWISE;
  VertexGPUInit();

  pipelineInitVars.pipeline_desc.vertex_input_state.num_vertex_buffers =
      1; // We only bind one buffer at a time
  pipelineInitVars.pipeline_desc.vertex_input_state.vertex_buffer_descriptions =
      &this->pipelineInitVars.vertex_buffer_desc;
  pipelineInitVars.pipeline_desc.vertex_input_state.num_vertex_attributes = 5;
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

  // UI Pipeline
  SDL_GPUShader *ui_vert =
      LoadShader(this->basicInitVars.GPU, "UI.vert", 0, 0, 0, 0);
  SDL_GPUShader *ui_frag =
      LoadShader(this->basicInitVars.GPU, "UI.frag", 1, 0, 0, 0);

  SDL_GPUGraphicsPipelineCreateInfo ui_desc;
  SDL_zero(ui_desc);

  ui_desc.vertex_shader = ui_vert;
  ui_desc.fragment_shader = ui_frag;

  ui_desc.target_info.num_color_targets = 1;
  ui_desc.target_info.color_target_descriptions =
      this->pipelineInitVars.colorTargetDescriptions;
  ui_desc.target_info.has_depth_stencil_target = false;

  ui_desc.depth_stencil_state.enable_depth_test = false;
  ui_desc.depth_stencil_state.enable_depth_write = false;

  ui_desc.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
  ui_desc.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
  ui_desc.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;

  ui_desc.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

  ui_desc.vertex_input_state.num_vertex_buffers = 1;
  ui_desc.vertex_input_state.vertex_buffer_descriptions =
      &this->pipelineInitVars.vertex_buffer_desc;
  ui_desc.vertex_input_state.num_vertex_attributes = 4;
  ui_desc.vertex_input_state.vertex_attributes =
      this->pipelineInitVars.vertex_attributes;

  if (!ui_vert || !ui_frag) {
    SDL_Log("UI Shaders failed to load! vert: %p, frag: %p", (void *)ui_vert,
            (void *)ui_frag);
  } else {
    this->pipelineInitVars.uiPipeline =
        SDL_CreateGPUGraphicsPipeline(this->basicInitVars.GPU, &ui_desc);
    if (!this->pipelineInitVars.uiPipeline) {
      SDL_Log("Failed to create UI pipeline: %s", SDL_GetError());
    }
  }

  if (ui_vert)
    SDL_ReleaseGPUShader(this->basicInitVars.GPU, ui_vert);
  if (ui_frag)
    SDL_ReleaseGPUShader(this->basicInitVars.GPU, ui_frag);

  // Create UI Buffer
  SDL_GPUBufferCreateInfo uiBufferInfo = {};
  uiBufferInfo.size = sizeof(Vertex) * 2048; // Increased from 12 to 2048
  uiBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  this->uiVars.UIVertexBuffer =
      SDL_CreateGPUBuffer(this->basicInitVars.GPU, &uiBufferInfo);

  SDL_GPUTransferBufferCreateInfo uiTransferInfo = {};
  uiTransferInfo.size = sizeof(Vertex) * 2048; // Increased from 12 to 2048
  uiTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  this->uiVars.UIVertexTransferBuffer =
      SDL_CreateGPUTransferBuffer(this->basicInitVars.GPU, &uiTransferInfo);

  // Create Entity Buffer
  SDL_GPUBufferCreateInfo entityBufferInfo = {};
  entityBufferInfo.size =
      sizeof(Vertex) * 24 * MAX_PLAYERS; // Sufficient for cubes
  entityBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  this->EntityBuffer =
      SDL_CreateGPUBuffer(this->basicInitVars.GPU, &entityBufferInfo);

  SDL_GPUTransferBufferCreateInfo entityTransferInfo = {};
  entityTransferInfo.size = sizeof(Vertex) * 24 * MAX_PLAYERS;
  entityTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  this->EntityTransferBuffer =
      SDL_CreateGPUTransferBuffer(this->basicInitVars.GPU, &entityTransferInfo);

  // Create Entity Index Buffer
  SDL_GPUBufferCreateInfo entityIndexBufferInfo = {};
  entityIndexBufferInfo.size = sizeof(Uint32) * 36 * MAX_PLAYERS;
  entityIndexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
  this->EntityIndexBuffer =
      SDL_CreateGPUBuffer(this->basicInitVars.GPU, &entityIndexBufferInfo);

  SDL_GPUTransferBufferCreateInfo entityIndexTransferInfo = {};
  entityIndexTransferInfo.size = sizeof(Uint32) * 36 * MAX_PLAYERS;
  entityIndexTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  this->EntityIndexTransferBuffer = SDL_CreateGPUTransferBuffer(
      this->basicInitVars.GPU, &entityIndexTransferInfo);
}

Renderer::Renderer(GameClient &gameClient, ChunkManager &manager)
    : gameClient(gameClient), chunkManager(manager) {
  TextureAtlas = nullptr;
  Sampler = nullptr;

  Init();
  GenerateBuffer();
  ColorTargetDes();
  LoadTexture();
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