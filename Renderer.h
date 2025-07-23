#pragma once

#include "common.hpp"

#include "Chunck.h"
#include "ChunkManager.h"

struct Plane
{
	Vector3 normal = { 0.f, 1.f, 0.f };

	double getSignedDistanceToPlane(const Vector3& point) const
	{
		return normal.Dot(point);
	}
};
struct Frustum
{
	Plane topFace, bottomFace, rightFace, leftFace, farFace, nearFace;

	Frustum createFrustumFromCamera(const Player& cam, float aspect, float fovY, float zNear, float zFar) const {
		Frustum frustum;

		float tanHalfFovY = fovY; // assuming fovY is already in tan(fovY / 2) form
		float halfVSide = zFar * tanHalfFovY;
		float halfHSide = halfVSide * aspect;

		Vector3 camForward = cam.Rotation.AngleToRadians().Forward();
		Vector3 camRight = cam.Rotation.AngleToRadians().Right();
		Vector3 camUp = cam.Rotation.AngleToRadians().Up();

		Vector3 nearCenter = cam.Position + camForward * zNear;
		Vector3 farCenter = cam.Position + camForward * zFar;

		// Near and far
		frustum.nearFace.normal = camForward;

		frustum.farFace.normal = camForward * -1;

		// Right
		{
			Vector3 A = cam.Position;
			Vector3 B = farCenter + (camRight * halfHSide);
			Vector3 C = B + (camUp * halfVSide);

			Vector3 normal = (B - A).Cross(C - A).Normalized();//camUp.Cross(rightEdge).Normalized();
			frustum.rightFace.normal = normal;
		}

		// Left
		{
			Vector3 A = cam.Position;
			Vector3 B = farCenter + (camRight * halfHSide * -1);
			Vector3 C = B + (camUp * halfVSide);

			Vector3 normal = (B - A).Cross(C - A).Normalized();//camUp.Cross(rightEdge).Normalized();
			frustum.leftFace.normal = normal;
		}

		// Top
		{
			Vector3 A = cam.Position;
			Vector3 B = farCenter;
			Vector3 C = B + (camUp * halfVSide);

			Vector3 normal = (B - A).Cross(C - A).Normalized();//camUp.Cross(rightEdge).Normalized();
			frustum.topFace.normal = normal;
		}

		// Bottom
		{
			Vector3 A = cam.Position;
			Vector3 B = farCenter;
			Vector3 C = B + (camUp * halfVSide * -1);

			Vector3 normal = (B - A).Cross(C - A).Normalized();//camUp.Cross(rightEdge).Normalized();
			frustum.bottomFace.normal = normal;
		}

		return frustum;
	}

};
struct Volume
{
	virtual bool isOnFrustum(const Frustum& camFrustum, 
		const Vector3* pointPosition,
		const Vector3& rotation) const = 0;
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

	bool isOnFrustum(const Frustum& camFrustum, const Vector3* pointPosition, const Vector3& rotation) const override
	{
		Vector3 forward = rotation.AngleToRadians().Forward();
		Vector3 right = rotation.AngleToRadians().Right();
		Vector3 up = rotation.AngleToRadians().Up();

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
		return plane.normal.Dot(point);
	}
};
struct Mesh
{
	std::vector<SDL_Vertex> Vertices;
	std::vector<int> Indices;
	std::vector<int> maxZ;
	int faces;
};

class GameClient;

class Renderer 
{
private:
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Event event;
	TTF_Font* font = nullptr;
	SDL_Texture* texture = nullptr;
	Vector3 ScreenSize = { 0,0,0 };
	ChunkManager chunkManager;
	GameClient& gameClient;
	int BlockPixelSize = 50;
	Mesh terrainMesh;
	bool fullScreen = false;
	Frustum frustum;

	SDL_FPoint getUV(int tileIndex, int cornerX, int cornerY);
	Vector3 rotate(const Vector3 pos, const Vector3 Angle);
	void DrawFace(Mesh& mesh, Player& player, Vector3 blocks, int color, int Side);
public:
	Renderer(GameClient& gameClient);
	~Renderer() {
		if (renderer) {
			SDL_DestroyRenderer(renderer);
			renderer = nullptr;
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
			SDL_DestroyTexture(texture);
			texture = nullptr;
		}
		event = {};
		terrainMesh = {};
	};

	void Stats(Player& player);
	void RenderChunk(ChunkPrefab& chunk, Player& player, Mesh& mesh);
	void DrawTerrain(Player& player);
	void DrawPlayer(SDL_Renderer* Renderer, Vector3 Range, std::vector<Player>& PlayerPos);
	void MainRenderLoop(std::vector<Slot>& inventory, int inventorySlot, std::vector<Player>& players);
};