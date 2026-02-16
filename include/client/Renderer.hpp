#pragma once

#include "../common/Common.hpp"

#include "../common/Chunck.hpp"
#include "../common/ChunkManager.hpp"
#include <SDL3/SDL_gpu.h>
#include <SDL3_image/SDL_image.h>
#include <vector>

class SpiralIterator {
private:
  int size;
  std::vector<std::pair<int, int>> path;
  int currentIndex;

public:
  SpiralIterator(int gridSize) : size(gridSize), currentIndex(0) {
    generatePath();
  }

  void reset() { currentIndex = 0; }

  std::pair<int, int> next() {
    if (currentIndex < path.size()) {
      return path[currentIndex++];
    }
    return {0, 0};
  }

  void generatePath() {
    path.clear();
    int x = 0, y = 0;
    int dx = 1, dy = 0; // Start moving RIGHT
    int steps = 1;
    int stepCount = 0;
    int turnsInCurrentSize = 0;

    path.push_back({x, y}); // Add starting position

    int maxSteps = size * size;
    for (int i = 1; i < maxSteps; i++) {
      x += dx;
      y += dy;
      path.push_back({x, y});

      stepCount++;

      // Time to turn?
      if (stepCount == steps) {
        stepCount = 0;
        turnsInCurrentSize++;

        // Turn right (clockwise): (dx, dy) -> (-dy, dx)
        int temp = dx;
        dx = -dy;
        dy = temp;

        // Increase step size after every 2 turns
        if (turnsInCurrentSize == 2) {
          steps++;
          turnsInCurrentSize = 0;
        }
      }
    }
  }

  bool hasNext() { return currentIndex < path.size(); }
};
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
  static Frustum createFrustumFromCamera(float aspect, float tanHalfFov,
                                         float Znear, float Zfar) {
    Frustum frustum;

    // half sizes at far plane
    const float halfVSide = Zfar * tanHalfFov;
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

  // Check if an axis-aligned bounding box (AABB) is inside or intersecting the
  // frustum Uses the "p-vertex/n-vertex" test for each plane
bool isChunkInFrustum(const Vector3 &minPoint,
                      const Vector3 &maxPoint, 
                      float tolerance = 5.0f) const {
    
    // Get all 8 corners of the AABB
    const Vector3 corners[8] = {
        {minPoint.x, minPoint.y, minPoint.z},
        {maxPoint.x, minPoint.y, minPoint.z},
        {minPoint.x, maxPoint.y, minPoint.z},
        {maxPoint.x, maxPoint.y, minPoint.z},
        {minPoint.x, minPoint.y, maxPoint.z},
        {maxPoint.x, minPoint.y, maxPoint.z},
        {minPoint.x, maxPoint.y, maxPoint.z},
        {maxPoint.x, maxPoint.y, maxPoint.z}
    };
    
    // Check all 6 frustum planes
    const Plane* const planes[6] = {
        &nearFace, &farFace, &leftFace,
        &rightFace, &topFace, &bottomFace
    };
    
    // For each plane, check if ALL corners are outside
    for (int p = 0; p < 6; ++p) {
        bool allOutside = true;
        for (int i = 0; i < 8; ++i) {
            // If ANY corner is inside (or within tolerance), chunk might be visible
            if (planes[p]->getSignedDistanceToPlane(corners[i]) >= -tolerance) {
                allOutside = false;
                break; // Early exit - found a corner inside this plane
            }
        }
        // If all corners are outside ANY single plane, the chunk is completely outside
        if (allOutside) {
            return false;
        }
    }
    return true; // Chunk intersects or is inside the frustum
}

  // Transform frustum from camera space to world space
  // This rotates and translates the frustum to match the camera's position and
  // orientation
  void transformToWorldSpace(const Vector3 &cameraPosition,
                             const Vector3 &cameraRotation) {
    // Calculate rotation angles
    Vector3 rot = cameraRotation.AngleToRadians();
    float cosY = cos(rot.y), sinY = sin(rot.y);
    float cosX = cos(rot.x), sinX = sin(rot.x);

    // Create rotation matrix (Y * X order, same as camera)
    // This is the inverse of the view rotation (transpose of the view's
    // rotation part)
    auto rotateNormal = [&](const Vector3 &n) -> Vector3 {
      // First rotate around X axis
      Vector3 temp;
      temp.x = n.x;
      temp.y = n.y * cosX - n.z * sinX;
      temp.z = n.y * sinX + n.z * cosX;

      // Then rotate around Y axis
      Vector3 result;
      result.x = temp.x * cosY + temp.z * sinY;
      result.y = temp.y;
      result.z = -temp.x * sinY + temp.z * cosY;

      return result;
    };

    // Transform each plane
    auto transformPlane = [&](Plane &plane) {
      // Rotate the normal
      plane.normal = rotateNormal(plane.normal);

      // Update distance: d' = d - n'·p
      // where n' is the rotated normal and p is the camera position
      plane.distance = plane.distance - plane.normal.Dot(cameraPosition);
    };

    transformPlane(nearFace);
    transformPlane(farFace);
    transformPlane(leftFace);
    transformPlane(rightFace);
    transformPlane(topFace);
    transformPlane(bottomFace);
  }
};
struct Vertex {
  Vector3 Position;
  Vector3 Color;
  SDL_FPoint UV;
  float BlockID;
  float LightLevel = 15;
};
struct Mesh {
  SDL_GPUTransferBuffer *VertextransferBuffer = nullptr;
  SDL_GPUTransferBuffer *IndextransferBuffer = nullptr;
  SDL_GPUBufferBinding VertexBuffer;
  SDL_GPUBufferBinding IndexBuffer;
  std::vector<DrawnFace> Faces;
  int BaseVertex = 0, BaseIndex = 0;

