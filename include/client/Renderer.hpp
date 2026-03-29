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
  // Check if an axis-aligned bounding box (AABB) is inside or intersecting the
  // frustum Uses the "p-vertex/n-vertex" test for each plane
  bool isChunkInFrustum(const Vector3 &minPoint, const Vector3 &maxPoint,
                        float tolerance = 10.0f) const {
    const Plane *const planes[6] = {&nearFace,  &farFace, &leftFace,
                                    &rightFace, &topFace, &bottomFace};

    for (int p = 0; p < 6; ++p) {
      // P-vertex is the corner of the AABB most in the direction of the plane
      // normal
      Vector3 p_vertex = minPoint;
      if (planes[p]->normal.x >= 0)
        p_vertex.x = maxPoint.x;
      if (planes[p]->normal.y >= 0)
        p_vertex.y = maxPoint.y;
      if (planes[p]->normal.z >= 0)
        p_vertex.z = maxPoint.z;

      if (planes[p]->getSignedDistanceToPlane(p_vertex) < -tolerance) {
        return false;
      }
    }
    return true;
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
struct DVertex {
  Vector3 Position; // x.u, y.v, z
  float Data;       // Packed bits: side(3), blockID(ChunkPrefab::xSize), light(4)
};
struct Vertex {
  Vector3 Position; // x, y (NDC), z (packed UV)
  Uint32 Color;     // Packed RGBA8
  float BlockID;
};
struct Mesh {
  SDL_GPUTransferBuffer *VertextransferBuffer = nullptr;
  SDL_GPUTransferBuffer *IndextransferBuffer = nullptr;
  SDL_GPUBufferBinding VertexBuffer;
  SDL_GPUBufferBinding IndexBuffer;
  std::vector<DrawnFace> Faces;
  int BaseVertex = 0, BaseIndex = 0;

  // Stability tracking
  std::vector<ChunkPrefab *> currentChunks;
  bool needsUpdate = true;

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
  SDL_GPUGraphicsPipeline *textPipeline = nullptr;
  SDL_GPUVertexBufferDescription vertex_buffer_desc;
  SDL_GPUVertexBufferDescription UIvertex_buffer_desc;
  SDL_GPUVertexAttribute vertex_attributes[5];
  SDL_GPUVertexAttribute UIvertex_attributes[4];
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
  std::vector<DVertex> vertices;
  std::vector<Uint32> indices;
};
struct UIVars {
  std::vector<Vertex> uiVertices;
  SDL_GPUBuffer *UIVertexBuffer = nullptr;
  SDL_GPUBuffer *UIIndexBuffer = nullptr;
  SDL_GPUTransferBuffer *UIVertexTransferBuffer = nullptr;
  SDL_GPUTransferBuffer *UIIndexTransferBuffer = nullptr;

  TTF_Font *font = nullptr;
  SDL_GPUTexture *fontTexture = nullptr;
  SDL_GPUSampler *fontSampler = nullptr;
  std::vector<Vertex> textVertices;
  SDL_GPUBuffer *textVertexBuffer = nullptr;
  SDL_GPUTransferBuffer *textVertexTransferBuffer = nullptr;
};
struct InventoryBox {
  int index;
  float xNDC, yNDC, wNDC, hNDC;
  bool isHotbar;
};
struct CraftingVars {
  bool is3x3;
  int gridSize;
  float storageBaseY, storageH, craftToStorageGap, craftW, panelX, panelW,
      craftSlotSize, craftGap, craftGridH, craftGridW;
  std::vector<InventoryBox> &boxes;
};
struct UIRuntimeVars {
  bool fullScreen = false, bigInventory = false, isCraftingTable = false,
       showDebug = false, usingUI = false, isFurnace = false;
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
  UIRuntimeVars uiRuntimeVars;
  Frustum frustum;
  std::vector<Mesh> Terrain;
  int chunksPerBuffer = 25, totalBuffers = 0;
  Vector3 lastPlayerChunk{-999, -999, -999};
  Vector3 lastRot{-999, -999, -999};
  std::vector<ChunkPrefab *> opaqueChunks;
  std::vector<ChunkPrefab *> lastVisibleChunks;

  auto AddRect(float x, float y, float w, float h, Vector3 color, float blockID = 0);
  void AddTextRect(float x, float y, float w, float h, SDL_FPoint uvMin, SDL_FPoint uvMax, Vector3 color);
  void DrawText(const std::string &text, float x, float y, float scale, Vector3 color, float maxWidth, float wrapWidth);
  void UICrossHair();
  std::vector<InventoryBox> BuildInventoryBoxes(float aspect, bool is3x3, float &outPanelX, float &outPanelY, float &outPanelW, float &outPanelH);
  void CraftingTable(CraftingVars &Crafting);
  void SmeltingTable(CraftingVars &Crafting);
  void UIBigInventory(const std::vector<Slot> &inventory, int inventorySlot);
  void UIInventory(const std::vector<Slot> &inventory, int inventorySlot);
  void BuildUIGeometry(const std::vector<Slot> &inventory, int inventorySlot, Player &player);
  void UploadUIBuffers();
  void UploadVertexBuffer(const std::vector<Vertex> &vertices, SDL_GPUTransferBuffer *transferBuffer, SDL_GPUBuffer *gpuBuffer);
  void RenderUIPass();
  void DrawUISprites();
  void DrawUIText();
  std::vector<ChunkDistance> SortChunks(Player &player, Vector3 PlayerChunk);
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
  void HandleQuit();
  void HandleMouseWheel(int &inventorySlot);
  Vector2 ScreenToNDC(float mx, float my) const;
  int GetHoveredBoxIndex(Vector2 ndc,
                         const std::vector<InventoryBox> &boxes) const;
  void HandleMouseButtonDown(Player &player);
  void HandleLeftClickDown(Player &player, int slotIndex);
  void HandleRightClickDown(Player &player, int slotIndex);
  void PickUpHalfStack(Player &player, int slotIndex);
  void PlaceOneItem(Player &player, int slotIndex);
  void ConsumeIngredients(Player &player);
  void HandleMouseMotion(Player &player);
  void HandleMouseButtonUp(Player &player);
  void FinalizeLeftClickDrag(Player &player);
  void DistributeAcrossSlots(Player &player);
  void HandleKeyDown(Player &player, int &inventorySlot);
  void HandleEscapeKey();
  void HandleF11Key();
  void HandleEKey();
  void UIVertexGPUInit();

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
    if (pipelineInitVars.textPipeline) {
      SDL_ReleaseGPUGraphicsPipeline(this->basicInitVars.GPU,
                                     pipelineInitVars.textPipeline);
    }

    if (TextureAtlas) {
      SDL_ReleaseGPUTexture(this->basicInitVars.GPU, TextureAtlas);
      TextureAtlas = nullptr;
    }
    if (Sampler) {
      SDL_ReleaseGPUSampler(this->basicInitVars.GPU, Sampler);
      Sampler = nullptr;
    }
    if (uiVars.fontTexture) {
      SDL_ReleaseGPUTexture(this->basicInitVars.GPU, uiVars.fontTexture);
    }
    if (uiVars.fontSampler) {
      SDL_ReleaseGPUSampler(this->basicInitVars.GPU, uiVars.fontSampler);
    }
    if (uiVars.font) {
      TTF_CloseFont(uiVars.font);
    }
    if (this->uiVars.UIVertexBuffer) {
      SDL_ReleaseGPUBuffer(this->basicInitVars.GPU,
                           this->uiVars.UIVertexBuffer);
      this->uiVars.UIVertexBuffer = nullptr;
    }
    if (this->uiVars.UIVertexTransferBuffer) {
      SDL_ReleaseGPUTransferBuffer(this->basicInitVars.GPU,
                                   this->uiVars.UIVertexTransferBuffer);
      this->uiVars.UIVertexTransferBuffer = nullptr;
    }
    if (this->uiVars.textVertexBuffer) {
      SDL_ReleaseGPUBuffer(this->basicInitVars.GPU,
                           this->uiVars.textVertexBuffer);
    }
    if (this->uiVars.textVertexTransferBuffer) {
      SDL_ReleaseGPUTransferBuffer(this->basicInitVars.GPU,
                                   this->uiVars.textVertexTransferBuffer);
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
      SDL_WaitForGPUIdle(basicInitVars.GPU);
      SDL_DestroyGPUDevice(basicInitVars.GPU);
      basicInitVars.GPU = nullptr;
    }
    if (TTF_WasInit()) {
      TTF_Quit();
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

    SDL_Quit();
  };

  void SetUi(int i) {
    if (i == 24) {
      OpenInventory(true);
    } else if (i == 25) {
      this->uiRuntimeVars.isFurnace = true;
      OpenInventory(false);
    }
  };
  bool UsingUI() { return this->uiRuntimeVars.usingUI; };
  void Stats(Player &player);
  void UIDebug(Player &player);
  void DrawBg(std::vector<Player> &players);
  void DrawPlayers(std::vector<Player> &players);
  void DrawUI(const std::vector<Slot> &inventory, int inventorySlot, Player &player);

  void MainRenderLoop(std::vector<Slot> &inventory, int inventorySlot,
                      std::vector<Player> &players);
  void OpenInventory(bool craftingTable);
};