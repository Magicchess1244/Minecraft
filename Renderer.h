#pragma once

#include "common.hpp"

#include "Chunck.h"
#include "ChunkManager.h"

struct Plane
{
	Vector3 normal = { 0.f, 1.f, 0.f };

	double distance = 0;

	double getSignedDistanceToPlane(const Vector3& point) const
	{
		return normal.Dot(point);
	}
};
struct Frustum
{
	Plane topFace, bottomFace, rightFace, leftFace, farFace, nearFace;

	Frustum createFrustumFromCamera(float aspect, float fovY, float Znear, float Zfar) const {
		Frustum frustum;

		float tanHalfFovY = fovY;
		float halfVSide = Zfar * tanHalfFovY;
		float halfHSide = halfVSide * aspect;

		Vector3 camForward = { 0,0,1 };
		Vector3 camRight = { 1,0, 0 };
		Vector3 camUp = { 0,1,0 };

		Vector3 nearCenter = camForward * Znear;
		Vector3 farCenter = camForward * Zfar;

		// Near and far
		frustum.nearFace.normal = camForward;
		frustum.nearFace.distance = Znear;

		frustum.farFace.normal = camForward * -1;
		frustum.farFace.distance = Zfar;

		Vector3 A = { 0,0,0 };

		// Right
		{
			Vector3 B = farCenter + (camRight * halfHSide);
			Vector3 C = B + (camUp * halfVSide);

			Vector3 normal = (C - A).Cross(B - A).Normalized();//camUp.Cross(rightEdge).Normalized();
			frustum.rightFace.normal = normal;
		}

		// Left
		{
			Vector3 B = farCenter + (camRight * halfHSide * -1);
			Vector3 C = B + (camUp * halfVSide);

			Vector3 normal = (B - A).Cross(C - A).Normalized();//camUp.Cross(rightEdge).Normalized();
			frustum.leftFace.normal = normal;
		}

		// Top
		{
			Vector3 B = farCenter + (camUp * halfVSide);
			Vector3 C = B + (camRight * halfHSide);

			Vector3 normal = (B - A).Cross(C - A).Normalized();//camUp.Cross(rightEdge).Normalized();
			frustum.topFace.normal = normal;
		}

		// Bottom
		{
			Vector3 B = farCenter + (camUp * halfVSide * -1);
			Vector3 C = B + (camRight * halfHSide);

			Vector3 normal = (C - A).Cross(B - A).Normalized();//camUp.Cross(rightEdge).Normalized();
			frustum.bottomFace.normal = normal;
		}

		return frustum;
	}

};
struct Volume
{
	virtual bool isOnFrustum(const Frustum& camFrustum, Vector3* pointPosition) const = 0;
};
struct AABB : public Volume
{
	Vector3 center{ 0.0, 0.0, 0.0 };
	Vector3 extents{ 0.0, 0.0, 0.0 };

	AABB(const Vector3& min, const Vector3& max)
		: Volume{},
		center{ (max + min) * 0.5 },
		extents{ max.x - center.x, max.y - center.y, max.z - center.z }
	{
	}
	AABB(const Vector3& inCenter, double iI, double iJ, double iK)
		: Volume{}, center{ inCenter }, extents{ iI, iJ, iK }
	{
	}

	bool isOnFrustum(const Frustum& camFrustum, Vector3* pointPosition) const override
	{
		Vector3 forward = { 0,0,1 };
		Vector3 right = { 1,0, 0 };
		Vector3 up = { 0,1,0 };

		for (int i = 0; i < 4; i++) {
			bool IsOnNearPlane = isOnOrForwardPlane(camFrustum.nearFace, pointPosition[i]);
			bool IsOnFarPlane = isOnOrForwardPlane(camFrustum.farFace, pointPosition[i]);
			bool IsOnRightPlane = isOnOrForwardPlane(camFrustum.rightFace, pointPosition[i]);
			bool IsOnLeftPlane = isOnOrForwardPlane(camFrustum.leftFace, pointPosition[i]);
			bool IsOnTopPlane = isOnOrForwardPlane(camFrustum.topFace, pointPosition[i]);
			bool IsOnBottomPlane = isOnOrForwardPlane(camFrustum.bottomFace, pointPosition[i]);

			if (IsOnNearPlane && IsOnFarPlane && IsOnRightPlane && IsOnLeftPlane && IsOnTopPlane && IsOnBottomPlane) return false;
		}
		return true;
	}
	bool isOnOrForwardPlane(const Plane& plane, Vector3 point) const
	{
		return plane.normal.Dot(point) - plane.distance;
	}
};
struct Mesh
{
	SDL_GPUTransferBuffer* transferBuffer = nullptr;
	SDL_GPUBufferBinding* VertexBuffer;
	SDL_GPUBufferBinding* IndexBuffer;
	std::vector<double> MaxZ;
	int faces;
};

class GameClient;

class Renderer 
{
private:
	SDL_Window* window = nullptr;
	SDL_GPUDevice* GPU = nullptr;
	SDL_GPURenderPass* pass = nullptr;
	SDL_Event event;
	TTF_Font* font = nullptr;
	SDL_GPUTexture* texture = nullptr;
	Vector3 ScreenSize = { 0,0,0 };
	ChunkManager chunkManager;
	GameClient& gameClient;
	int BlockPixelSize = 50;
	bool fullScreen = false;
	Frustum frustum;
	unsigned int Width, Height;
	SDL_GPUCommandBuffer* cmdRender = nullptr;
	SDL_GPUCommandBuffer* cmdCopy = nullptr;
	std::vector<Mesh&> Terrain;
	SDL_GPUGraphicsPipeline* graphicsPipeline = nullptr;

	SDL_FPoint getUV(int tileIndex, int cornerX, int cornerY);
	Vector3 rotate(const Vector3 pos, const Vector3 Angle);
	void DrawFace(Player& player, Vector3 blocks, int color, int Side, Mesh& mesh);
public:
	Renderer(GameClient& gameClient);
	~Renderer() {
		for (Mesh& mesh : Terrain) {
			SDL_ReleaseGPUBuffer(this->GPU, mesh.IndexBuffer->buffer);
			SDL_ReleaseGPUBuffer(this->GPU, mesh.VertexBuffer->buffer);
			SDL_ReleaseGPUTransferBuffer(this->GPU, mesh.transferBuffer);
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
		if (texture) {
			//SDL_DestroyTexture(texture);
			texture = nullptr;
		}
		event = {};
		cmdCopy = nullptr;
		cmdRender = nullptr;
	};

	void Stats(Player& player);
	void RenderChunk(ChunkPrefab& chunk, Player& player, int NumChunk);
	void DrawTerrain(Player& player);
	void DrawPlayer(SDL_Renderer* Renderer, Vector3 Range, std::vector<Player>& PlayerPos);
	void MainRenderLoop(std::vector<Slot>& inventory, int inventorySlot, std::vector<Player>& players);
};