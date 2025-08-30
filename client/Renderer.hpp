#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <cmath>
#include <vector>

#include "../core/Chunk.hpp"
#include "../core/ChunkManager.hpp"
#include "../core/common.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef PI
#define PI M_PI
#endif

struct Plane {
    Vector3 normal = {0.f, 1.f, 0.f};
    float distance = 0;

    float getSignedDistanceToPlane(const Vector3& point) const {
        return normal.Dot(point) - distance;
    }
};

struct Frustum {
    Plane topFace, bottomFace, rightFace, leftFace, farFace, nearFace;

    Frustum createFrustumFromCamera(float aspect, float fovY, float Znear, float Zfar) const {
        Frustum frustum;

        float tanHalfFovY = tan(fovY * 0.5f);
        float halfVSide = Zfar * tanHalfFovY;
        float halfHSide = halfVSide * aspect;

        Vector3 camForward = {0, 0, 1};
        Vector3 camRight = {1, 0, 0};
        Vector3 camUp = {0, 1, 0};

        Vector3 farCenter = camForward * Zfar;

        // Near and far planes
        frustum.nearFace.normal = camForward;
        frustum.nearFace.distance = Znear;

        frustum.farFace.normal = camForward * -1;
        frustum.farFace.distance = Zfar;

        Vector3 origin = {0, 0, 0};

        // Right plane
        Vector3 rightPoint = farCenter + (camRight * halfHSide);
        Vector3 rightTopPoint = rightPoint + (camUp * halfVSide);
        frustum.rightFace.normal = (rightTopPoint - origin).Cross(rightPoint - origin).Normalized();

        // Left plane
        Vector3 leftPoint = farCenter + (camRight * -halfHSide);
        Vector3 leftTopPoint = leftPoint + (camUp * halfVSide);
        frustum.leftFace.normal = (leftPoint - origin).Cross(leftTopPoint - origin).Normalized();

        // Top plane
        Vector3 topPoint = farCenter + (camUp * halfVSide);
        Vector3 topRightPoint = topPoint + (camRight * halfHSide);
        frustum.topFace.normal = (topPoint - origin).Cross(topRightPoint - origin).Normalized();

        // Bottom plane
        Vector3 bottomPoint = farCenter + (camUp * -halfVSide);
        Vector3 bottomRightPoint = bottomPoint + (camRight * halfHSide);
        frustum.bottomFace.normal =
            (bottomRightPoint - origin).Cross(bottomPoint - origin).Normalized();

        return frustum;
    }
};

struct Volume {
    virtual ~Volume() = default;
    virtual bool isOnFrustum(const Frustum& camFrustum, const Vector3* points) const = 0;
};

struct AABB : public Volume {
    Vector3 center{0.0, 0.0, 0.0};
    Vector3 extents{0.0, 0.0, 0.0};

    AABB(const Vector3& min, const Vector3& max)
        : center{(max + min) * 0.5f}, extents{(max - min) * 0.5f} {}

    AABB(const Vector3& inCenter, float iI, float iJ, float iK)
        : center{inCenter}, extents{iI, iJ, iK} {}

    bool isOnFrustum(const Frustum& camFrustum, const Vector3* points) const override {
        // Check if all points are outside any single plane
        const Plane* planes[6] = {&camFrustum.nearFace,  &camFrustum.farFace,
                                  &camFrustum.rightFace, &camFrustum.leftFace,
                                  &camFrustum.topFace,   &camFrustum.bottomFace};

        for (int p = 0; p < 6; p++) {
            bool allOutside = true;
            for (int i = 0; i < 8; i++) {
                if (planes[p]->getSignedDistanceToPlane(points[i]) >= 0) {
                    allOutside = false;
                    break;
                }
            }
            if (allOutside) return false;  // All points outside this plane
        }
        return true;  // At least one point inside all planes
    }

    bool isOnOrForwardPlane(const Plane& plane, const Vector3& point) const {
        return plane.getSignedDistanceToPlane(point) >= 0;
    }
};

struct Mesh {
    SDL_GPUTransferBuffer* vertexTransferBuffer = nullptr;
    SDL_GPUTransferBuffer* indexTransferBuffer = nullptr;
    SDL_GPUBufferBinding vertexBuffer;
    SDL_GPUBufferBinding indexBuffer;
    int faces = 0;
};

struct Vertex {
    Vector3 position;
    Vector3 color;
};

class Renderer {
   private:
    SDL_Window* window = nullptr;
    SDL_GPUDevice* GPU = nullptr;
    SDL_GPURenderPass* renderPass = nullptr;
    SDL_Event event;
    TTF_Font* font = nullptr;
    SDL_GPUTexture* texture = nullptr;
    Vector3 screenSize = {800, 600, 0};
    ChunkManager chunkManager;
    int blockPixelSize = 50;
    bool fullScreen = false;
    Frustum frustum;
    unsigned int width = 800, height = 600;
    SDL_GPUCommandBuffer* cmdRender = nullptr;
    SDL_GPUCommandBuffer* cmdCopy = nullptr;
    std::vector<Mesh> terrain;
    SDL_GPUGraphicsPipeline* graphicsPipeline = nullptr;
    SDL_GPUCopyPass* copyPass = nullptr;

    static constexpr float FOV_DEGREES = 45.0f;
    static constexpr float FOV_RADIANS = FOV_DEGREES * (M_PI / 180.0f);
    static constexpr float Z_NEAR = 0.1f;
    static constexpr float Z_FAR = 1000.0f;
    static constexpr int RENDER_DISTANCE = 1;
    static constexpr Uint32 VERTEX_SIZE = sizeof(Vertex) * 4 * 7000;
    static constexpr Uint32 INDEX_SIZE = sizeof(Uint32) * 6 * 7000;

    SDL_FPoint getUV(int tileIndex, int cornerX, int cornerY);
    Vector3 rotate(const Vector3& pos, const Vector3& angle);
    void drawFace(Player& player, const Vector3& blockPos, int color, int side, Mesh* mesh,
                  Vertex* vertexData, Uint32* indexData);
    void renderChunk(const ChunkPrefab& chunk, Player& player, int numChunk);
    void drawTerrain(Player& player);
    void createGraphicsPipeline();

   public:
    Renderer();
    ~Renderer();

    void stats(const Player& player);
    void drawPlayer(const Vector3& range, const std::vector<Player>& playerPos);
    void mainRenderLoop(std::vector<Slot>& inventory, int inventorySlot,
                        std::vector<Player>& players);
};
