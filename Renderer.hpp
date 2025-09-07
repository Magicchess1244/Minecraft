#pragma once

#include "Chunk.hpp"
#include "ChunkManager.hpp"
#include "common.hpp"

struct Plane {
    Vector3 normal{0.f, 1.f, 0.f};  // must be normalized
    float distance = 0.f;           // the "d" in ax+by+cz+d=0

    // signed distance = n·p + d
    float getSignedDistanceToPlane(const Vector3& point) const {
        return normal.Dot(point) + distance;
    }
};
struct Frustum {
    Plane topFace, bottomFace, rightFace, leftFace, farFace, nearFace;

    // Creates a frustum in camera space. Camera is at origin looking +Z, Y up, X right.
    static Frustum createFrustumFromCamera(float aspect, float fovY_radians, float Znear,
                                           float Zfar) {
        Frustum frustum;

        // half sizes at far plane
        float halfVSide = Zfar * fovY_radians;
        float halfHSide = halfVSide * aspect;

        Vector3 camForward = {0.f, 0.f, 1.f};
        Vector3 camRight = {1.f, 0.f, 0.f};
        Vector3 camUp = {0.f, 1.f, 0.f};

        Vector3 nearCenter = camForward * Znear;
        Vector3 farCenter = camForward * Zfar;

        // Near plane (points toward +Z)
        frustum.nearFace.normal = camForward;                                  // (0,0,1)
        frustum.nearFace.distance = -frustum.nearFace.normal.Dot(nearCenter);  // -Znear

        // Far plane (points toward -Z)
        frustum.farFace.normal = camForward * -1;
        frustum.farFace.distance = -frustum.farFace.normal.Dot(farCenter);

        // Right plane: use right edge direction and ensure normal points inward
        {
            Vector3 rightEdge =
                (farCenter + camRight * halfHSide).Normalized();               // dir to far-right
            frustum.rightFace.normal = (rightEdge.Cross(camUp)).Normalized();  // inward
            frustum.rightFace.distance = 0.0f;  // plane goes through origin (camera)
        }

        // Left plane
        {
            Vector3 leftEdge = (farCenter - camRight * halfHSide).Normalized();
            frustum.leftFace.normal = (camUp.Cross(leftEdge)).Normalized();  // inward
            frustum.leftFace.distance = 0.0f;
        }

        // Top plane
        {
            Vector3 topEdge = (farCenter + camUp * halfVSide).Normalized();
            frustum.topFace.normal = (camRight.Cross(topEdge)).Normalized();  // inward
            frustum.topFace.distance = 0.0f;
        }

        // Bottom plane
        {
            Vector3 bottomEdge = (farCenter - camUp * halfVSide).Normalized();
            frustum.bottomFace.normal = (bottomEdge.Cross(camRight)).Normalized();  // inward
            frustum.bottomFace.distance = 0.0f;
        }

        return frustum;
    }
};
struct Mesh {
    SDL_GPUTransferBuffer* VertextransferBuffer = nullptr;
    SDL_GPUTransferBuffer* IndextransferBuffer = nullptr;
    SDL_GPUBufferBinding VertexBuffer;
    SDL_GPUBufferBinding IndexBuffer;
    std::vector<DrawnFace> Faces;
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
    Vector3 LastChunk = {999, 999, 999};
    Vector3 CurrentChunk = {0,0,0};

    SDL_FPoint getUV(int tileIndex, int cornerX, int cornerY);
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
