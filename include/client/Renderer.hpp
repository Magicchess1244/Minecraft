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
};
struct Vertex {
  Vector3 Position;
  // float pad1;  // padding to 16 bytes
  Vector3 Color;
  // float pad2;  // padding to 16 bytes
};
struct Mesh {
  SDL_GPUTransferBuffer *VertextransferBuffer = nullptr;
  SDL_GPUTransferBuffer *IndextransferBuffer = nullptr;
  SDL_GPUBufferBinding VertexBuffer;
  SDL_GPUBufferBinding IndexBuffer;
  std::vector<DrawnFace> Faces;
  std::vector<int> chunkFaceCounts; // Face count for each chunk in this buffer
  int faces;                        // Total faces in buffer (for compatibility)

  // Temporary pointers for mapped buffers during DrawTerrain
  Vertex *mappedVertexData = nullptr;
  Uint32 *mappedIndexData = nullptr;
};
class GameClient;
class SpiralIterator {
private:
    int size;
    std::vector<std::pair<int, int>> path;
    int currentIndex;
    
    void generatePath() {
        path.clear();
        
        int halfSize = size / 2;
        int x = -halfSize;
        int y = -halfSize;
        
        path.push_back({x, y});
        
        int n = size - 1;
        
        // Right n steps
        for (int i = 0; i < n; i++) {
            x++;
            path.push_back({x, y});
        }
        
        // Up n steps
        for (int i = 0; i < n; i++) {
            y++;
            path.push_back({x, y});
        }
        
        // Left n steps
        for (int i = 0; i < n; i++) {
            x--;
            path.push_back({x, y});
        }
        
        // Down n-1 steps
        for (int i = 0; i < n - 1; i++) {
            y--;
            path.push_back({x, y});
        }
        
        // Continue spiraling inward
        n -= 2;
        
        while (n > 0) {
            // Right n steps
            for (int i = 0; i < n; i++) {
                x++;
                path.push_back({x, y});
            }
            
            // Up n steps
            for (int i = 0; i < n; i++) {
                y++;
                path.push_back({x, y});
            }
            
            // Left n steps
            for (int i = 0; i < n; i++) {
                x--;
                path.push_back({x, y});
            }
            
            // Down n steps
            for (int i = 0; i < n; i++) {
                y--;
                path.push_back({x, y});
            }
            
            n -= 2;
        }
        
        // Final step to center if needed
        if (path.size() < size * size) {
            x++;
            path.push_back({x, y});
        }
    }
    
public:
    SpiralIterator(int gridSize) : size(gridSize), currentIndex(0) {
        generatePath();
    }
    
    void reset() {
        currentIndex = 0;
    }
    
    std::pair<int, int> next() {
        if (currentIndex < path.size()) {
            return path[currentIndex++];
        }
        return {0, 0};
    }
    
    bool hasNext() {
        return currentIndex < path.size();
    }
};


class Renderer {
private:
  SDL_Window *window = nullptr;
  SDL_GPUDevice *GPU = nullptr;
  SDL_GPURenderPass *pass = nullptr;
  SDL_Event event;
  TTF_Font *font = nullptr;
  SDL_GPUTexture *DepthTexture = nullptr;
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
  SDL_GPUCopyPass *copyPass = nullptr;

  // Buffer packing optimization
  int chunksPerBuffer = 3; // How many chunks fit in one buffer
  int totalBuffers = 0;    // Total number of GPU buffers allocated

  SDL_FPoint getUV(int tileIndex, int cornerX, int cornerY);
  Vector3 rotate(const Vector3 &pos, const Vector3 &Angle);
  void DrawFace(Player &player, Vector3 blocks, int blockID, int Side,
                Mesh *mesh, Vertex *Vertexdata, Uint32 *Indexdata);
  SDL_GPUTexture *CreateDepthTexture(Uint32 drawablew, Uint32 drawableh);
  void UpdateViewportAndProjection();
  void DrawTestTriangle(Mesh *mesh, Vertex *Vertexdata, Uint32 *Indexdata);

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
  void RenderChunk(ChunkPrefab &chunk, Player &player, int chunkIndex,
                   int bufferIndex, int bufferOffset);
  void DrawTerrain(Player &player);
  void MainRenderLoop(std::vector<Slot> &inventory, int inventorySlot,
                      std::vector<Player> &players);
};