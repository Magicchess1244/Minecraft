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
#include <cassert>

#pragma comment(lib, "ws2_32.lib")

constexpr unsigned int MAX_PLAYERS = 8;

typedef struct
{
	double x, y, z;
} Vector3;

typedef struct {
	unsigned int r, g, b;
} Color;

typedef struct {
	Vector3 Position;
	Vector3 Rotation;
	Color color;
} Player;

typedef struct {
	short Amount;
	short Type;
} Slot;

constexpr double PI = 3.1415926535f;

#define AngleToRadians(angle) (angle * (PI / 180.0f))
#define ADDVECTORS(V1, V2) {V1.x + V2.x, V1.y + V2.y, V1.z + V2.z} 
#define SUBSVECTORS(V1, V2) {V1.x - V2.x, V1.y - V2.y, V1.z - V2.z} 
#define MULTIPLYVECTOR(V1, V2) {V1.x * V2, V1.y * V2, V1.z * V2} 
#define ADDCOLOR(C1, C2) {SDL_clamp(C1.r + C2.r, 0, 1), SDL_clamp(C1.g + C2.g, 0, 1), SDL_clamp(C1.b + C2.b, 0, 1), SDL_clamp(C1.a + C2.a, 0, 1)}

static std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
	std::vector<std::string> tokens;
	size_t start = 0;
	size_t end;

	while ((end = s.find(delimiter, start)) != std::string::npos) {
		tokens.push_back(s.substr(start, end - start));
		start = end + delimiter.length();
	}
	tokens.push_back(s.substr(start));
	return tokens;
}

#include "PerlinNoise.h"
#include "BiomeBuilder.h"
#include "ChunkManager.h"
#include "Chunck.h"
#include "Renderer.h"
#include "GameClient.h"
#include "GameServer.h"

#endif