#pragma once

#include "../common/Common.hpp"

#include "../common/Chunck.hpp"
#include "../common/ChunkManager.hpp"
#include <SDL3/SDL_gpu.h>

struct Plane {
  Vector3 normal{0.f, 1.f, 0.f}; // must be normalized
  float distance = 0.f;          // the "d" in ax+by+cz+d=0

  // signed distance = n·p + d
  float getSignedDistanceToPlane(const Vector3 &point) const {
    return normal.Dot(point) + distance;
  }
};
struct Frustum {
  Plane topFace, bottomFace, rightFace, leftFace, farFace, nearFace;

  // Creates a frustum in camera space. Camera is at origin looking +Z, Y up, X
  // right.
  static Frustum createFrustumFromCamera(float aspect, float fovY_radians,
                                         float Znear, float Zfar) {
    Frustum frustum;

    // half sizes at far plane (use tan(fov/2))
    const float halfVSide = Zfar * fovY_radians;
    const float halfHSide = halfVSide * aspect;

    Vector3 camForward = {0.f, 0.f, 1.f};
    Vector3 camRight = {1.f, 0.f, 0.f};
    Vector3 camUp = {0.f, 1.f, 0.f};

    Vector3 nearCenter = camForward * Znear;
    Vector3 farCenter = camForward * Zfar;

    // Near plane (points toward +Z)
    frustum.nearFace.normal = camForward; // (0,0,1)
    frustum.nearFace.distance =
        -frustum.nearFace.normal.Dot(nearCenter); // -Znear

    // Far plane (points toward -Z)
    frustum.farFace.normal = camForward * -1;
    frustum.farFace.distance = -frustum.farFace.normal.Dot(farCenter);

    // Right plane: use right edge direction and ensure normal points inward
    {
      Vector3 rightEdge =
          (farCenter + camRight * halfHSide).Normalized(); // dir to far-right
      frustum.rightFace.normal =
          (rightEdge.Cross(camUp)).Normalized(); // inward
      frustum.rightFace.distance = 0.0f; // plane goes through origin (camera)
    }

    // Left plane
    {
      Vector3 leftEdge = (farCenter - camRight * halfHSide).Normalized();
      frustum.leftFace.normal = (camUp.Cross(leftEdge)).Normalized(); // inward
      frustum.leftFace.distance = 0.0f;
    }

    // Top plane
    {
      Vector3 topEdge = (farCenter + camUp * halfVSide).Normalized();
      frustum.topFace.normal = (camRight.Cross(topEdge)).Normalized(); // inward
      frustum.topFace.distance = 0.0f;
    }

    // Bottom plane
    {
      Vector3 bottomEdge = (farCenter - camUp * halfVSide).Normalized();
      frustum.bottomFace.normal =
          (bottomEdge.Cross(camRight)).Normalized(); // inward
      frustum.bottomFace.distance = 0.0f;
    }

    return frustum;
  }

  bool IsBoxVisible(const Vector3 &min, const Vector3 &max) const {
    const Plane *planes[6] = {&topFace,  &bottomFace, &rightFace,
                              &leftFace, &farFace,    &nearFace};
    for (int i = 0; i < 6; ++i) {
      const Plane &p = *planes[i];
      Vector3 pVertex = min;
      if (p.normal.x >= 0)
        pVertex.x = max.x;
      if (p.normal.y >= 0)
        pVertex.y = max.y;
      if (p.normal.z >= 0)
        pVertex.z = max.z;

      if (p.getSignedDistanceToPlane(pVertex) < 0) {
        return false;
      }
    }
    return true;
  }
};
struct Mesh {
  SDL_GPUTransferBuffer *VertextransferBuffer = nullptr;
  SDL_GPUTransferBuffer *IndextransferBuffer = nullptr;
  SDL_GPUBufferBinding VertexBuffer;
  SDL_GPUBufferBinding IndexBuffer;
  std::vector<DrawnFace> Faces;
  int faces;
  bool dirty = true;
  int x = -999999;
  int z = -999999;
};
struct Vertex {
  Vector3 Position;
  // float pad1;  // padding to 16 bytes
  Vector3 Color;
  // float pad2;  // padding to 16 bytes
  Vector2 UV;
  Vector2 Pad; // Padding to align to 16 bytes or just to keep it clean.
               // Position (12) + Color (12) + UV (8) = 32 bytes.
  // Actually Position is 3 floats (12), Color is 3 floats (12). Total 24.
  // UV is 2 floats (8). Total 32. Perfect alignment!
};
class GameClient;

