#include "../../include/client/Renderer.hpp"
#include "../../include/client/GameManager.hpp"
#include "../../include/common/Chunck.hpp"
#include "../../include/common/EntityDef.hpp"
#include "../../include/common/RecipeDef.hpp"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_stdinc.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <sys/types.h>
#include <vector>

constexpr Uint32 FacesPerChunk = 2200;

Slot g_heldItem = {0, 0};
std::vector<int> g_draggedSlots;
bool g_isRightClickDragging = false;
bool g_isLeftClickDragging = false;
bool g_justPickedUp = false;
int g_initialClickSlot = -1;

const float FOV = 80.0f;
const float Znear = 0.1f;
constexpr float Zfar = 500.0f;
constexpr int RenderDistance = 9;
const float tanHalfFov = tan(FOV * PI / 360.0f);
static constexpr int CRAFTING_RESULT_SLOT = 49;
static constexpr int CRAFTING_INPUT_FIRST = 40;
static constexpr int CRAFTING_INPUT_LAST = 48;

const Vector3 Verts[6][4] = {
    {// Front (+Z) - looking at face from outside (positive Z direction)
     // Counter-clockwise: bottom-right, bottom-left, top-right, top-left
     {0.0, 1.0, 1.0},  // 3: top-left
     {1.0, 1.0, 1.0},  // 2: top-right
     {0.0, 0.0, 1.0},  // 1: bottom-left
     {1.0, 0.0, 1.0}}, // 0: bottom-right
    {// Back (-Z) - looking at face from outside (negative Z direction)
     // Counter-clockwise: bottom-left, bottom-right, top-left, top-right
     {1.0, 1.0, 0.0},  // 3: top-right
     {0.0, 1.0, 0.0},  // 2: top-left
     {1.0, 0.0, 0.0},  // 1: bottom-right
     {0.0, 0.0, 0.0}}, // 0: bottom-left
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
  const int tileSize = ChunkPrefab::xSize;
  const int atlasSize = 512;
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
  Uint32 c = (Uint32)(color.x * 255.0f) << 0 | (Uint32)(color.y * 255.0f) << 8 |
             (Uint32)(color.z * 255.0f) << ChunkPrefab::xSize |
             (Uint32)(255) << 24;

  auto packUV = [](float u, float v) -> float {
    uint32_t uu = (uint32_t)(u * 65535.0f) & 0xFFFF;
    uint32_t vv = (uint32_t)(v * 65535.0f) & 0xFFFF;
    uint32_t packed = (uu << ChunkPrefab::xSize) | vv;
    return *(float *)&packed;
  };

  this->uiVars.uiVertices.push_back({{x, y, packUV(0.0f, 1.0f)}, c, blockID});
  this->uiVars.uiVertices.push_back(
      {{x + w, y, packUV(1.0f, 1.0f)}, c, blockID});
  this->uiVars.uiVertices.push_back(
      {{x, y + h, packUV(0.0f, 0.0f)}, c, blockID});
  this->uiVars.uiVertices.push_back(
      {{x + w, y, packUV(1.0f, 1.0f)}, c, blockID});
  this->uiVars.uiVertices.push_back(
      {{x + w, y + h, packUV(1.0f, 0.0f)}, c, blockID});
  this->uiVars.uiVertices.push_back(
      {{x, y + h, packUV(0.0f, 0.0f)}, c, blockID});
}
void Renderer::AddTextRect(float x, float y, float w, float h, SDL_FPoint uvMin,
                           SDL_FPoint uvMax, Vector3 color) {
  Uint32 c = (Uint32)(color.x * 255.0f) << 0 | (Uint32)(color.y * 255.0f) << 8 |
             (Uint32)(color.z * 255.0f) << ChunkPrefab::xSize |
             (Uint32)(255) << 24;

  auto packUV = [](float u, float v) -> float {
    uint32_t uu = (uint32_t)(u * 65535.0f) & 0xFFFF;
    uint32_t vv = (uint32_t)(v * 65535.0f) & 0xFFFF;
    uint32_t packed = (uu << ChunkPrefab::xSize) | vv;
    return *(float *)&packed;
  };

  uiVars.textVertices.push_back({{x, y, packUV(uvMin.x, uvMax.y)}, c, 0.0f});
  uiVars.textVertices.push_back(
      {{x, y + h, packUV(uvMin.x, uvMin.y)}, c, 0.0f});
  uiVars.textVertices.push_back(
      {{x + w, y, packUV(uvMax.x, uvMax.y)}, c, 0.0f});
  uiVars.textVertices.push_back(
      {{x + w, y, packUV(uvMax.x, uvMax.y)}, c, 0.0f});
  uiVars.textVertices.push_back(
      {{x, y + h, packUV(uvMin.x, uvMin.y)}, c, 0.0f});
  uiVars.textVertices.push_back(
      {{x + w, y + h, packUV(uvMax.x, uvMin.y)}, c, 0.0f});
}
void Renderer::DrawText(const std::string &text, float x, float y, float scale,
                        Vector3 color, float maxWidth = 0,
                        float wrapWidth = 0) {
  if (text.empty())
    return;

  static const char *fontChars =
      " 0123456789:.XYZ/-abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static const int numChars = (int)strlen(fontChars);

  float fontAspectRatio = 0.5f;
  float baseCharSize = 0.08f * scale;
  float charW =
      (baseCharSize * fontAspectRatio) / this->runTimeRenderVars.aspect;
  float charH = baseCharSize;
  float charAdvance = charW * 1.2f;

  float currentX = x;
  float currentY = y;

  for (char c : text) {
    // Word wrap: start a new line if we exceed wrapWidth
    if (wrapWidth > 0.0f && (currentX - x) + charAdvance > wrapWidth) {
      currentX = x;
      currentY -= charH * 1.4f; // move down one line
    }

    // Hard clip: stop rendering entirely if we exceed maxWidth
    if (maxWidth > 0.0f && (currentX - x) + charW > maxWidth)
      break;

    const char *ptr = strchr(fontChars, c);
    if (!ptr)
      continue;

    int idx = (int)(ptr - fontChars);
    float uvX1 = (float)idx / (float)numChars;
    float uvX2 = (float)(idx + 1) / (float)numChars;
    float padding = 0.0005f;

    AddTextRect(currentX, currentY, charW, charH, {uvX1 + padding, 0.0f},
                {uvX2 - padding, 1.0f}, color);

    currentX += charAdvance;
  }
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

std::vector<InventoryBox>
Renderer::BuildInventoryBoxes(float aspect, bool is3x3, float &outPanelX,
                              float &outPanelY, float &outPanelW,
                              float &outPanelH) {

  std::vector<InventoryBox> boxes;

  // Constants for layout
  const int cols = 9;
  const int storageRows = 3;
  const float slotSize = 0.1f;
  const float slotSpacing = 0.012f;
  const float hotbarGap = 0.03f;
  const float craftSlotSize = 0.08f;
  const float craftGap = 0.01f;
  const float craftToStorageGap = 0.1f;

  float storageW = cols * slotSize + (cols + 1) * slotSpacing;
  float storageH = (storageRows + 1) * slotSize + storageRows * slotSpacing +
                   hotbarGap + 2 * slotSpacing;

  // Calculate crafting area size
  int gridSize = is3x3 ? 3 : 2;
  float craftGridW = gridSize * craftSlotSize + (gridSize - 1) * craftGap;
  float craftGridH = gridSize * craftSlotSize + (gridSize - 1) * craftGap;
  float craftW = craftGridW + 0.15f + craftSlotSize;
  float craftH = craftGridH;

  float totalLogicalWidth = std::max(storageW, craftW);
  float totalLogicalHeight = storageH + craftH + craftToStorageGap;

  float panelW = (totalLogicalWidth + 0.1f) / aspect;
  float panelH = totalLogicalHeight + 0.1f;
  float panelX = -panelW / 2.0f;
  float panelY = -panelH / 2.0f;

  outPanelX = panelX;
  outPanelY = panelY;
  outPanelW = panelW;
  outPanelH = panelH;

  // 1. Storage & Hotbar
  float storageBaseX = panelX + (panelW - (storageW / aspect)) / 2.0f;
  float storageBaseY = panelY + slotSpacing;

  for (int row = 0; row < storageRows + 1; row++) {
    for (int col = 0; col < cols; col++) {
      int i = (row == 0) ? col : (row * cols + col);
      float xNDC = storageBaseX +
                   (slotSpacing + col * (slotSize + slotSpacing)) / aspect;
      float yNDC = storageBaseY + row * (slotSize + slotSpacing);
      if (row >= 1)
        yNDC += hotbarGap;

      boxes.push_back({i, xNDC, yNDC, slotSize / aspect, slotSize, row == 0});
    }
  }
  if (this->uiRuntimeVars.isFurnace) {
    CraftingVars Crafting = {
        is3x3,      gridSize,   storageBaseY, storageH,      craftToStorageGap,
        craftW,     panelX,     panelW,       craftSlotSize, craftGap,
        craftGridH, craftGridW, boxes};
    SmeltingTable(Crafting);
  } else {
    CraftingVars Crafting = {
        is3x3,      gridSize,   storageBaseY, storageH,      craftToStorageGap,
        craftW,     panelX,     panelW,       craftSlotSize, craftGap,
        craftGridH, craftGridW, boxes};
    CraftingTable(Crafting);
  }
  return boxes;
}
void Renderer::CraftingTable(CraftingVars &Crafting) {
  float craftingOffset = !Crafting.is3x3 ? 0.3 : 1;
  float aspect = this->runTimeRenderVars.aspect;
  float craftBaseY = Crafting.storageBaseY + Crafting.storageH +
                     (Crafting.craftToStorageGap / 2.0f);
  float craftAreaW = Crafting.craftW / aspect;
  float craftBaseX =
      Crafting.panelX * craftingOffset + (Crafting.panelW - craftAreaW) / 2.0f;

  for (int y = 0; y < Crafting.gridSize; y++) {
    for (int x = 0; x < Crafting.gridSize; x++) {
      int slotIdx =
          40 + (Crafting.gridSize - 1 - y) * 3 + x; // Map to 3x3 logical grid
      if (!Crafting.is3x3) {
        // If 2x2, we use 40,41 and 43,44
        slotIdx = 40 + (1 - y) * 3 + x;
      }
      float xNDC = craftBaseX +
                   (x * (Crafting.craftSlotSize + Crafting.craftGap)) / aspect;
      float yNDC =
          craftBaseY + (y * (Crafting.craftSlotSize + Crafting.craftGap));
      Crafting.boxes.push_back({slotIdx, xNDC, yNDC,
                                Crafting.craftSlotSize / aspect,
                                Crafting.craftSlotSize, false});
    }
  }

  // Output slot
  float outX = craftBaseX + 0.05f + Crafting.craftGridW / aspect;
  float outY =
      craftBaseY + (Crafting.craftGridH - Crafting.craftSlotSize) / 2.0f;
  Crafting.boxes.push_back({49, outX, outY, Crafting.craftSlotSize / aspect,
                            Crafting.craftSlotSize, false});
}
void Renderer::SmeltingTable(CraftingVars &Crafting) {
  float craftingOffset = !Crafting.is3x3 ? 0.3 : 1;
  float aspect = this->runTimeRenderVars.aspect;
  float craftBaseY = Crafting.storageBaseY + Crafting.storageH +
                     (Crafting.craftToStorageGap / 2.0f);
  float craftAreaW = Crafting.craftW / aspect;
  float craftBaseX =
      Crafting.panelX * craftingOffset + (Crafting.panelW - craftAreaW) / 2.0f;

  for (int y = 0; y < 2; y++) {
    for (int x = 0; x < 1; x++) {
      int slotIdx =
          40 + (Crafting.gridSize - 1 - y) * 3 + x; // Map to 3x3 logical grid
      if (!Crafting.is3x3) {
        // If 2x2, we use 40,41 and 43,44
        slotIdx = 40 + (1 - y) * 3 + x;
      }
      float xNDC = craftBaseX +
                   (x * (Crafting.craftSlotSize + Crafting.craftGap)) / aspect;
      float yNDC =
          craftBaseY + (y * (Crafting.craftSlotSize + Crafting.craftGap));
      Crafting.boxes.push_back({slotIdx, xNDC, yNDC,
                                Crafting.craftSlotSize / aspect,
                                Crafting.craftSlotSize, false});
    }
  }

  // Output slot
  float outX = craftBaseX + Crafting.craftGridW / aspect;
  float outY =
      craftBaseY + (Crafting.craftGridH - Crafting.craftSlotSize) / 2.0f;
  Crafting.boxes.push_back({49, outX, outY, Crafting.craftSlotSize / aspect,
                            Crafting.craftSlotSize, false});
}
void Renderer::UIBigInventory(const std::vector<Slot> &inventory,
                              int inventorySlot) {
  float panelX, panelY, panelW, panelH;
  std::vector<InventoryBox> boxes = BuildInventoryBoxes(
      this->runTimeRenderVars.aspect, this->uiRuntimeVars.isCraftingTable,
      panelX, panelY, panelW, panelH);

  // Dimmed background overlay (full screen)
  AddRect(-2.0f, -2.0f, 4.0f, 4.0f, {0.0f, 0.0f, 0.0f}, -1.0f);

  // Panel background
  AddRect(panelX, panelY, panelW, panelH, {0.1f, 0.1f, 0.1f});

  float mx, my;
  SDL_GetMouseState(&mx, &my);
  float ndc_mx = (mx / (float)this->basicInitVars.Width) * 2.0f - 1.0f;
  float ndc_my = 1.0f - (my / (float)this->basicInitVars.Height) * 2.0f;

  for (const auto &box : boxes) {
    int i = box.index;
    float xNDC = box.xNDC;
    float yNDC = box.yNDC;
    float wNDC = box.wNDC;
    float hNDC = box.hNDC;

    bool isHovered = (ndc_mx >= xNDC && ndc_mx <= xNDC + wNDC &&
                      ndc_my >= yNDC && ndc_my <= yNDC + hNDC);

    // Slot background - highlight selected hotbar slot or hovered slot
    bool isSelected = box.isHotbar && (i == inventorySlot);
    Vector3 bgColor;
    if (isHovered) {
      bgColor = Vector3{0.5f, 0.5f, 0.5f};
    } else if (isSelected) {
      bgColor = Vector3{0.4f, 0.4f, 0.4f};
    } else {
      bgColor = Vector3{0.2f, 0.2f, 0.2f};
    }
    AddRect(xNDC, yNDC, wNDC, hNDC, bgColor);

    // Border highlight for selected slot
    if (isSelected) {
      float b = 0.005f;
      float bX = b / this->runTimeRenderVars.aspect;
      float bY = b;
      AddRect(xNDC - bX, yNDC + hNDC, wNDC + 2 * bX, bY,
              {1.0f, 1.0f, 1.0f}); // top
      AddRect(xNDC - bX, yNDC - bY, wNDC + 2 * bX, bY,
              {1.0f, 1.0f, 1.0f});                              // bottom
      AddRect(xNDC - bX, yNDC, bX, hNDC, {1.0f, 1.0f, 1.0f});   // left
      AddRect(xNDC + wNDC, yNDC, bX, hNDC, {1.0f, 1.0f, 1.0f}); // right
    }

    // Item icon
    if (i < (int)inventory.size() && inventory[i].Type != 0) {
      float iconPadding = 0.01f;
      if (BlockDef[inventory[i].Type].CanPlace())
        iconPadding = 0.02f;
      float pX = iconPadding / this->runTimeRenderVars.aspect;
      float pY = iconPadding;
      AddRect(xNDC + pX, yNDC + pY, wNDC - 2 * pX, hNDC - 2 * pY, {1, 1, 1},
              (float)BlockDef[inventory[i].Type].Textures[0]);
    }

    // Stack count
    if (i < (int)inventory.size() && inventory[i].Type != 0 &&
        inventory[i].Amount > 1) {
      std::string amountStr = std::to_string(inventory[i].Amount);
      float textScale = 0.7f;
      float textX = xNDC + wNDC * 0.58f;
      float textY = yNDC + 0.008f;
      DrawText(amountStr, textX, textY, textScale, {1.0f, 1.0f, 1.0f});
    }
  }

  // Draw globally held item
  if (g_heldItem.Type != 0) {
    float mx, my;
    unsigned int w, h;
    SDL_GetWindowSizeInPixels(this->basicInitVars.window, (int *)&w, (int *)&h);
    SDL_GetMouseState(&mx, &my);
    float ndc_mx = (mx / (float)w) * 2.0f - 1.0f;
    float ndc_my = 1.0f - (my / (float)h) * 2.0f;

    float heldSlotSize = 0.12f; // Standard size for held
    float wNDC_held = heldSlotSize / this->runTimeRenderVars.aspect;
    float hNDC_held = heldSlotSize;
    float pX_held = 0.012f / this->runTimeRenderVars.aspect;
    float pY_held = 0.012f;

    AddRect(ndc_mx - wNDC_held / 2 + pX_held, ndc_my - hNDC_held / 2 + pY_held,
            wNDC_held - 2 * pX_held, hNDC_held - 2 * pY_held, {1, 1, 1},
            (float)BlockDef[g_heldItem.Type].Textures[0]);

    if (g_heldItem.Amount > 1) {
      std::string amountStr = std::to_string(g_heldItem.Amount);
      DrawText(amountStr, ndc_mx + wNDC_held * 0.15f,
               ndc_my - hNDC_held * 0.35f, 0.7f, {1.0f, 1.0f, 1.0f});
    }
  }
}
static void UpdateCraftingSelection(Player &player) {
  auto &inv = player.inventory;

  const int resultSlot = 49;
  const int gridStart = 40;

  inv[resultSlot] = {0, 0, false};

  // 1. Identify valid bounding box of the input items
  int minX = 3, minY = 3, maxX = -1, maxY = -1;
  bool inputEmpty = true;

  for (int y = 0; y < 3; y++) {
    for (int x = 0; x < 3; x++) {
      if (inv[gridStart + y * 3 + x].Type != 0) {
        minX = std::min(minX, x);
        minY = std::min(minY, y);
        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
        inputEmpty = false;
      }
    }
  }

  if (inputEmpty)
    return;

  int inputW = maxX - minX + 1;
  int inputH = maxY - minY + 1;

  // 2. Iterate through recipes
  for (int rIdx = 0; rIdx < (int)CraftingListAmount; rIdx++) {
    const auto &recipe = CraftingList[rIdx];

    // 3. Identify valid bounding box of the recipe
    int rMinX = 3, rMinY = 3, rMaxX = -1, rMaxY = -1;
    bool recipeEmpty = true;

    for (int ry = 0; ry < 3; ry++) {
      for (int rx = 0; rx < 3; rx++) {
        if (recipe.input[ry * 3 + rx].Type != 0) {
          rMinX = std::min(rMinX, rx);
          rMinY = std::min(rMinY, ry);
          rMaxX = std::max(rMaxX, rx);
          rMaxY = std::max(rMaxY, ry);
          recipeEmpty = false;
        }
      }
    }

    if (recipeEmpty)
      continue;

    int recipeW = rMaxX - rMinX + 1;
    int recipeH = rMaxY - rMinY + 1;

    // 4. Compare bounding boxes
    if (inputW != recipeW || inputH != recipeH)
      continue;

    bool match = true;
    for (int y = 0; y < inputH && match; y++) {
      for (int x = 0; x < inputW && match; x++) {
        const auto &invSlot = inv[gridStart + (minY + y) * 3 + (minX + x)];
        const auto &recipeSlot = recipe.input[(rMinY + y) * 3 + (rMinX + x)];

        if (invSlot.Type != recipeSlot.Type ||
            invSlot.isEntity != recipeSlot.isEntity)
          match = false;
      }
    }

    if (match) {
      inv[resultSlot] = recipe.output;
      return;
    }
  }
}
static void UpdateSmeltingSelection(Player &player) {
  auto &inv = player.inventory;

  const int resultSlot = 49;
  const int gridStart = 40;

  inv[resultSlot] = {0, 0, false};

  for (auto &recepie : SmeltingList) {
    if (recepie.input[0].Type == inv[gridStart].Type &&
        BlockDef[inv[gridStart + 3].Type].Type == ItemType::FUEL) {
      inv[resultSlot] = recepie.output;
      // if (inv[gridStart].Amount-- < 1) inv[gridStart].Type = 0;
      // if (inv[gridStart + 3].Amount-- < 1) inv[gridStart + 3].Type = 0;
    }
  }
}
void Renderer::UIInventory(const std::vector<Slot> &inventory,
                           int inventorySlot) {
  float hotbarHeight = 0.12f;
  float slotCount = 9;
  float slotSpacing = 0.01f;

  float logicalSlotSize = hotbarHeight - 2 * slotSpacing;
  float logicalTotalWidth =
      slotCount * logicalSlotSize + (slotCount + 1) * slotSpacing;

  float hotbarWidthNDC = logicalTotalWidth / this->runTimeRenderVars.aspect;
  float hotbarX = -hotbarWidthNDC / 2.0f;
  float hotbarY = -0.95f;

  AddRect(hotbarX, hotbarY, hotbarWidthNDC, hotbarHeight, {0.1f, 0.1f, 0.1f});

  for (int i = 0; i < slotCount; i++) {
    float xNDC = hotbarX + (slotSpacing + i * (logicalSlotSize + slotSpacing)) /
                               this->runTimeRenderVars.aspect;
    float yNDC = hotbarY + slotSpacing;
    float wNDC = logicalSlotSize / this->runTimeRenderVars.aspect;
    float hNDC = logicalSlotSize;

    Vector3 bgColor = (i == inventorySlot) ? Vector3{0.4f, 0.4f, 0.4f}
                                           : Vector3{0.2f, 0.2f, 0.2f};
    AddRect(xNDC, yNDC, wNDC, hNDC, bgColor);

    if (i == inventorySlot) {
      float b = 0.005f;
      float bX = b / this->runTimeRenderVars.aspect;
      float bY = b;

      AddRect(xNDC - bX, yNDC + hNDC, wNDC + 2 * bX, bY, {1.0f, 1.0f, 1.0f});
      AddRect(xNDC - bX, yNDC - bY, wNDC + 2 * bX, bY, {1.0f, 1.0f, 1.0f});
      AddRect(xNDC - bX, yNDC, bX, hNDC, {1.0f, 1.0f, 1.0f});
      AddRect(xNDC + wNDC, yNDC, bX, hNDC, {1.0f, 1.0f, 1.0f});
    }

    if (i < inventory.size() && inventory[i].Type != 0) {
      float iconPadding = 0.0075f;
      if (BlockDef[inventory[i].Type].CanPlace())
        iconPadding = 0.015f;
      float pX = iconPadding / this->runTimeRenderVars.aspect;
      float pY = iconPadding;
      AddRect(xNDC + pX, yNDC + pY, wNDC - 2 * pX, hNDC - 2 * pY, {1, 1, 1},
              (float)BlockDef[inventory[i].Type].Textures[0]);
    }

    if (i < inventory.size() && inventory[i].Type != 0 &&
        inventory[i].Amount > 1) {
      std::string amountStr = std::to_string(inventory[i].Amount);
      float textScale = 0.55f;
      float textX = xNDC + wNDC * 0.62f;
      float textY = yNDC + 0.008f;
      DrawText(amountStr, textX, textY, textScale, {1.0f, 1.0f, 1.0f});
    }
  }
}
void Renderer::UIDebug(Player &player) {
  if (!this->uiRuntimeVars.showDebug)
    return;

  float startX = -0.98f;
  float startY = 0.95f;
  float lineStep = 0.08f;
  float scale = 0.8f;

  char buf[256];
  // Coordinates
  SDL_snprintf(buf, sizeof(buf), "XYZ: %.3f / %.3f / %.3f", player.Position.x,
               player.Position.y, player.Position.z);
  DrawText(buf, startX, startY, scale, {1, 1, 1});

  // Rotation
  SDL_snprintf(buf, sizeof(buf), "ROT: %.1f / %.1f", player.Rotation.x,
               player.Rotation.y);
  DrawText(buf, startX, startY - lineStep, scale, {1, 1, 1});

  // Chunks
  int cx = (int)floor(player.Position.x / ChunkPrefab::xSize);
  int cz = (int)floor(player.Position.z / ChunkPrefab::xSize);
  SDL_snprintf(buf, sizeof(buf), "Chunk: %d / %d", cx, cz);
  DrawText(buf, startX, startY - lineStep * 2.0f, scale, {1, 1, 1});
}

void Renderer::DrawUI(const std::vector<Slot> &inventory, int inventorySlot,
                      Player &player) {
  BuildUIGeometry(inventory, inventorySlot, player);
  UploadUIBuffers();
  RenderUIPass();
}

void Renderer::BuildUIGeometry(const std::vector<Slot> &inventory,
                               int inventorySlot, Player &player) {
  uiVars.uiVertices.clear();
  uiVars.textVertices.clear();

  UICrossHair();
  UIInventory(inventory, inventorySlot);
  if (uiRuntimeVars.bigInventory)
    UIBigInventory(inventory, inventorySlot);
  UIDebug(player);
}

void Renderer::UploadUIBuffers() {
  UploadVertexBuffer(uiVars.uiVertices, uiVars.UIVertexTransferBuffer,
                     uiVars.UIVertexBuffer);
  UploadVertexBuffer(uiVars.textVertices, uiVars.textVertexTransferBuffer,
                     uiVars.textVertexBuffer);
}

void Renderer::UploadVertexBuffer(const std::vector<Vertex> &vertices,
                                  SDL_GPUTransferBuffer *transferBuffer,
                                  SDL_GPUBuffer *gpuBuffer) {
  if (vertices.empty())
    return;

  void *mapped =
      SDL_MapGPUTransferBuffer(basicInitVars.GPU, transferBuffer, true);
  if (!mapped)
    return;

  const Uint32 byteSize = static_cast<Uint32>(vertices.size() * sizeof(Vertex));

  SDL_memcpy(mapped, vertices.data(), byteSize);
  SDL_UnmapGPUTransferBuffer(basicInitVars.GPU, transferBuffer);

  SDL_GPUTransferBufferLocation src = {transferBuffer, 0};
  SDL_GPUBufferRegion dst = {gpuBuffer, 0, byteSize};

  SDL_GPUCopyPass *copyPass =
      SDL_BeginGPUCopyPass(this->runTimeRenderVars.cmdRender);
  SDL_UploadToGPUBuffer(copyPass, &src, &dst, true);
  SDL_EndGPUCopyPass(copyPass);
}

void Renderer::RenderUIPass() {
  SDL_GPUColorTargetInfo colorTarget = {};
  colorTarget.texture = runTimeRenderVars.swap_texture;
  colorTarget.load_op = SDL_GPU_LOADOP_LOAD;
  colorTarget.store_op = SDL_GPU_STOREOP_STORE;

  this->runTimeRenderVars.pass = SDL_BeginGPURenderPass(
      this->runTimeRenderVars.cmdRender, &colorTarget, 1, nullptr);

  DrawUISprites();
  DrawUIText();

  SDL_EndGPURenderPass(this->runTimeRenderVars.pass);
}

void Renderer::DrawUISprites() {
  if (uiVars.uiVertices.empty())
    return;

  SDL_BindGPUGraphicsPipeline(this->runTimeRenderVars.pass,
                              pipelineInitVars.uiPipeline);

  if (TextureAtlas && Sampler) {
    SDL_GPUTextureSamplerBinding samplerBinding = {TextureAtlas, Sampler};
    SDL_BindGPUFragmentSamplers(this->runTimeRenderVars.pass, 0,
                                &samplerBinding, 1);
  }

  SDL_GPUBufferBinding vb = {uiVars.UIVertexBuffer, 0};
  SDL_BindGPUVertexBuffers(this->runTimeRenderVars.pass, 0, &vb, 1);
  SDL_DrawGPUPrimitives(this->runTimeRenderVars.pass,
                        static_cast<Uint32>(uiVars.uiVertices.size()), 1, 0, 0);
}

void Renderer::DrawUIText() {
  if (uiVars.textVertices.empty() || !pipelineInitVars.textPipeline)
    return;

  SDL_BindGPUGraphicsPipeline(this->runTimeRenderVars.pass,
                              pipelineInitVars.textPipeline);

  if (uiVars.fontTexture && uiVars.fontSampler) {
    SDL_GPUTextureSamplerBinding samplerBinding = {uiVars.fontTexture,
                                                   uiVars.fontSampler};
    SDL_BindGPUFragmentSamplers(this->runTimeRenderVars.pass, 0,
                                &samplerBinding, 1);
  }

  SDL_GPUBufferBinding vb = {uiVars.textVertexBuffer, 0};
  SDL_BindGPUVertexBuffers(this->runTimeRenderVars.pass, 0, &vb, 1);
  SDL_DrawGPUPrimitives(this->runTimeRenderVars.pass,
                        static_cast<Uint32>(uiVars.textVertices.size()), 1, 0,
                        0);
}
std::vector<ChunkDistance> Renderer::SortChunks(Player &player,
                                                Vector3 PlayerChunk) {
  SpiralIterator spiral(RenderDistance * 2 + 1);
  std::vector<ChunkDistance> visibleChunkList;

  int maxChunks = (RenderDistance * 2 + 1) * (RenderDistance * 2 + 1);
  visibleChunkList.reserve(maxChunks);
  ChunkManager &chunkManager = this->gameManager.GetChunkManager();

  while (spiral.hasNext()) {
    std::pair<int, int> Pos = spiral.next();

    Vector3 ChunkPos = {(float)Pos.first, 0, (float)Pos.second};
    ChunkPos += PlayerChunk;

    Vector3 Min = {ChunkPos.x * (float)ChunkPrefab::xSize, 0,
                   ChunkPos.z * (float)ChunkPrefab::zSize};
    Vector3 Max = {(ChunkPos.x + 1) * (float)ChunkPrefab::xSize,
                   (float)ChunkPrefab::ySize,
                   (ChunkPos.z + 1) * (float)ChunkPrefab::zSize};

    if (!frustum.isChunkInFrustum(Min, Max))
      continue;

    ChunkPrefab &chunk = chunkManager.get_chunk(ChunkPos);

    Vector3 chunkCenter = {ChunkPos.x * (float)ChunkPrefab::xSize +
                               (float)ChunkPrefab::xSize / 2.0f,
                           (float)ChunkPrefab::ySize / 2.0f,
                           ChunkPos.z * (float)ChunkPrefab::zSize +
                               (float)ChunkPrefab::zSize / 2.0f};
    float d2 = (chunkCenter - player.Position).LengthSquared();

    visibleChunkList.push_back({&chunk, d2});
  }

  return visibleChunkList;
}
void Renderer::EvictUnusedMeshes(Vector3 playerPos, float maxDistance) {
  float maxDistSq = maxDistance * maxDistance;
  size_t startCount = opaqueMeshCache.size();
  for (auto it = opaqueMeshCache.begin(); it != opaqueMeshCache.end();) {
    Vector3 chunkWorldPos = {
        (float)it->first.x * ChunkPrefab::xSize + ChunkPrefab::xSize / 2.0f, 0,
        (float)it->first.z * ChunkPrefab::zSize + ChunkPrefab::zSize / 2.0f};
    if ((chunkWorldPos - playerPos).LengthSquared() > maxDistSq) {
      it = opaqueMeshCache.erase(it);
    } else {
      ++it;
    }
  }
  if (opaqueMeshCache.size() < startCount) {
    PrintLog("Evicted " + std::to_string(startCount - opaqueMeshCache.size()) +
             " cached meshes.");
  }
}
void Renderer::DrawTerrain(Player &player) {
  Vector3 PlayerChunk = worldToChunkKey(player.Position);

  float rotDiff = (player.Rotation - this->lastRot).LengthSquared();

  if (PlayerChunk == lastPlayerChunk && rotDiff < 0.1f) {
    return;
  }

  // Unload chunks and evict meshes every time the player moves to a new chunk
  float unloadDistance = (RenderDistance + 12) * ChunkPrefab::xSize;
  this->gameManager.GetChunkManager().UnloadFarChunks(player.Position,
                                                      unloadDistance);
  this->EvictUnusedMeshes(player.Position, unloadDistance);

  this->lastPlayerChunk = PlayerChunk;
  this->lastRot = player.Rotation;

  std::vector<ChunkDistance> visibleChunks = SortChunks(player, PlayerChunk);

  std::vector<std::vector<ChunkPrefab *>> newBuckets(this->totalBuffers - 1);

  for (auto &cd : visibleChunks) {
    int bx = cd.chunk->xPos / ChunkPrefab::xSize;
    int bz = cd.chunk->zPos / ChunkPrefab::zSize;
    // Simple hash to distribute chunks stably across available opaque buffers
    // (totalBuffers-1)
    int bIdx = (std::abs(bx) * 7 + std::abs(bz)) % (this->totalBuffers - 1);
    if (newBuckets[bIdx].size() < (size_t)chunksPerBuffer) {
      newBuckets[bIdx].push_back(cd.chunk);
    }
  }

  // 3. Opaque Pass - Fill buffers 0 to totalBuffers-2 using stable buckets
  for (int bufferIdx = 0; bufferIdx < this->totalBuffers - 1; bufferIdx++) {
    auto *mesh = &this->Terrain[bufferIdx];
    const std::vector<ChunkPrefab *> &newChunks = newBuckets[bufferIdx];

    bool anyChunkDirty = false;
    for (auto *c : newChunks)
      anyChunkDirty = c->needsMeshUpdate || anyChunkDirty;

    if (newChunks == mesh->currentChunks && !anyChunkDirty &&
        !mesh->needsUpdate) {
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

    DVertex *Vertexdata = (DVertex *)SDL_MapGPUTransferBuffer(
        this->basicInitVars.GPU, mesh->VertextransferBuffer, true);
    // Uint32 *Indexdata = (Uint32
    // *)SDL_MapGPUTransferBuffer(this->basicInitVars.GPU,
    // mesh->IndextransferBuffer, true);

    if (!Vertexdata)
      continue;

    size_t currentVertexOffset = 0;
    size_t currentIndexOffset = 0;
    const size_t maxVertices = chunksPerBuffer * FacesPerChunk;
    const size_t maxIndices = chunksPerBuffer * FacesPerChunk;

    for (auto *chunk : newChunks) {
      if (!chunk->isGenerated)
        continue;
      Vector3 chunkPosKey =
          worldToChunkKey(Vector3{(float)chunk->xPos, 0, (float)chunk->zPos});

      if (chunk->needsMeshUpdate ||
          opaqueMeshCache.find(chunkPosKey) == opaqueMeshCache.end()) {
        auto &cache = opaqueMeshCache[chunkPosKey];
        cache.vertices.clear();
        cache.indices.clear();
        cache.vertices.reserve(chunk->opaqueFaces.size());
        cache.indices.reserve(chunk->opaqueFaces.size());

        Vector3 chunkWorldPos{(float)chunk->xPos, 0, (float)chunk->zPos};

        for (uint32_t packed : chunk->opaqueFaces) {
          uint16_t posIndex;
          uint8_t side, light;
          uint16_t blockID;
          DrawnFace::Unpack(packed, posIndex, side, light, blockID);

          int lx = posIndex % ChunkPrefab::xSize;
          int ly = (posIndex / ChunkPrefab::xSize) % ChunkPrefab::ySize;
          int lz = posIndex / (ChunkPrefab::xSize * ChunkPrefab::ySize);

          Vector3 blockPos = {(float)lx, (float)ly, (float)lz};
          Vector3 worldPos = blockPos + chunkWorldPos;

          DVertex vert;
          vert.Position = worldPos;

          // Pack Data: side(3), tileIndex(16), light(4)
          Uint32 tileIndex = (Uint32)BlockDef[blockID].Textures[side];
          Uint32 packedData = (side & 0x7) | ((tileIndex & 0xFFFF) << 3) |
                              ((uint32_t(light << 1) & 0xF) << 19);
          vert.Data = *(float *)&packedData;

          cache.vertices.push_back(vert);
        }
        chunk->needsMeshUpdate = false;
      }

      auto &cache = opaqueMeshCache[chunkPosKey];
      if (cache.vertices.empty())
        continue;

      if (currentVertexOffset + cache.vertices.size() > maxVertices ||
          currentIndexOffset + cache.indices.size() > maxIndices)
        continue;

      memcpy(&Vertexdata[currentVertexOffset], cache.vertices.data(),
             cache.vertices.size() * sizeof(DVertex));

      currentVertexOffset += cache.vertices.size();
    }

    if (currentVertexOffset == 0)
      continue;

    mesh->OpaqueIndexCount = (int)currentVertexOffset;
    mesh->BaseVertex = (int)currentVertexOffset;
    mesh->BaseIndex = 0;

    SDL_UnmapGPUTransferBuffer(this->basicInitVars.GPU,
                               mesh->VertextransferBuffer);
    // SDL_UnmapGPUTransferBuffer(this->basicInitVars.GPU,
    // mesh->IndextransferBuffer);

    SDL_GPUTransferBufferLocation vLoc{mesh->VertextransferBuffer, 0};
    SDL_GPUBufferRegion vReg{mesh->VertexBuffer.buffer, 0,
                             (Uint32)(currentVertexOffset * sizeof(DVertex))};
    SDL_UploadToGPUBuffer(this->runTimeRenderVars.copyPass, &vLoc, &vReg, true);
  }
  // 4. Transparent Pass - Dedicated buffer (totalBuffers-1)
  struct TransparentFace {
    Vector3 pos;
    Uint8 side;
    Uint16 blockID;
    Uint8 light;
  };
  static std::vector<TransparentFace> cachedTransparentFaces;
  static Vector3 lastSortPos;
  static float lastSortRotY = -999.0f;

  std::vector<TransparentFace> transparentFaces;
  transparentFaces.reserve(500);

  bool anyTransparentDirty = false;
  for (auto &cd : visibleChunks) {
    ChunkPrefab *chunk = cd.chunk;
    anyTransparentDirty =
        (chunk->needsMeshUpdate && chunk->isGenerated) || anyTransparentDirty;

    Vector3 chunkWorldPos{(float)chunk->xPos, 0, (float)chunk->zPos};
    for (uint32_t packed : chunk->transparentFaces) {
      uint16_t posIndex;
      uint8_t side, light;
      uint16_t blockID;
      DrawnFace::Unpack(packed, posIndex, side, light, blockID);

      int lx = posIndex % ChunkPrefab::xSize;
      int ly = (posIndex / ChunkPrefab::xSize) % ChunkPrefab::ySize;
      int lz = posIndex / (ChunkPrefab::xSize * ChunkPrefab::ySize);

      Vector3 blockPos = {(float)lx, (float)ly, (float)lz};
      transparentFaces.push_back(
          {blockPos + chunkWorldPos, side, blockID, (uint8_t)(light << 1)});
    }
  }

  Mesh *tMesh = &this->Terrain[this->totalBuffers - 1];
  tMesh->OpaqueIndexCount = 0;

  if (transparentFaces.empty()) {
    tMesh->TransparentIndexCount = 0;
    cachedTransparentFaces.clear();
    return;
  }
  DVertex *vData = (DVertex *)SDL_MapGPUTransferBuffer(
      this->basicInitVars.GPU, tMesh->VertextransferBuffer, true);
  // Uint32 *iData = (Uint32 *)SDL_MapGPUTransferBuffer(this->basicInitVars.GPU,
  // tMesh->IndextransferBuffer, true);

  if (vData) {
    size_t vOffset = 0;
    // size_t iOffset = 0;
    const size_t maxV = chunksPerBuffer * FacesPerChunk;

    for (auto &face : transparentFaces) {
      if (vOffset + 1 > maxV)
        break;

      DVertex vert;
      vert.Position = face.pos;

      Uint32 tileIndex = (Uint32)BlockDef[face.blockID].Textures[face.side];
      Uint32 packed = (face.side & 0x7) | ((tileIndex & 0xFFFF) << 3) |
                      ((face.light & 0xF) << 19);
      vert.Data = *(float *)&packed;
      vData[vOffset] = vert;

      vOffset += 1;
    }

    tMesh->TransparentIndexCount = (int)vOffset;
    tMesh->BaseVertex = (int)vOffset;
    SDL_UnmapGPUTransferBuffer(this->basicInitVars.GPU,
                               tMesh->VertextransferBuffer);
    // SDL_UnmapGPUTransferBuffer(this->basicInitVars.GPU,
    // tMesh->IndextransferBuffer);

    SDL_GPUTransferBufferLocation vLoc{tMesh->VertextransferBuffer, 0};
    SDL_GPUBufferRegion vReg{tMesh->VertexBuffer.buffer, 0,
                             (Uint32)(vOffset * sizeof(DVertex))};
    SDL_UploadToGPUBuffer(this->runTimeRenderVars.copyPass, &vLoc, &vReg, true);
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
    PrintError("Error in submitting the cmd buffer");
    return;
  }

  this->runTimeRenderVars.cmdRender =
      SDL_AcquireGPUCommandBuffer(this->basicInitVars.GPU);

  SDL_WaitAndAcquireGPUSwapchainTexture(
      this->runTimeRenderVars.cmdRender, this->basicInitVars.window,
      &this->runTimeRenderVars.swap_texture, &this->basicInitVars.Width,
      &this->basicInitVars.Height);

  if (this->runTimeRenderVars.swap_texture == NULL) {
    PrintError("La swap_texture no s'ha fet be");
    SDL_SubmitGPUCommandBuffer(this->runTimeRenderVars.cmdRender);
    return;
  }
  SDL_GPUColorTargetInfo color_target_info;
  SDL_zero(color_target_info);
  color_target_info.clear_color = SDL_FColor{0.1f, 0.79f, 1.0f, 1.0f};
  color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
  color_target_info.store_op = SDL_GPU_STOREOP_STORE;
  color_target_info.texture = this->runTimeRenderVars.swap_texture;

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

  SDL_PushGPUVertexUniformData(
      this->runTimeRenderVars.cmdRender, 0, proj.getColumnMajorData().data(),
      sizeof(float) * proj.getColumnMajorData().size());

  SDL_PushGPUVertexUniformData(
      this->runTimeRenderVars.cmdRender, 1, view.getColumnMajorData().data(),
      sizeof(float) * view.getColumnMajorData().size());

  Uint32 water =
      (this->gameManager.GetChunkManager().GetBlockID(player.Position) == 5)
          ? 1
          : 0;

  SDL_PushGPUFragmentUniformData(this->runTimeRenderVars.cmdRender, 0, &water,
                                 sizeof(Uint32));

  this->runTimeRenderVars.pass =
      SDL_BeginGPURenderPass(this->runTimeRenderVars.cmdRender,
                             &color_target_info, 1, &depth_target_info);

  if (this->TextureAtlas && this->Sampler) {
    SDL_GPUTextureSamplerBinding samplerBinding = {this->TextureAtlas,
                                                   this->Sampler};
    SDL_BindGPUFragmentSamplers(this->runTimeRenderVars.pass, 0,
                                &samplerBinding, 1);
  }

  SDL_BindGPUGraphicsPipeline(this->runTimeRenderVars.pass,
                              this->pipelineInitVars.graphicsPipeline);

  SDL_BindGPUIndexBuffer(this->runTimeRenderVars.pass, &this->QuadIndexBinding,
                         SDL_GPU_INDEXELEMENTSIZE_32BIT);

  for (auto &mesh : this->Terrain) {
    int faceCount = mesh.OpaqueIndexCount + mesh.TransparentIndexCount;
    if (faceCount == 0)
      continue;

    SDL_BindGPUVertexBuffers(this->runTimeRenderVars.pass, 0,
                             &mesh.VertexBuffer, 1);
    SDL_DrawGPUIndexedPrimitives(this->runTimeRenderVars.pass, 6, faceCount, 0,
                                 0, 0);
  }

  // DrawPlayers(players);

  SDL_EndGPURenderPass(this->runTimeRenderVars.pass);
}
void Renderer::DrawPlayers(std::vector<Player> &players) {
  if (players.size() == 1)
    return;

  std::vector<DVertex> verts;
  std::vector<Uint32> indices;

  bool first = true;
  for (const auto &p : players) {
    if (first) {
      first = false;
      continue;
    }
    Vector3 pos = p.Position;

    for (int side = 0; side < 6; side++) {
      Uint32 base = verts.size();
      for (int i = 0; i < 4; i++) {
        Vector3 p = EntityDef[(int)EntityType::PLAYER].Model[side][i] + pos;
        // Dummy UVs since players don't use them yet
        Uint32 packedData = (side & 0x7) | (0 << 3) | (15 << 19);
        DVertex vert;
        vert.Position = Vector3(p.x, p.y, p.z);
        vert.Data = *(float *)&packedData;
        verts.push_back(vert);
      }
      indices.push_back(base + 0);
      indices.push_back(base + 2);
      indices.push_back(base + 1);
      indices.push_back(base + 1);
      indices.push_back(base + 2);
      indices.push_back(base + 3);
    }
  }

  if (verts.empty())
    return;

  void *vData =
      SDL_MapGPUTransferBuffer(basicInitVars.GPU, EntityTransferBuffer, true);
  SDL_memcpy(vData, verts.data(), verts.size() * sizeof(DVertex));
  SDL_UnmapGPUTransferBuffer(basicInitVars.GPU, EntityTransferBuffer);

  void *iData = SDL_MapGPUTransferBuffer(basicInitVars.GPU,
                                         EntityIndexTransferBuffer, true);
  SDL_memcpy(iData, indices.data(), indices.size() * sizeof(Uint32));
  SDL_UnmapGPUTransferBuffer(basicInitVars.GPU, EntityIndexTransferBuffer);

  SDL_GPUCommandBuffer *cmd = SDL_AcquireGPUCommandBuffer(basicInitVars.GPU);
  SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
  SDL_GPUTransferBufferLocation vSrc = {EntityTransferBuffer, 0};
  SDL_GPUBufferRegion vDst = {EntityBuffer, 0,
                              (Uint32)(verts.size() * sizeof(DVertex))};
  SDL_UploadToGPUBuffer(copy, &vSrc, &vDst, true);
  SDL_GPUTransferBufferLocation iSrc = {EntityIndexTransferBuffer, 0};
  SDL_GPUBufferRegion iDst = {EntityIndexBuffer, 0,
                              (Uint32)(indices.size() * sizeof(Uint32))};
  SDL_UploadToGPUBuffer(copy, &iSrc, &iDst, true);
  SDL_EndGPUCopyPass(copy);
  SDL_SubmitGPUCommandBuffer(cmd);

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

  PrintLog("Updating viewport to: " + std::to_string(drawableWidth) + "x" +
           std::to_string(drawableHeight));

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
void Renderer::OpenInventory(bool craftingTable) {
  this->gameManager.SetUi(!this->gameManager.GetUsingUI());
  this->uiRuntimeVars.bigInventory = this->gameManager.GetUsingUI();
  this->uiRuntimeVars.isCraftingTable = craftingTable;
  SDL_SetWindowRelativeMouseMode(this->basicInitVars.window,
                                 !this->uiRuntimeVars.bigInventory);
  if (!this->uiRuntimeVars.bigInventory)
    this->uiRuntimeVars.isCraftingTable = false;
}
void Renderer::EventManager(Player &player, int &inventorySlot) {
  while (SDL_PollEvent(&this->basicInitVars.event)) {
    switch (this->basicInitVars.event.type) {
    case SDL_EVENT_QUIT:
      HandleQuit();
      break;
    case SDL_EVENT_WINDOW_RESIZED:
      UpdateViewportAndProjection();
      break;
    case SDL_EVENT_MOUSE_WHEEL:
      HandleMouseWheel(inventorySlot);
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      HandleMouseButtonDown(player);
      break;
    case SDL_EVENT_MOUSE_MOTION:
      HandleMouseMotion(player);
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
      HandleMouseButtonUp(player);
      break;
    case SDL_EVENT_KEY_DOWN:
      HandleKeyDown(player, inventorySlot);
      break;
    }
  }
}

void Renderer::HandleQuit() { this->gameManager.GetGameClient().Quit(); }

void Renderer::HandleMouseWheel(int &inventorySlot) {
  int delta = this->basicInitVars.event.wheel.y;
  if (delta > 0)
    inventorySlot = (inventorySlot + 7) % 9; // scroll up   = previous
  else if (delta < 0)
    inventorySlot = (inventorySlot + 1) % 9; // scroll down = next
}

Vector2 Renderer::ScreenToNDC(float mx, float my) const {
  float ndc_x = (mx / (float)this->basicInitVars.Width) * 2.0f - 1.0f;
  float ndc_y = 1.0f - (my / (float)this->basicInitVars.Height) * 2.0f;
  return {ndc_x, ndc_y};
}

int Renderer::GetHoveredBoxIndex(Vector2 ndc,
                                 const std::vector<InventoryBox> &boxes) const {
  for (const auto &box : boxes) {
    bool inX = ndc.x >= box.xNDC && ndc.x <= box.xNDC + box.wNDC;
    bool inY = ndc.y >= box.yNDC && ndc.y <= box.yNDC + box.hNDC;
    if (inX && inY)
      return box.index;
  }
  return -1;
}

void Renderer::HandleMouseButtonDown(Player &player) {
  if (!this->uiRuntimeVars.bigInventory)
    return;

  auto ndc = ScreenToNDC(this->basicInitVars.event.button.x,
                         this->basicInitVars.event.button.y);

  float panelX, panelY, panelW, panelH;
  auto boxes = BuildInventoryBoxes(this->runTimeRenderVars.aspect,
                                   this->uiRuntimeVars.isCraftingTable, panelX,
                                   panelY, panelW, panelH);

  int slotIndex = GetHoveredBoxIndex(ndc, boxes);
  if (slotIndex == -1)
    return;

  // Init drag state
  g_initialClickSlot = slotIndex;
  g_draggedSlots.clear();
  g_justPickedUp = false;

  bool slotCompatible = g_heldItem.Type == 0 ||
                        player.inventory[slotIndex].Type == 0 ||
                        player.inventory[slotIndex].Type == g_heldItem.Type;

  if (slotCompatible)
    g_draggedSlots.push_back(slotIndex);

  uint8_t btn = this->basicInitVars.event.button.button;
  if (btn == SDL_BUTTON_LEFT)
    HandleLeftClickDown(player, slotIndex);
  else if (btn == SDL_BUTTON_RIGHT)
    HandleRightClickDown(player, slotIndex);
}

void Renderer::HandleLeftClickDown(Player &player, int slotIndex) {
  g_isLeftClickDragging = true;
  g_isRightClickDragging = false;

  if (g_heldItem.Type != 0)
    return; // already holding something

  // Pick up entire stack
  g_heldItem = player.inventory[slotIndex];
  player.inventory[slotIndex] = {0, 0};
  g_justPickedUp = true;

  // Crafting result slot: consume one of each ingredient
  if (slotIndex == CRAFTING_RESULT_SLOT && g_heldItem.Type != 0) {
    ConsumeIngredients(player);
  }

  if (this->uiRuntimeVars.isFurnace)
    UpdateSmeltingSelection(player);
  else
    UpdateCraftingSelection(player);
}

void Renderer::HandleRightClickDown(Player &player, int slotIndex) {
  g_isRightClickDragging = true;
  g_isLeftClickDragging = false;

  if (g_heldItem.Type == 0) {
    PickUpHalfStack(player, slotIndex);
  } else {
    PlaceOneItem(player, slotIndex);
  }
}

void Renderer::PickUpHalfStack(Player &player, int slotIndex) {
  auto &slot = player.inventory[slotIndex];
  if (slot.Type == 0)
    return;

  if (slotIndex == CRAFTING_RESULT_SLOT) {
    // Craft exactly one
    g_heldItem = slot;
    slot = {0, 0};
    ConsumeIngredients(player);
  } else {
    // Pick up the top half
    int take = (slot.Amount + 1) / 2;
    g_heldItem = {(short)take, slot.Type};
    slot.Amount -= take;
    if (slot.Amount <= 0)
      slot.Type = 0;
  }

  g_justPickedUp = true;
  if (this->uiRuntimeVars.isFurnace)
    UpdateSmeltingSelection(player);
  else
    UpdateCraftingSelection(player);
}

void Renderer::PlaceOneItem(Player &player, int slotIndex) {
  if (slotIndex == CRAFTING_RESULT_SLOT)
    return;

  auto &slot = player.inventory[slotIndex];
  bool compatible = slot.Type == 0 || slot.Type == g_heldItem.Type;
  if (!compatible)
    return;

  slot.Type = g_heldItem.Type;
  slot.Amount++;
  g_heldItem.Amount--;
  if (g_heldItem.Amount <= 0)
    g_heldItem = {0, 0};

  if (this->uiRuntimeVars.isFurnace)
    UpdateSmeltingSelection(player);
  else
    UpdateCraftingSelection(player);
}

void Renderer::ConsumeIngredients(Player &player) {
  for (int s = CRAFTING_INPUT_FIRST; s <= CRAFTING_INPUT_LAST; s++) {
    if (player.inventory[s].Amount <= 0)
      continue;
    player.inventory[s].Amount--;
    if (player.inventory[s].Amount == 0) {
      player.inventory[s].Type = 0;
    }
  }
}

void Renderer::HandleMouseMotion(Player &player) {
  if (!this->uiRuntimeVars.bigInventory)
    return;
  if (!g_isLeftClickDragging && !g_isRightClickDragging)
    return;

  auto ndc = ScreenToNDC(this->basicInitVars.event.motion.x,
                         this->basicInitVars.event.motion.y);

  float panelX, panelY, panelW, panelH;
  auto boxes = BuildInventoryBoxes(this->runTimeRenderVars.aspect,
                                   this->uiRuntimeVars.isCraftingTable, panelX,
                                   panelY, panelW, panelH);

  int slotIndex = GetHoveredBoxIndex(ndc, boxes);
  if (slotIndex == -1)
    return;

  // Skip already-visited slots and the result slot
  bool alreadyVisited = std::find(g_draggedSlots.begin(), g_draggedSlots.end(),
                                  slotIndex) != g_draggedSlots.end();

  bool compatible = (slotIndex != CRAFTING_RESULT_SLOT) &&
                    (g_heldItem.Type != 0) &&
                    (player.inventory[slotIndex].Type == 0 ||
                     player.inventory[slotIndex].Type == g_heldItem.Type);

  if (alreadyVisited || !compatible)
    return;

  if (g_isLeftClickDragging) {
    g_draggedSlots.push_back(slotIndex);
  } else if (g_isRightClickDragging && g_heldItem.Amount > 0) {
    g_draggedSlots.push_back(slotIndex);
    PlaceOneItem(player, slotIndex);
  }
}

void Renderer::HandleMouseButtonUp(Player &player) {
  if (!this->uiRuntimeVars.bigInventory)
    return;

  if (this->basicInitVars.event.button.button == SDL_BUTTON_LEFT &&
      g_isLeftClickDragging)
    FinalizeLeftClickDrag(player);

  // Reset all drag state
  g_isLeftClickDragging = false;
  g_isRightClickDragging = false;
  g_draggedSlots.clear();
  g_initialClickSlot = -1;
  g_justPickedUp = false;
}

void Renderer::FinalizeLeftClickDrag(Player &player) {
  if (g_justPickedUp || g_heldItem.Type == 0)
    return;

  if (g_draggedSlots.size() > 1) {
    DistributeAcrossSlots(player);
  } else {
    // Simple click: swap or merge into single slot
    int i = g_draggedSlots.empty() ? g_initialClickSlot : g_draggedSlots[0];
    if (i == -1 || i == CRAFTING_RESULT_SLOT)
      return;

    if (player.inventory[i].Type == g_heldItem.Type) {
      player.inventory[i].Amount += g_heldItem.Amount;
      g_heldItem = {0, 0};
    } else {
      std::swap(g_heldItem, player.inventory[i]);
    }

    if (this->uiRuntimeVars.isFurnace)
      UpdateSmeltingSelection(player);
    else
      UpdateCraftingSelection(player);
  }
}

void Renderer::DistributeAcrossSlots(Player &player) {
  std::vector<int> targets;
  for (int s : g_draggedSlots)
    if (s != CRAFTING_RESULT_SLOT)
      targets.push_back(s);

  if (targets.empty())
    return;

  int amountPer = g_heldItem.Amount / (int)targets.size();
  int remainder = g_heldItem.Amount % (int)targets.size();

  for (size_t j = 0; j < targets.size(); j++) {
    int toAdd = amountPer + (j < (size_t)remainder ? 1 : 0);
    if (toAdd <= 0)
      continue;
    player.inventory[targets[j]].Type = g_heldItem.Type;
    player.inventory[targets[j]].Amount += toAdd;
  }

  g_heldItem = {0, 0};
  if (this->uiRuntimeVars.isFurnace)
    UpdateSmeltingSelection(player);
  else
    UpdateCraftingSelection(player);
}

void Renderer::HandleKeyDown(Player &player, int &inventorySlot) {
  SDL_Keycode key = this->basicInitVars.event.key.scancode;

  switch (key) {
  case SDL_SCANCODE_ESCAPE:
    HandleEscapeKey();
    break;
  case SDL_SCANCODE_F11:
    HandleF11Key();
    break;
  case SDL_SCANCODE_F3:
    this->uiRuntimeVars.showDebug = !this->uiRuntimeVars.showDebug;
    break;
  case SDL_SCANCODE_E:
    HandleEKey();
    break;
  }
}

void Renderer::HandleEscapeKey() {
  if (!this->gameManager.GetUsingUI())
    return;
  this->uiRuntimeVars.bigInventory = false;
  this->uiRuntimeVars.isCraftingTable = false;
  this->uiRuntimeVars.isFurnace = false;
  this->gameManager.SetUi(false);
  SDL_SetWindowRelativeMouseMode(this->basicInitVars.window, true);
}

void Renderer::HandleF11Key() {
  this->uiRuntimeVars.fullScreen = !this->uiRuntimeVars.fullScreen;
  SDL_SetWindowFullscreen(this->basicInitVars.window,
                          this->uiRuntimeVars.fullScreen);
  UpdateViewportAndProjection();
}

void Renderer::HandleEKey() { this->OpenInventory(false); }
void Renderer::MainRenderLoop(std::vector<Slot> &inventory, int inventorySlot,
                              std::vector<Player> &players) {
  EventManager(players[0], inventorySlot);

  this->runTimeRenderVars.aspect =
      (float)this->basicInitVars.Width / (float)this->basicInitVars.Height;
  Frustum worldFrustum = Frustum::createFrustumFromCamera(
      this->runTimeRenderVars.aspect, tanHalfFov, Znear, Zfar);
  worldFrustum.transformToWorldSpace(players[0].Position, players[0].Rotation);
  this->frustum = worldFrustum;

  DrawBg(players);
  DrawUI(inventory, inventorySlot, players[0]);

  if (!SDL_SubmitGPUCommandBuffer(this->runTimeRenderVars.cmdRender)) {
    PrintError("Failed to submit render command buffer");
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
    PrintWarning("Unknown shader type: %s" + std::string(filename));
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
    PrintWarning("No supported shader format found!");
    return NULL;
  }

  size_t code_size;
  void *code = SDL_LoadFile(fullpath, &code_size);
  if (!code) {
    PrintWarning("Couldn't load shader file %s: %s" + std::string(fullpath) +
                 std::string(SDL_GetError()));
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
    PrintWarning("Couldn't create shader %s: %s" + std::string(filename) +
                 SDL_GetError());
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
    PrintWarning("Failed to create depth texture: %s" +
                 std::string(SDL_GetError()));
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
    PrintLog("GPU Driver: " +
             std::string(SDL_GetGPUDeviceDriver(this->basicInitVars.GPU)));
  } else {
    PrintError("Failed to create GPU device: " + std::string(SDL_GetError()));
    return;
  }

  this->basicInitVars.window =
      SDL_CreateWindow("Bit Miner", this->basicInitVars.Width,
                       this->basicInitVars.Height, SDL_WINDOW_RESIZABLE);
  if (this->basicInitVars.window == nullptr) {
    PrintError("Error creating basicInitVars.window: " +
               std::string(SDL_GetError()));
    SDL_Quit();
    assert(false);
  }

  if (!SDL_ClaimWindowForGPUDevice(this->basicInitVars.GPU,
                                   this->basicInitVars.window)) {
    PrintWarning("Error claiming window for GPU device: " +
                 std::string(SDL_GetError()));
  }
  this->basicInitVars.event = {};
  SDL_SetWindowRelativeMouseMode(basicInitVars.window, true);

  if (!TTF_WasInit() && !TTF_Init()) {
    PrintWarning("Failed to initialize SDL_ttf: " +
                 std::string(SDL_GetError()));
  }
}
void Renderer::GenerateBuffer() {
  // Calculate buffer packing
  int totalChunks = (RenderDistance * 2 + 1) * (RenderDistance * 2 + 1) / 2.5f;
  for (int i = std::sqrt(totalChunks) + 1; i > 0; i--) {
    if (totalChunks % i) {
      chunksPerBuffer = i;
      break;
    }
  }
  this->totalBuffers = totalChunks / chunksPerBuffer;

  // 1 vertex per face now (was 4), no per-mesh index buffer needed
  const Uint32 singleChunkVertexSize = sizeof(DVertex) * FacesPerChunk;
  const Uint32 packedVertexSize = singleChunkVertexSize * chunksPerBuffer;

  PrintLog("Vertex size per buffer: " + std::to_string(packedVertexSize) +
           "\n Total buffers: " + std::to_string(this->totalBuffers) +
           "\t Chunks per buffer: " + std::to_string(this->chunksPerBuffer));

  for (int i = 0; i < this->totalBuffers; i++) {
    Mesh mesh{};

    SDL_GPUBufferCreateInfo vertexInfo = {};
    vertexInfo.size = packedVertexSize;
    vertexInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    mesh.VertexBuffer.buffer =
        SDL_CreateGPUBuffer(this->basicInitVars.GPU, &vertexInfo);
    mesh.VertexBuffer.offset = 0;

    SDL_GPUTransferBufferCreateInfo vertexTransferInfo{};
    vertexTransferInfo.size = packedVertexSize;
    vertexTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    mesh.VertextransferBuffer = SDL_CreateGPUTransferBuffer(
        this->basicInitVars.GPU, &vertexTransferInfo);

    this->Terrain.push_back(mesh);
  }

  // Single static index buffer shared by all draw calls: {0,1,2, 1,3,2}
  // gl_VertexID reads these as corner IDs (0-3), gl_InstanceIndex = face index
  // Clean existing buffer if it exists
  if (this->QuadIndexBinding.buffer) {
    SDL_ReleaseGPUBuffer(this->basicInitVars.GPU,
                         this->QuadIndexBinding.buffer);
    this->QuadIndexBinding.buffer = nullptr;
  }

  Uint32 quadIndices[6] = {0, 1, 2, 1, 3, 2};

  SDL_GPUBufferCreateInfo quadIdxInfo = {};
  quadIdxInfo.size = sizeof(quadIndices);
  quadIdxInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
  this->QuadIndexBinding.buffer =
      SDL_CreateGPUBuffer(this->basicInitVars.GPU, &quadIdxInfo);

  if (!this->QuadIndexBinding.buffer) {
    PrintError("Failed to create quad index buffer: " +
               std::string(SDL_GetError()));
    return;
  }

  SDL_GPUTransferBufferCreateInfo transferInfo = {};
  transferInfo.size = sizeof(quadIndices);
  transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  SDL_GPUTransferBuffer *transferBuf =
      SDL_CreateGPUTransferBuffer(this->basicInitVars.GPU, &transferInfo);

  Uint32 *mapped = (Uint32 *)SDL_MapGPUTransferBuffer(this->basicInitVars.GPU,
                                                      transferBuf, false);
  memcpy(mapped, quadIndices, sizeof(quadIndices));
  SDL_UnmapGPUTransferBuffer(this->basicInitVars.GPU, transferBuf);

  SDL_GPUCommandBuffer *cmd =
      SDL_AcquireGPUCommandBuffer(this->basicInitVars.GPU);
  SDL_GPUCopyPass *copy = SDL_BeginGPUCopyPass(cmd);
  SDL_GPUTransferBufferLocation src = {transferBuf, 0};
  SDL_GPUBufferRegion dst = {this->QuadIndexBinding.buffer, 0,
                             sizeof(quadIndices)};
  SDL_UploadToGPUBuffer(copy, &src, &dst, false);
  SDL_EndGPUCopyPass(copy);
  SDL_SubmitGPUCommandBuffer(cmd);
  SDL_ReleaseGPUTransferBuffer(this->basicInitVars.GPU, transferBuf);

  this->QuadIndexBinding = {this->QuadIndexBinding.buffer, 0};
}
void Renderer::VertexGPUInit() {
  auto &desc = this->pipelineInitVars.vertex_buffer_desc;
  auto &attrs = this->pipelineInitVars.vertex_attributes;

  desc.slot = 0;
  desc.input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
  desc.instance_step_rate = 0;
  desc.pitch = sizeof(DVertex);

  auto setAttr = [&](int loc, SDL_GPUVertexElementFormat fmt, Uint32 offset) {
    attrs[loc].buffer_slot = 0;
    attrs[loc].location = loc;
    attrs[loc].format = fmt;
    attrs[loc].offset = offset;
  };
  setAttr(0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
  setAttr(1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, sizeof(float) * 3);
}
void Renderer::UIVertexGPUInit() {
  auto &desc = this->pipelineInitVars.UIvertex_buffer_desc;
  auto &attrs = this->pipelineInitVars.UIvertex_attributes;

  // Buffer descriptor
  desc.slot = 0;
  desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
  desc.instance_step_rate = 0;
  desc.pitch = sizeof(Vertex);

  // Helper to reduce repetition
  auto setAttr = [&](int loc, SDL_GPUVertexElementFormat fmt, Uint32 offset) {
    attrs[loc].buffer_slot = 0;
    attrs[loc].location = loc;
    attrs[loc].format = fmt;
    attrs[loc].offset = offset;
  };

  //                loc  format                                offset
  setAttr(0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, 0);
  setAttr(1, SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM, sizeof(float) * 3);
  setAttr(2, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT, sizeof(float) * 3 + 4);
}
void Renderer::LoadTexture() {
  if (!this->basicInitVars.GPU) {
    PrintWarning("Cannot load texture: GPU device not initialized");
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
    PrintWarning("Failed to load texture at %s: %s" + std::string(fullPath) +
                 SDL_GetError());
    return;
  }

  PrintLog("Loaded texture: %s (%dx%d)" + std::string(fullPath) +
           std::to_string(surface->w) + std::to_string(surface->h));

  // Convert to a guaranteed RGBA8 per-pixel format
  SDL_Surface *rgbaSurface =
      SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
  SDL_DestroySurface(surface);
  if (!rgbaSurface) {
    PrintWarning("Failed to convert surface: " + std::string(SDL_GetError()));
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
    PrintWarning("Failed to create GPU texture: " +
                 std::string(SDL_GetError()));
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
    PrintError("Failed to create transfer buffer: %s" +
               std::string(SDL_GetError()));
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
    PrintSuccesfull("Sampler created successfully");
  } else {
    PrintWarning("Failed to create sampler: %s" + std::string(SDL_GetError()));
  }

  // Load Font
  const char *fontPathRelative = "assets/square_pixel-7.ttf";
  char fullFontPath[512];
  if (basePath) {
    SDL_snprintf(fullFontPath, sizeof(fullFontPath), "%s%s", basePath,
                 fontPathRelative);
  } else {
    SDL_strlcpy(fullFontPath, fontPathRelative, sizeof(fullFontPath));
  }

  uiVars.font =
      TTF_OpenFont(fullFontPath, 48); // Sharper text at higher resolution
  if (!uiVars.font) {
    PrintWarning("Failed to load font at %s: %s" + std::string(fullFontPath) +
                 SDL_GetError());
    return;
  }
  PrintLog("Loaded font: %s" + std::string(fullFontPath));

  // Create font atlas (Alphanumeric + basic symbols)
  const char *fontChars =
      " 0123456789:.XYZ/-abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  SDL_Surface *digitSurface =
      TTF_RenderText_Blended(uiVars.font, fontChars, 0, {255, 255, 255, 255});
  if (digitSurface) {
    PrintLog("Digit surface created: %dx%d, format: %s" +
             std::to_string(digitSurface->w) + std::to_string(digitSurface->h) +
             SDL_GetPixelFormatName(digitSurface->format));
    SDL_Surface *rgbaDigitSurface =
        SDL_ConvertSurface(digitSurface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(digitSurface);
    digitSurface = rgbaDigitSurface;

    SDL_GPUTextureCreateInfo fontTexInfo = {};
    fontTexInfo.type = SDL_GPU_TEXTURETYPE_2D;
    fontTexInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    fontTexInfo.width = (Uint32)digitSurface->w;
    fontTexInfo.height = (Uint32)digitSurface->h;
    fontTexInfo.layer_count_or_depth = 1;
    fontTexInfo.num_levels = 1;
    fontTexInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;

    uiVars.fontTexture =
        SDL_CreateGPUTexture(this->basicInitVars.GPU, &fontTexInfo);
    if (uiVars.fontTexture) {
      Uint32 fontTexSize = (Uint32)(digitSurface->w * digitSurface->h * 4);
      SDL_GPUTransferBufferCreateInfo fontTransferInfo = {};
      fontTransferInfo.size = fontTexSize;
      fontTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
      SDL_GPUTransferBuffer *fontTransferBuffer = SDL_CreateGPUTransferBuffer(
          this->basicInitVars.GPU, &fontTransferInfo);

      if (fontTransferBuffer) {
        void *fontData = SDL_MapGPUTransferBuffer(this->basicInitVars.GPU,
                                                  fontTransferBuffer, false);
        if (fontData) {
          // Copy row by row to handle pitch
          for (int y = 0; y < digitSurface->h; y++) {
            SDL_memcpy((Uint8 *)fontData + (y * digitSurface->w * 4),
                       (Uint8 *)digitSurface->pixels +
                           (y * digitSurface->pitch),
                       digitSurface->w * 4);
          }
          SDL_UnmapGPUTransferBuffer(this->basicInitVars.GPU,
                                     fontTransferBuffer);
        }

        SDL_GPUCommandBuffer *fontCmd =
            SDL_AcquireGPUCommandBuffer(this->basicInitVars.GPU);
        if (fontCmd) {
          SDL_GPUCopyPass *fontCopyPass = SDL_BeginGPUCopyPass(fontCmd);
          SDL_GPUTextureTransferInfo fontTexTransfer = {};
          fontTexTransfer.transfer_buffer = fontTransferBuffer;
          fontTexTransfer.pixels_per_row = (Uint32)digitSurface->w;
          fontTexTransfer.rows_per_layer = (Uint32)digitSurface->h;

          SDL_GPUTextureRegion fontTexRegion = {};
          fontTexRegion.texture = uiVars.fontTexture;
          fontTexRegion.w = (Uint32)digitSurface->w;
          fontTexRegion.h = (Uint32)digitSurface->h;
          fontTexRegion.d = 1;

          SDL_UploadToGPUTexture(fontCopyPass, &fontTexTransfer, &fontTexRegion,
                                 false);
          SDL_EndGPUCopyPass(fontCopyPass);
          SDL_SubmitGPUCommandBuffer(fontCmd);
          SDL_WaitForGPUIdle(this->basicInitVars.GPU);
        }
        SDL_ReleaseGPUTransferBuffer(this->basicInitVars.GPU,
                                     fontTransferBuffer);
      }
      PrintSuccesfull("Font texture uploaded successfully");
    }
    SDL_DestroySurface(digitSurface);
  }

  SDL_GPUSamplerCreateInfo fontSamplerInfo = {};
  fontSamplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
  fontSamplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
  fontSamplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
  fontSamplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
  uiVars.fontSampler =
      SDL_CreateGPUSampler(this->basicInitVars.GPU, &fontSamplerInfo);
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
    PrintError("Couldn't load vertex shader: %s" + std::string(SDL_GetError()));
  }
  this->pipelineInitVars.fragment_shader =
      LoadShader(this->basicInitVars.GPU, "Shader.frag", 1, 1, 0, 0);
  if (!this->pipelineInitVars.fragment_shader) {
    PrintError("Couldn't load fragment shader: %s" +
               std::string(SDL_GetError()));
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
  UIVertexGPUInit();

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
      &this->pipelineInitVars.UIvertex_buffer_desc;
  ui_desc.vertex_input_state.num_vertex_attributes = 3;
  ui_desc.vertex_input_state.vertex_attributes =
      this->pipelineInitVars.UIvertex_attributes;

  if (!ui_vert || !ui_frag) {
    SDL_Log("UI Shaders failed to load! vert: %p, frag: %p", (void *)ui_vert,
            (void *)ui_frag);
  } else {
    this->pipelineInitVars.uiPipeline =
        SDL_CreateGPUGraphicsPipeline(this->basicInitVars.GPU, &ui_desc);
    if (!this->pipelineInitVars.uiPipeline) {
      PrintError("Failed to create UI pipeline: %s" +
                 std::string(SDL_GetError()));
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

  // Text Pipeline
  SDL_GPUShader *text_vert =
      LoadShader(this->basicInitVars.GPU, "Text.vert", 0, 0, 0, 0);
  SDL_GPUShader *text_frag =
      LoadShader(this->basicInitVars.GPU, "Text.frag", 1, 0, 0, 0);

  if (text_vert && text_frag) {
    SDL_GPUGraphicsPipelineCreateInfo text_desc = ui_desc;
    text_desc.vertex_shader = text_vert;
    text_desc.fragment_shader = text_frag;

    this->pipelineInitVars.textPipeline =
        SDL_CreateGPUGraphicsPipeline(this->basicInitVars.GPU, &text_desc);
    if (this->pipelineInitVars.textPipeline) {
      PrintSuccesfull("Text pipeline created successfully");
    } else {
      PrintWarning("Failed to create text pipeline: %s" +
                   std::string(SDL_GetError()));
    }

    SDL_ReleaseGPUShader(this->basicInitVars.GPU, text_vert);
    SDL_ReleaseGPUShader(this->basicInitVars.GPU, text_frag);
  } else {
    PrintWarning("Failed to load text shaders!");
  }

  // Create Text Buffer

  // Create Text Buffer
  SDL_GPUBufferCreateInfo textBufferInfo = {};
  textBufferInfo.size = sizeof(Vertex) * 4096;
  textBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  this->uiVars.textVertexBuffer =
      SDL_CreateGPUBuffer(this->basicInitVars.GPU, &textBufferInfo);

  SDL_GPUTransferBufferCreateInfo textTransferInfo = {};
  textTransferInfo.size = sizeof(Vertex) * 4096;
  textTransferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  this->uiVars.textVertexTransferBuffer =
      SDL_CreateGPUTransferBuffer(this->basicInitVars.GPU, &textTransferInfo);

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
Renderer::Renderer(GameManager &manager) : gameManager(manager) {

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    PrintError("SDL_Init failed: " + std::string(SDL_GetError()));

  TextureAtlas = nullptr;
  Sampler = nullptr;
  SDL_zero(uiVars);

  Init();
  GenerateBuffer();
  ColorTargetDes();
  LoadTexture();
  PipelineInit();

  if (!this->pipelineInitVars.graphicsPipeline ||
      !this->pipelineInitVars.transparentPipeline) {
    PrintError("Failed to create pipelines: %s" + std::string(SDL_GetError()));
  }

  SDL_ReleaseGPUShader(this->basicInitVars.GPU,
                       this->pipelineInitVars.vertex_shader);
  SDL_ReleaseGPUShader(this->basicInitVars.GPU,
                       this->pipelineInitVars.fragment_shader);

  UpdateViewportAndProjection();
}