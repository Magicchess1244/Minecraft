#pragma once

#include "Chunk.hpp"
#include "ChunkManager.hpp"
#include "common.hpp"

struct Plane {
    Vector3 normal = {0.f, 1.f, 0.f};

    float distance = 0;

    float getSignedDistanceToPlane(const Vector3& point) const { return normal.Dot(point); }
};

struct Frustum {
    Plane topFace, bottomFace, rightFace, leftFace, farFace, nearFace;

    Frustum createFrustumFromCamera(float aspect, float fovY, float Znear, float Zfar) const {
        Frustum frustum;

        // Half-angles
        float halfVSide = Zfar * fovY;
        float halfHSide = halfVSide * aspect;

        // Camera basis (assuming looking down +Z, Y up, X right)
        Vector3 camForward = {0, 0, 1};
        Vector3 camRight = {1, 0, 0};
        Vector3 camUp = {0, 1, 0};

        Vector3 nearCenter = camForward * Znear;
        Vector3 farCenter = camForward * Zfar;

        // --- Near plane ---
        frustum.nearFace.normal = camForward;
        frustum.nearFace.distance = -camForward.Dot(nearCenter);

        // --- Far plane ---
        frustum.farFace.normal = camForward * -1;
        frustum.farFace.distance = frustum.farFace.normal.Dot(farCenter);

        // --- Right plane ---
        {
            Vector3 rightEdge = (farCenter + camRight * halfHSide).Normalized();
            Vector3 normal = camUp.Cross(rightEdge).Normalized();
            frustum.rightFace.normal = normal;
            frustum.rightFace.distance = 0.0f;  // passes through origin
        }

        // --- Left plane ---
        {
            Vector3 leftEdge = (farCenter - camRight * halfHSide).Normalized();
            Vector3 normal = leftEdge.Cross(camUp).Normalized();
            frustum.leftFace.normal = normal;
            frustum.leftFace.distance = 0.0f;
        }

        // --- Top plane ---
        {
            Vector3 topEdge = (farCenter + camUp * halfVSide).Normalized();
            Vector3 normal = topEdge.Cross(camRight).Normalized();
            frustum.topFace.normal = normal;
            frustum.topFace.distance = 0.0f;
        }

        // --- Bottom plane ---
        {
            Vector3 bottomEdge = (farCenter - camUp * halfVSide).Normalized();
            Vector3 normal = camRight.Cross(bottomEdge).Normalized();
            frustum.bottomFace.normal = normal;
            frustum.bottomFace.distance = 0.0f;
        }

        return frustum;
    }
};
struct Volume {
    virtual bool isOnFrustum(const Frustum& camFrustum, Vector3* pointPosition) const = 0;
};
struct AABB : public Volume {
    Vector3 center{0.0, 0.0, 0.0};
    Vector3 extents{0.0, 0.0, 0.0};

    AABB(const Vector3& min, const Vector3& max)
        : Volume{},
          center{(max + min) * 0.5},
          extents{max.x - center.x, max.y - center.y, max.z - center.z} {}
    AABB(const Vector3& inCenter, float iI, float iJ, float iK)
        : Volume{}, center{inCenter}, extents{iI, iJ, iK} {}