  // Temporary pointers for mapped buffers during DrawTerrain
  Vertex *mappedVertexData = nullptr;
  Uint32 *mappedIndexData = nullptr;

  int OpaqueIndexCount = 0;
  int TransparentIndexCount = 0;
};
struct ChunkDistance {
  ChunkPrefab *chunk;
  float distSq;
};
struct BasicInitVars {
  SDL_Window *window = nullptr;
  SDL_GPUDevice *GPU = nullptr;
  SDL_Event event;
  unsigned int Width, Height;
};
struct PipileInitVars {
  SDL_GPUGraphicsPipeline *graphicsPipeline = nullptr;
  SDL_GPUGraphicsPipeline *transparentPipeline = nullptr;
  SDL_GPUGraphicsPipeline *uiPipeline = nullptr;
  SDL_GPUVertexBufferDescription vertex_buffer_desc;
  SDL_GPUVertexAttribute vertex_attributes[5];
  SDL_GPUGraphicsPipelineCreateInfo pipeline_desc;
  SDL_GPUShader *vertex_shader;
  SDL_GPUShader *fragment_shader;
  SDL_GPUColorTargetDescription colorTargetDescriptions[1];
};
struct RunTimeRenderVars {
  SDL_GPURenderPass *pass = nullptr;
  SDL_GPUCopyPass *copyPass = nullptr;
  SDL_GPUCommandBuffer *cmdRender = nullptr;
  SDL_GPUCommandBuffer *cmdCopy = nullptr;
  SDL_GPUTexture *swap_texture = nullptr;
  float aspect = 0;
};
class GameClient;

struct CachedChunkMesh {
  std::vector<Vertex> vertices;
  std::vector<Uint32> indices;
};
struct UIVars{
  std::vector<Vertex> uiVertices;
  SDL_GPUBuffer *UIVertexBuffer = nullptr;
  SDL_GPUBuffer *UIIndexBuffer = nullptr;
  SDL_GPUTransferBuffer *UIVertexTransferBuffer = nullptr;
  SDL_GPUTransferBuffer *UIIndexTransferBuffer = nullptr;
};
class Renderer {
private:
  std::unordered_map<Vector3, CachedChunkMesh> opaqueMeshCache;
  BasicInitVars basicInitVars;
  PipileInitVars pipelineInitVars;
  RunTimeRenderVars runTimeRenderVars;
  SDL_GPUTexture *DepthTexture = nullptr;
  SDL_GPUTexture *TextureAtlas = nullptr;
  SDL_GPUSampler *Sampler = nullptr;
  UIVars uiVars;
  SDL_GPUBuffer *EntityBuffer = nullptr;
  SDL_GPUTransferBuffer *EntityTransferBuffer = nullptr;
  SDL_GPUBuffer *EntityIndexBuffer = nullptr;
  SDL_GPUTransferBuffer *EntityIndexTransferBuffer = nullptr;
  ChunkManager &chunkManager;
  GameClient &gameClient;
  bool fullScreen = false;
  Frustum frustum;
  std::vector<Mesh> Terrain;
  int chunksPerBuffer = 25, totalBuffers = 0;

