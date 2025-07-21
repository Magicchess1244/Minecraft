#ifndef COMMON_HPP
#define COMMON_HPP

#include <cmath>
#include <vector>
#include <string>
#include <unordered_map>
#include <tuple>
#include <algorithm>
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <cmath>
#include <cassert>

#pragma comment(lib, "ws2_32.lib")

constexpr unsigned int MAX_PLAYERS = 8;
constexpr double PI = 3.1415926535f;

struct Vector3 {
	double x, y, z;

	Vector3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}

	Vector3 operator+(const Vector3& other) const {
		return { x + other.x, y + other.y, z + other.z };
	}

	Vector3 operator-(const Vector3& other) const {
		return { x - other.x, y - other.y, z - other.z };
	}

	Vector3 operator*(double scalar) const {
		return { x * scalar, y * scalar, z * scalar };
	}

	Vector3 operator/(double scalar) const {
		return { x / scalar, y / scalar, z / scalar };
	}

	bool operator==(const Vector3& other) const {
		return x == other.x && y == other.y && z == other.z;
	}

	double Dot(const Vector3& other) const {
		return x * other.x + y * other.y + z * other.z;
	}

	Vector3 Cross(const Vector3& other) const {
		return {
			y * other.z - z * other.y,
			z * other.x - x * other.z,
			x * other.y - y * other.x
		};
	}

	double Length() const {
		return std::sqrt(x * x + y * y + z * z);
	}

	Vector3 Normalized() const {
		double len = Length();
		return len != 0 ? *this / len : Vector3();
	}

	Vector3 AngleToRadians() const {
		return { x * (PI / 180.0), y * (PI / 180.0), z * (PI / 180.0) };
	}
};

struct Color {
	unsigned int r, g, b;


	Color operator+(const Color& other) {
		return { SDL_clamp(r + other.r, 0u, 255u) , SDL_clamp(g + other.g, 0u, 255u), SDL_clamp(b + other.b, 0u, 255u) };
	}

	Color operator-(const Color& other) {
		return { SDL_clamp(r - other.r, 0u, 255u) , SDL_clamp(g - other.g, 0u, 255u), SDL_clamp(b - other.b, 0u, 255u) };
	}

	bool operator==(const Color& other) const {
		return r == other.r && g == other.g && b == other.b;
	}

	SDL_FColor ToSDL() const {
		return SDL_FColor{
			static_cast<float>(r) / 255.0f,
			static_cast<float>(g) / 255.0f,
			static_cast<float>(b) / 255.0f,
			1.0f
		};
	}
};

typedef struct {
	Vector3 Position;
	Vector3 Rotation;
	Color color;
} Player;

typedef struct {
	short Amount;
	short Type;
} Slot;


namespace std {
	template <>
	struct hash<Vector3> {
		std::size_t operator()(const Vector3& v) const {
			std::size_t h1 = std::hash<int>{}(v.x);
			std::size_t h2 = std::hash<int>{}(v.y);
			std::size_t h3 = std::hash<int>{}(v.z);
			return h1 ^ (h2 << 1) ^ (h3 << 2);
		}
	};
}
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
inline double Lerp(double a, double b, double t)
{
	return a + t * (b - a);
}

#endif