    bool isOnFrustum(const Frustum& camFrustum, Vector3* pointPosition) const override {
        /*        Vector3 forward = {0, 0, 1};
                Vector3 right = {1, 0, 0};
                Vector3 up = {0, 1, 0};
        */

        for (int i = 0; i < 4; i++) {
            bool IsOnNearPlane = isOnOrForwardPlane(camFrustum.nearFace, pointPosition[i]);
            bool IsOnFarPlane = isOnOrForwardPlane(camFrustum.farFace, pointPosition[i]);
            bool IsOnRightPlane = isOnOrForwardPlane(camFrustum.rightFace, pointPosition[i]);
            bool IsOnLeftPlane = isOnOrForwardPlane(camFrustum.leftFace, pointPosition[i]);
            bool IsOnTopPlane = isOnOrForwardPlane(camFrustum.topFace, pointPosition[i]);
            bool IsOnBottomPlane = isOnOrForwardPlane(camFrustum.bottomFace, pointPosition[i]);

            if (IsOnNearPlane && IsOnFarPlane && IsOnRightPlane && IsOnLeftPlane && IsOnTopPlane &&
                IsOnBottomPlane)
                return false;
        }
        return true;
    }
    bool isOnOrForwardPlane(const Plane& plane, Vector3 point) const {
        return plane.normal.Dot(point) - plane.distance;
    }
};
struct Mesh {
    SDL_GPUTransferBuffer* VertextransferBuffer = nullptr;
    SDL_GPUTransferBuffer* IndextransferBuffer = nullptr;
    SDL_GPUBufferBinding VertexBuffer;
    SDL_GPUBufferBinding IndexBuffer;
    int faces;
};
struct Vertex {
    Vector3 Position;
    //float pad1;  // padding to 16 bytes
    Vector3 Color;
    //float pad2;  // padding to 16 bytes
};


class GameClient;

class Renderer {
   private:
    SDL_Window* window = nullptr;
    SDL_GPUDevice* GPU = nullptr;
    SDL_GPURenderPass* pass = nullptr;
    SDL_Event event;
    TTF_Font* font = nullptr;
    SDL_GPUTexture* texture = nullptr;
    Vector3 ScreenSize = {0, 0, 0};
    GameClient& gameClient;
    ChunkManager chunkManager;
    int BlockPixelSize = 50;
    bool fullScreen = false;
    Frustum frustum;
    unsigned int Width, Height;
    SDL_GPUCommandBuffer* cmdRender = nullptr;
    SDL_GPUCommandBuffer* cmdCopy = nullptr;
    std::vector<Mesh> Terrain;
    SDL_GPUGraphicsPipeline* graphicsPipeline = nullptr;
    SDL_GPUCopyPass* copyPass = nullptr;
    SDL_GPUTexture* depthTexture = nullptr;

    SDL_FPoint getUV(int tileIndex, int cornerX, int cornerY);
    Vector3 rotate(const Vector3 pos, const Vector3 Angle);
    void DrawFace(Player& player, Vector3 blocks, int color, int Side, Mesh* mesh,
                  Vertex* Vertexdata, Uint32* Indexdata);
    SDL_GPUTexture* CreateDepthTexture(Uint32 drawablew, Uint32 drawableh);

   public:
    explicit Renderer(GameClient& gameClient);
    ~Renderer() {
        for (auto& mesh : this->Terrain) {
            SDL_ReleaseGPUBuffer(this->GPU, mesh.IndexBuffer.buffer);
            SDL_ReleaseGPUBuffer(this->GPU, mesh.VertexBuffer.buffer);
            SDL_ReleaseGPUTransferBuffer(this->GPU, mesh.IndextransferBuffer);
            SDL_ReleaseGPUTransferBuffer(this->GPU, mesh.VertextransferBuffer);
        }

        SDL_ReleaseGPUGraphicsPipeline(this->GPU, graphicsPipeline);
        if (this->GPU) {
            SDL_DestroyGPUDevice(this->GPU);
            this->GPU = nullptr;
        }
        if (this->pass) {
            this->pass = nullptr;
        }
        if (this->window) {
            SDL_DestroyWindow(this->window);
            this->window = nullptr;
        }
        if (this->font) {
            TTF_CloseFont(this->font);
            this->font = nullptr;
        }
        if (this->texture) {
            // SDL_DestroyTexture(texture);
            this->texture = nullptr;
        }

        this->event = {};
        this->cmdCopy = nullptr;
        this->cmdRender = nullptr;
    }

    void Stats(const Player& player);
    void RenderChunk(const ChunkPrefab& chunk, Player& player, int NumChunk);
    void DrawTerrain(Player& player);
    void DrawPlayer(SDL_Renderer* Renderer, Vector3 Range, const std::vector<Player>& PlayerPos);
    void MainRenderLoop(std::vector<Slot>& inventory, int inventorySlot,
                        std::vector<Player>& players);
};