  auto AddRect(float x, float y, float w, float h, Vector3 color, float blockID = 0);
  void UICrossHair();
  void UIInventory(const std::vector<Slot> &inventory, int inventorySlot);
  std::vector<ChunkDistance> SortChunks(Player &player);
  void DrawTerrain(Player &player);
  SDL_GPUTexture *CreateDepthTexture(Uint32 drawablew, Uint32 drawableh);
  void UpdateViewportAndProjection();
  void Init();
  void GenerateBuffer();
  void VertexGPUInit();
  void PipelineInit();
  void ColorTargetDes();
  void LoadTexture();
  void EventManager(Player &player, int &inventorySlot);

public:
  Renderer(GameClient &gameClient, ChunkManager &manager);
  ~Renderer() {
    for (auto &mesh : this->Terrain) {
      SDL_ReleaseGPUBuffer(this->basicInitVars.GPU, mesh.IndexBuffer.buffer);
      SDL_ReleaseGPUBuffer(this->basicInitVars.GPU, mesh.VertexBuffer.buffer);
      SDL_ReleaseGPUTransferBuffer(this->basicInitVars.GPU,
                                   mesh.IndextransferBuffer);
      SDL_ReleaseGPUTransferBuffer(this->basicInitVars.GPU,
                                   mesh.VertextransferBuffer);
    }

    if (pipelineInitVars.graphicsPipeline) {
      SDL_ReleaseGPUGraphicsPipeline(this->basicInitVars.GPU,
                                     pipelineInitVars.graphicsPipeline);
    }
    if (pipelineInitVars.transparentPipeline) {
      SDL_ReleaseGPUGraphicsPipeline(this->basicInitVars.GPU,
                                     pipelineInitVars.transparentPipeline);
    }
    if (pipelineInitVars.uiPipeline) {
      SDL_ReleaseGPUGraphicsPipeline(this->basicInitVars.GPU,
                                     pipelineInitVars.uiPipeline);
    }

    if (TextureAtlas) {
      SDL_ReleaseGPUTexture(this->basicInitVars.GPU, TextureAtlas);
      TextureAtlas = nullptr;
    }
    if (Sampler) {
      SDL_ReleaseGPUSampler(this->basicInitVars.GPU, Sampler);
      Sampler = nullptr;
    }
    if (this->uiVars.UIVertexBuffer) {
      SDL_ReleaseGPUBuffer(this->basicInitVars.GPU, this->uiVars.UIVertexBuffer);
      this->uiVars.UIVertexBuffer = nullptr;
    }
    if (this->uiVars.UIVertexTransferBuffer) {
      SDL_ReleaseGPUTransferBuffer(this->basicInitVars.GPU, this->uiVars.UIVertexTransferBuffer);
      this->uiVars.UIVertexTransferBuffer = nullptr;
    }
    if (EntityBuffer) {
      SDL_ReleaseGPUBuffer(this->basicInitVars.GPU, EntityBuffer);
      EntityBuffer = nullptr;
    }
    if (EntityTransferBuffer) {
      SDL_ReleaseGPUTransferBuffer(this->basicInitVars.GPU,
                                   EntityTransferBuffer);
      EntityTransferBuffer = nullptr;
    }
    if (EntityIndexBuffer) {
      SDL_ReleaseGPUBuffer(this->basicInitVars.GPU, EntityIndexBuffer);
      EntityIndexBuffer = nullptr;
    }
    if (EntityIndexTransferBuffer) {
      SDL_ReleaseGPUTransferBuffer(this->basicInitVars.GPU,
                                   EntityIndexTransferBuffer);
      EntityIndexTransferBuffer = nullptr;
    }

    // Release DepthTexture before destroying GPU device
    if (DepthTexture) {
      SDL_ReleaseGPUTexture(this->basicInitVars.GPU, DepthTexture);
      DepthTexture = nullptr;
    }

    if (basicInitVars.GPU) {
      SDL_DestroyGPUDevice(basicInitVars.GPU);
      basicInitVars.GPU = nullptr;
    }
    if (runTimeRenderVars.pass) {
      runTimeRenderVars.pass = nullptr;
    }
    if (basicInitVars.window) {
      SDL_DestroyWindow(basicInitVars.window);
      basicInitVars.window = nullptr;
    }
    basicInitVars.event = {};
    runTimeRenderVars.cmdCopy = nullptr;
    runTimeRenderVars.cmdRender = nullptr;
  };

  void Stats(Player &player);
  void DrawBg(std::vector<Player> &players);
  void DrawPlayers(std::vector<Player> &players);
  void DrawUI(SDL_GPUCommandBuffer *cmd,
              const std::vector<Slot> &inventory, int inventorySlot);
      
  void MainRenderLoop(std::vector<Slot> &inventory, int &inventorySlot,
                      std::vector<Player> &players);
};