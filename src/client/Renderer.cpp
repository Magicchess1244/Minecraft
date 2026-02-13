#include "../../include/client/Renderer.hpp"
#include "../../include/client/GameClient.hpp"
#include <SDL3/SDL_gpu.h>
#include <cmath>
#include <iostream>
#include <ostream>
#include <vector>

const float FOV = 90.0f;
const float Znear = 0.1f;
constexpr float Zfar = 500.0f;
constexpr int RenderDistance = 17;
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
const Color Colors[3] = {
    {0, 0, 0},    // Front / Back
    {25, 25, 25}, // Right / Left
    {50, 50, 50}, // Top / Bottom
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
std::vector<ChunkDistance> Renderer::SortChunks(Player &player) {
  Vector3 PlayerChunk = (player.Position / 16).Truncate();
  PlayerChunk.y = 0;

  SpiralIterator spiral(RenderDistance * 2 + 1);
  std::vector<ChunkDistance> sortedChunkList;

  while (spiral.hasNext()) {
    std::pair<int, int> Pos = spiral.next();

    // Calculate chunk position in world space FIRST
    Vector3 ChunkPos = {(float)Pos.first, 0, (float)Pos.second};
    ChunkPos += PlayerChunk;

    // Now calculate world-space bounding box for this chunk
    Vector3 Min = {ChunkPos.x * ChunkPrefab::xSize, 0,
                   ChunkPos.z * ChunkPrefab::zSize};
    Vector3 Max = {(ChunkPos.x + 1) * ChunkPrefab::xSize, ChunkPrefab::ySize,
                   (ChunkPos.z + 1) * ChunkPrefab::zSize};

    if (!frustum.isChunkInFrustum(Min, Max))
      continue;

    ChunkPrefab &chunk = chunkManager.get_chunk(ChunkPos);

    // Use center of chunk for distance
    Vector3 chunkCenter = ChunkPos * 16.0f + Vector3(8, 0, 8);
    float d2 = (chunkCenter - player.Position).LengthSquared();

    sortedChunkList.push_back({&chunk, d2});
  }

  // 2. Sort chunks BACK TO FRONT (furthest first)
  std::sort(sortedChunkList.begin(), sortedChunkList.end(),
            [](const ChunkDistance &a, const ChunkDistance &b) {
              return a.distSq > b.distSq;
            });

  return sortedChunkList;
}
Vector3 getCoordinates(int pos) {
  // xSize=16 (2^4), ySize=64 (2^6), zSize=16 (2^4)
  // Index = x + y * xSize + z * xSize * ySize
  // Index = x + y * 16 + z * 1024
  int x = pos & 0xF;         // pos % 16
  int y = (pos >> 4) & 0x3F; // (pos / 16) % 64
  int z = (pos >> 10);       // pos / 1024
  return {(float)x, (float)y, (float)z};
}
void Renderer::DrawTerrain(Player &player) {
  // 1. Collect all chunks and their distances
  std::vector<ChunkDistance> sortedChunkList = SortChunks(player);

  // 2. Fill buffers using sorted chunks
  int currentSortedIdx = 0;
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

    std::vector<ChunkPrefab *> chunks;
    for (int i = 0;
         i < chunksPerBuffer && currentSortedIdx < sortedChunkList.size();
         i++) {
      chunks.push_back(sortedChunkList[currentSortedIdx++].chunk);
    }

    // OPTIMIZATION 1: Pre-calculate total size needed
    size_t totalOpaqueVertices = 0;
    size_t totalOpaqueIndices = 0;

    // First pass: Opaque - OPTIMIZED VERSION
    for (auto *chunk : chunks) {
      Vector3 chunkPosKey = {(float)chunk->xPos / 16, 0,
                             (float)chunk->zPos / 16};

      if (chunk->needsMeshUpdate ||
          opaqueMeshCache.find(chunkPosKey) == opaqueMeshCache.end()) {
        auto &cache = opaqueMeshCache[chunkPosKey];
        cache.vertices.clear();
        cache.indices.clear();

        // OPTIMIZATION 2: Count opaque faces first to reserve exact size
        size_t opaqueCount = 0;
        for (auto &face : chunk->allFaces) {
          if (!face.Transparent)
            opaqueCount++;
        }
        cache.vertices.reserve(opaqueCount * 4);
        cache.indices.reserve(opaqueCount * 6);

        // OPTIMIZATION 3: Pre-calculate chunk world position once
        Vector3 chunkWorldPos{(float)chunk->xPos, 0, (float)chunk->zPos};

        // OPTIMIZATION 4: Process faces in single loop
        for (auto &face : chunk->allFaces) {
          if (face.Transparent)
            continue;

          // Pre-calculate block world position
          Vector3 worldPos = getCoordinates(face.blockPos) + chunkWorldPos;

          // Pre-calculate color once per face
          Vector3 faceColor = BlockDef[face.blockID].color.ToFloat() -
                              Colors[(int)(face.side / 2)].ToFloat();

          Uint32 baseV = cache.vertices.size();

          // Scaled vertices based on greedy mesh width/height
          Vector3 v0 = Verts[face.side][0];
          Vector3 v1 = Verts[face.side][1];
          Vector3 v2 = Verts[face.side][2];
          Vector3 v3 = Verts[face.side][3];

          float fw = (float)face.w;
          float fh = (float)face.h;

          // Scale based on side orientation
          if (face.side < 2) { // Front/Back (+-Z): Width=X, Height=Y
            v0.x *= fw;
            v0.y *= fh;
            v1.x *= fw;
            v1.y *= fh;
            v2.x *= fw;
            v2.y *= fh;
            v3.x *= fw;
            v3.y *= fh;
          } else if (face.side < 4) { // Right/Left (+-X): Width=Z, Height=Y
            v0.z *= fw;
            v0.y *= fh;
            v1.z *= fw;
            v1.y *= fh;
            v2.z *= fw;
            v2.y *= fh;
            v3.z *= fw;
            v3.y *= fh;
          } else { // Top/Bottom (+-Y): Width=X, Height=Z
            v0.x *= fw;
            v0.z *= fh;
            v1.x *= fw;
            v1.z *= fh;
            v2.x *= fw;
            v2.z *= fh;
            v3.x *= fw;
            v3.z *= fh;
          }

          float bid = (float)face.blockID;

          Vector3 v[4] = {v0, v1, v2, v3};
          for (int j = 0; j < 4; j++) {
            SDL_FPoint uv;
            if (face.side < 2) { // Front/Back: X, Y
              uv = {Verts[face.side][j].x * fw, Verts[face.side][j].y * fh};
            } else if (face.side < 4) { // Right/Left: Z, Y
              uv = {Verts[face.side][j].z * fw, Verts[face.side][j].y * fh};
            } else { // Top/Bottom: X, Z
              uv = {Verts[face.side][j].x * fw, Verts[face.side][j].z * fh};
            }
            cache.vertices.push_back({v[j] + worldPos, faceColor, uv, bid});
          }
          cache.indices.insert(cache.indices.end(),
                               {baseV + 0, baseV + 2, baseV + 1, baseV + 1,
                                baseV + 2, baseV + 3});
        }
        chunk->needsMeshUpdate = false;
      }

      auto &cache = opaqueMeshCache[chunkPosKey];
      if (!cache.vertices.empty()) {
        totalOpaqueVertices += cache.vertices.size();
        totalOpaqueIndices += cache.indices.size();
      }
    }

    // OPTIMIZATION 7: Batch copy all opaque geometry at once
    size_t currentVertexOffset = 0;
    size_t currentIndexOffset = 0;

    for (auto *chunk : chunks) {
      Vector3 chunkPosKey = {(float)chunk->xPos / 16, 0,
                             (float)chunk->zPos / 16};
      auto &cache = opaqueMeshCache[chunkPosKey];

      if (cache.vertices.empty())
        continue;

      // OPTIMIZATION 8: Use memcpy for bulk vertex data
      memcpy(&Vertexdata[currentVertexOffset], cache.vertices.data(),
             cache.vertices.size() * sizeof(Vertex));

      // OPTIMIZATION 9: Batch index offset calculation
      Uint32 vertexBase = currentVertexOffset;
      for (size_t i = 0; i < cache.indices.size(); i++) {
        Indexdata[currentIndexOffset + i] = cache.indices[i] + vertexBase;
      }

      currentVertexOffset += cache.vertices.size();
      currentIndexOffset += cache.indices.size();
    }

    mesh->BaseVertex = currentVertexOffset;
    mesh->BaseIndex = currentIndexOffset;
    mesh->OpaqueIndexCount = mesh->BaseIndex;

    // Second pass: Transparent (unchanged for now)
    std::vector<TransparentDrawnFace> transparentFaces;
    for (auto *chunk : chunks) {
      for (auto &face : chunk->allFaces) {
        if (face.Transparent) {
          const TransparentDrawnFace TransparentFace{
              getCoordinates(face.blockPos) +
                  Vector3{(float)chunk->xPos, 0, (float)chunk->zPos},
              face.side, face.blockID, face.w, face.h};
          transparentFaces.push_back(TransparentFace);
        }
      }
    }

    // Sort transparent faces BACK TO FRONT (furthest first)
    std::sort(
        transparentFaces.begin(), transparentFaces.end(),
        [&](const TransparentDrawnFace &a, const TransparentDrawnFace &b) {
          float distA = (a.blockPos - player.Position).LengthSquared();
          float distB = (b.blockPos - player.Position).LengthSquared();
          return distA > distB;
        });

    for (auto &face : transparentFaces) {
      Vector3 WorldVerts[4];
      Vector3 v0 = Verts[face.side][0];
      Vector3 v1 = Verts[face.side][1];
      Vector3 v2 = Verts[face.side][2];
      Vector3 v3 = Verts[face.side][3];
      float fw = (float)face.w;
      float fh = (float)face.h;

      if (face.side < 2) { // Front/Back (+-Z): X, Y
        v0.x *= fw;
        v0.y *= fh;
        v1.x *= fw;
        v1.y *= fh;
        v2.x *= fw;
        v2.y *= fh;
        v3.x *= fw;
        v3.y *= fh;
      } else if (face.side < 4) { // Right/Left (+-X): Z, Y
        v0.z *= fw;
        v0.y *= fh;
        v1.z *= fw;
        v1.y *= fh;
        v2.z *= fw;
        v2.y *= fh;
        v3.z *= fw;
        v3.y *= fh;
      } else { // Top/Bottom (+-Y): X, Z
        v0.x *= fw;
        v0.z *= fh;
        v1.x *= fw;
        v1.z *= fh;
        v2.x *= fw;
        v2.z *= fh;
        v3.x *= fw;
        v3.z *= fh;
      }

      WorldVerts[0] = v0 + face.blockPos;
      WorldVerts[1] = v1 + face.blockPos;
      WorldVerts[2] = v2 + face.blockPos;
      WorldVerts[3] = v3 + face.blockPos;

      Vector3 faceColor = BlockDef[face.blockID].color.ToFloat() -
                          Colors[(int)(face.side / 2)].ToFloat();
      float bid = (float)face.blockID;

      Vector3 v[4] = {v0, v1, v2, v3};
      for (int j = 0; j < 4; j++) {
        SDL_FPoint uv;
        if (face.side < 2) { // Front/Back: X, Y
          uv = {Verts[face.side][j].x * fw, Verts[face.side][j].y * fh};
        } else if (face.side < 4) { // Right/Left: Z, Y
          uv = {Verts[face.side][j].z * fw, Verts[face.side][j].y * fh};
        } else { // Top/Bottom: X, Z
          uv = {Verts[face.side][j].x * fw, Verts[face.side][j].z * fh};
        }
        Vertexdata[mesh->BaseVertex + j] = {v[j] + face.blockPos, faceColor, uv,
                                            bid};
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

  // Transform frustum to world space based on player position and rotation
  // Create a fresh frustum in camera space
  float frustumAspect =
      (float)this->basicInitVars.Width / (float)this->basicInitVars.Height;
  float tanHalfFov = tan(FOV * PI / 360.0f);
  Frustum worldFrustum =
      Frustum::createFrustumFromCamera(frustumAspect, tanHalfFov, Znear, Zfar);

  // Transform to world space using player's position and rotation
  worldFrustum.transformToWorldSpace(players[0].Position, players[0].Rotation);

  // Store the world-space frustum for use in DrawTerrain
  this->frustum = worldFrustum;

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

  this->runTimeRenderVars.pass =
      SDL_BeginGPURenderPass(this->runTimeRenderVars.cmdRender,
                             &color_target_info, 1, &depth_target_info);

  // Bind Texture Atlas and Sampler
  SDL_GPUTextureSamplerBinding samplerBinding = {this->TextureAtlas,
                                                 this->Sampler};
  SDL_BindGPUFragmentSamplers(this->runTimeRenderVars.pass, 0, &samplerBinding,
                              1);

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

  SDL_EndGPURenderPass(this->runTimeRenderVars.pass);

  DrawUI(this->runTimeRenderVars.cmdRender, swap_texture);

  if (!SDL_SubmitGPUCommandBuffer(this->runTimeRenderVars.cmdRender)) {
    std::cout << "Failed to submit render command buffer\n";
    return;
  }
}
void Renderer::DrawUI(SDL_GPUCommandBuffer *cmd, SDL_GPUTexture *swap_texture) {
  float size = 0.005f;
  float aspect =
      (float)this->basicInitVars.Width / (float)this->basicInitVars.Height;
  
  // Apply aspect ratio correction to horizontal dimensions
  float sizeX = size * 5 / aspect;  // Divide by aspect to compensate
  float sizeY = size;
  
  Vertex uiVertices[12] = {
      // Horizontal crosshair line
      {{-sizeX, -sizeY, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}, 0},
      {{sizeX, -sizeY, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}, 0},
      {{-sizeX, sizeY, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}, 0},
      {{sizeX, -sizeY, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}, 0},
      {{sizeX, sizeY, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}, 0},
      {{-sizeX, sizeY, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}, 0},
      // Vertical crosshair line
      {{-sizeY / aspect, -sizeX * aspect, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}, 0},
      {{sizeY / aspect, -sizeX * aspect, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}, 0},
      {{-sizeY / aspect, sizeX * aspect, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}, 0},
      {{sizeY / aspect, -sizeX * aspect, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}, 0},
      {{sizeY / aspect, sizeX * aspect, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}, 0},
      {{-sizeY / aspect, sizeX * aspect, 0.0f}, {1.0f, 1.0f, 1.0f}, {0, 0}, 0},
  };
  
  // Upload vertex data to GPU
  void *mapData = SDL_MapGPUTransferBuffer(this->basicInitVars.GPU,
                                           this->UITransferBuffer, true);
  SDL_memcpy(mapData, uiVertices, sizeof(uiVertices));
  SDL_UnmapGPUTransferBuffer(this->basicInitVars.GPU, this->UITransferBuffer);
  
  // Copy data to GPU buffer
  SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(cmd);
  SDL_GPUTransferBufferLocation src = {this->UITransferBuffer, 0};
  SDL_GPUBufferRegion dst = {this->UIBuffer, 0, sizeof(uiVertices)};
  SDL_UploadToGPUBuffer(copyPass, &src, &dst, true);
  SDL_EndGPUCopyPass(copyPass);
  
  // Setup color target to load existing content (don't clear)
  SDL_GPUColorTargetInfo color_target_info;
  SDL_zero(color_target_info);
  color_target_info.texture = swap_texture;
  color_target_info.load_op = SDL_GPU_LOADOP_LOAD;
  color_target_info.store_op = SDL_GPU_STOREOP_STORE;
  
  // Begin UI render pass
  SDL_GPURenderPass *uiPass =
      SDL_BeginGPURenderPass(cmd, &color_target_info, 1, nullptr);
  
  // Bind UI pipeline and draw crosshair
  SDL_BindGPUGraphicsPipeline(uiPass, this->pipelineInitVars.uiPipeline);
  SDL_GPUBufferBinding vBinding = {this->UIBuffer, 0};
  SDL_BindGPUVertexBuffers(uiPass, 0, &vBinding, 1);
  SDL_DrawGPUPrimitives(uiPass, 12, 1, 0, 0);
  SDL_EndGPURenderPass(uiPass);
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
    SDL_Log("SDL basicInitVars.GPU creation failed: %s", SDL_GetError());
    return;
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
  int totalChunks =
      (int)((float)(RenderDistance * 2 + 1) * (RenderDistance * 2 + 1) / 2.0f);
  for (int i = std::sqrt(totalChunks) + 1; i > 0; i--) {
    if (totalChunks % i) {
      chunksPerBuffer = i;
      break;
    }
  }
  this->totalBuffers =
      (totalChunks + chunksPerBuffer) / chunksPerBuffer + 1; // Ceiling division

  // Each buffer now holds multiple chunks
  constexpr Uint32 singleChunkVertexSize = sizeof(Vertex) * 4 * 1200;
  constexpr Uint32 singleChunkIndexSize = sizeof(Uint32) * 6 * 1200;
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
}
void Renderer::LoadTexture() {
  if (!this->basicInitVars.GPU) {
    SDL_Log("Cannot load texture: GPU device not initialized");
    return;
  }

  const char *basePath = SDL_GetBasePath();
  char fullPath[512];
  if (basePath) {
    SDL_snprintf(fullPath, sizeof(fullPath), "%sassets/Textures/Dirt.png",
                 basePath);
  } else {
    SDL_strlcpy(fullPath, "assets/Textures/Dirt.png", sizeof(fullPath));
  }

  SDL_Surface *surface = IMG_Load(fullPath);
  if (!surface) {
    SDL_Log("Failed to load texture at %s: %s", fullPath, SDL_GetError());
    return;
  }

  // Convert to RGBA32 (R, G, B, A in byte order)
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
  SDL_GPUTransferBufferCreateInfo transferInfo = {};
  transferInfo.size = (Uint32)(surface->w * surface->h * 4);
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
    SDL_memcpy(data, surface->pixels, (size_t)transferInfo.size);
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
      LoadShader(this->basicInitVars.GPU, "Shader.frag", 1, 0, 0, 0);
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
  pipelineInitVars.pipeline_desc.vertex_input_state.num_vertex_attributes = 4;
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
      LoadShader(this->basicInitVars.GPU, "UI.frag", 0, 0, 0, 0);

  SDL_GPUGraphicsPipelineCreateInfo ui_desc =
      this->pipelineInitVars.pipeline_desc;
  ui_desc.vertex_shader = ui_vert;
  ui_desc.fragment_shader = ui_frag;
  ui_desc.depth_stencil_state.enable_depth_test = false;
  ui_desc.depth_stencil_state.enable_depth_write = false;
  ui_desc.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
  ui_desc.target_info.has_depth_stencil_target = false;

  ui_desc.vertex_input_state.num_vertex_attributes =
      2; // Only Position and Color

  this->pipelineInitVars.uiPipeline =
      SDL_CreateGPUGraphicsPipeline(this->basicInitVars.GPU, &ui_desc);

  SDL_ReleaseGPUShader(this->basicInitVars.GPU, ui_vert);
  SDL_ReleaseGPUShader(this->basicInitVars.GPU, ui_frag);

  // Create UI Buffer
  SDL_GPUBufferCreateInfo uiBufferInfo = {};
  uiBufferInfo.size = sizeof(Vertex) * 12;
  uiBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  this->UIBuffer = SDL_CreateGPUBuffer(this->basicInitVars.GPU, &uiBufferInfo);

  SDL_GPUTransferBufferCreateInfo uiTransferInfo = {};
  uiTransferInfo.size = sizeof(Vertex) * 12;
  uiTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  this->UITransferBuffer =
      SDL_CreateGPUTransferBuffer(this->basicInitVars.GPU, &uiTransferInfo);
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