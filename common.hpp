#ifndef COMMON_HPP
#define COMMON_HPP

#include <cmath>
#include <vector>
#include <string>
#include <WinSock2.h>
#include <map>
#include <unordered_map>
#include <tuple>
#include <algorithm>
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <WinSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#include "PerlinNoise.h"
#include "Chunck.h"
#include "GameClient.h"
#include "Renderer.h"
#include "ChunkManager.h"

typedef struct
{
	double x, y, z;
} Vector3;


constexpr double PI = 3.1415926535f;
#define AngleToRadians(angle) (angle * (PI / 180.0f))
#define ADDVECTORS(V1, V2) {V1.x + V2.x, V1.y + V2.y, V1.z + V2.z} 
#define SUBSVECTORS(V1, V2) {V1.x - V2.x, V1.y - V2.y, V1.z - V2.z} 
#define MULTIPLYVECTOR(V1, V2) {V1.x * V2, V1.y * V2, V1.z * V2} 
#define ADDCOLOR(C1, C2) {SDL_clamp(C1.r + C2.r, 0, 1), SDL_clamp(C1.g + C2.g, 0, 1), SDL_clamp(C1.b + C2.b, 0, 1), SDL_clamp(C1.a + C2.a, 0, 1)}

typedef struct {
	unsigned int r, g, b;
} Color;

typedef struct {
	Vector3 Position;
	Vector3 Rotation;
	Color color;
} Player;

#endif