class Renderer {
private:
  SDL_Window *window = nullptr;
  SDL_GPUDevice *GPU = nullptr;
  SDL_GPURenderPass *pass = nullptr;
  SDL_Event event;
  TTF_Font *font = nullptr;
  SDL_GPUTexture *DepthTexture = nullptr;
  SDL_GPUTexture *AtlasTexture = nullptr;
  SDL_GPUSampler *TextureSampler = nullptr;
  Vector3 ScreenSize = {0, 0, 0};
  ChunkManager chunkManager;
  GameClient &gameClient;
  int BlockPixelSize = 50;
  bool fullScreen = false;
  Frustum frustum;
  unsigned int Width, Height;
  SDL_GPUCommandBuffer *cmdRender = nullptr;
  SDL_GPUCommandBuffer *cmdCopy = nullptr;
  std::vector<Mesh> Terrain;
  SDL_GPUGraphicsPipeline *graphicsPipeline = nullptr;
  SDL_GPUGraphicsPipeline *uiPipeline = nullptr; // New UI pipeline
  SDL_GPUCopyPass *copyPass = nullptr;
  SDL_GPUTexture *whiteTexture = nullptr; // For colored quads
  Mesh uiMesh;                            // Mesh for UI elements

  // UI State
  SDL_GPUTexture *fpsTexture = nullptr;
  SDL_GPUTexture *coordsTexture = nullptr;
  int hotbarIndexStart = 0, hotbarIndexCount = 0;
  int fpsIndexStart = 0, fpsIndexCount = 0;
  int coordsIndexStart = 0, coordsIndexCount = 0;

  SDL_FPoint getUV(int tileIndex, int cornerX, int cornerY);
  Vector3 rotate(const Vector3 &pos, const Vector3 &Angle);
  void DrawFace(Player &player, Vector3 blocks, int blockID, int Side,
                Mesh *mesh, Vertex *Vertexdata, Uint32 *Indexdata);
  SDL_GPUTexture *CreateDepthTexture(Uint32 drawablew, Uint32 drawableh);
  SDL_GPUTexture *CreateTextureFromBMP(const char *filename);
  SDL_GPUTexture *CreateTextureFromSurface(SDL_GPUCopyPass *copyPass,
                                           SDL_Surface *surface);
  void UpdateViewportAndProjection();
  void DrawTestTriangle(Mesh *mesh, Vertex *Vertexdata, Uint32 *Indexdata);
  void InitMeshBuffers(Mesh &mesh);

  // UI Helpers
  void DrawRect(float x, float y, float w, float h, Color color,
                Vertex *vertices, Uint32 *indices, int &vCount, int &iCount);
  void UploadUI(SDL_GPUCopyPass *copyPass, Player &player, int inventorySlot,
                float fps);
  void DrawUIPass(SDL_GPURenderPass *pass);

public:
  Renderer(GameClient &gameClient);
  ~Renderer() {
    for (auto &mesh : this->Terrain) {
      SDL_ReleaseGPUBuffer(this->GPU, mesh.IndexBuffer.buffer);
      SDL_ReleaseGPUBuffer(this->GPU, mesh.VertexBuffer.buffer);
      SDL_ReleaseGPUTransferBuffer(this->GPU, mesh.IndextransferBuffer);
      SDL_ReleaseGPUTransferBuffer(this->GPU, mesh.VertextransferBuffer);
    }

    SDL_ReleaseGPUGraphicsPipeline(this->GPU, graphicsPipeline);
    if (GPU) {
      SDL_DestroyGPUDevice(GPU);
      GPU = nullptr;
    }
    if (pass) {
      pass = nullptr;
    }
    if (window) {
      SDL_DestroyWindow(window);
      window = nullptr;
    }
    if (font) {
      TTF_CloseFont(font);
      font = nullptr;
    }
    if (DepthTexture) {
      DepthTexture = nullptr;
    }
    event = {};
    cmdCopy = nullptr;
    cmdRender = nullptr;
  };

  void Stats(Player &player);
  void RenderChunk(ChunkPrefab &chunk, Player &player, int NumChunk);
  void DrawTerrain(Player &player);
  void MainRenderLoop(std::vector<Slot> &inventory, int inventorySlot,
                      std::vector<Player> &players);
};