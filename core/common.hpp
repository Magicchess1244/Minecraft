#ifndef COMMON_HPP
#define COMMON_HPP

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "../net/common.hpp"

constexpr unsigned int MAX_PLAYERS = 8;
constexpr float PI = 3.1415926535f;

struct Vector3 {
    float x, y, z;

    Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

    Vector3 operator+(const Vector3& other) const {
        return {x + other.x, y + other.y, z + other.z};
    }

    Vector3& operator+=(const Vector3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3 operator-(const Vector3& other) const {
        return {x - other.x, y - other.y, z - other.z};
    }

    Vector3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    Vector3 operator*(Vector3 vector) const { return {x * vector.x, y * vector.y, z * vector.z}; }

    Vector3& operator*=(const Vector3& other) {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        return *this;
    }

    Vector3 operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar}; }

    Vector3 operator%(Vector3 vector) const {
        return {std::fmod(x, vector.x), std::fmod(y, vector.y), std::fmod(z, vector.z)};
    }

    bool operator==(const Vector3& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator>(const Vector3& other) const { return x > other.x && y > other.y && z > other.z; }

    float Dot(const Vector3& other) const { return x * other.x + y * other.y + z * other.z; }

    Vector3 Cross(const Vector3& other) const {
        return {y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x};
    }

    float LengthSquared() const { return x * x + y * y + z * z; }

    float Length() const { return std::sqrt(LengthSquared()); }

    Vector3 Normalized() const {
        float len = Length();
        return len != 0 ? *this / len : Vector3();
    }

    Vector3 AngleToRadians() const {
        return {x * float(PI / 180.0), y * float(PI / 180.0), z * float(PI / 180.0)};
    }

    Vector3 Truncate() const { return {floorf(x), floorf(y), floorf(z)}; }

    Vector3 Forward() const {
        Vector3 rotationRadians = AngleToRadians();

        float pitch = rotationRadians.x;
        float yaw = rotationRadians.y;
        // float roll = rotationRadians.z;

        Vector3 forward;
        forward.x = cos(pitch) * sin(yaw);
        forward.y = -sin(pitch);
        forward.z = -cos(pitch) * cos(yaw);
        return forward.Normalized();
    }

    Vector3 Right() const {
        Vector3 rotationRadians = AngleToRadians();

        float yaw = rotationRadians.y;

        Vector3 right;
        right.x = cos(yaw);
        right.y = 0;
        right.z = sin(yaw);
        return right.Normalized();
    }

    Vector3 Up() const {
        Vector3 up = Right().Cross(Forward()).Normalized();
        return up;
    }

    Vector3 Max(const Vector3& a) const {
        return {std::max(x, a.x), std::max(y, a.y), std::max(z, a.z)};
    }

    Vector3 Min(const Vector3& a) const {
        return {std::min(x, a.x), std::min(y, a.y), std::min(z, a.z)};
    }
};

struct Color {
    unsigned int r, g, b;

    Color operator+(const Color& other) {
        return {SDL_clamp(r + other.r, 0u, 255u), SDL_clamp(g + other.g, 0u, 255u),
                SDL_clamp(b + other.b, 0u, 255u)};
    }

    Color operator-(const Color& other) {
        return {SDL_clamp(r - other.r, 0u, 255u), SDL_clamp(g - other.g, 0u, 255u),
                SDL_clamp(b - other.b, 0u, 255u)};
    }

    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b;
    }

    SDL_FColor ToSDL() const {
        return SDL_FColor{static_cast<float>(r) / 255.0f, static_cast<float>(g) / 255.0f,
                          static_cast<float>(b) / 255.0f, 1.0f};
    }
    Vector3 ToFloat() const {
        return Vector3{
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f,
        };
    }
};

struct Player {
    Vector3 Position;
    Vector3 Rotation;
    Color color;
};

struct Slot {
    short Amount;
    short Type;
};

namespace std {
template <>
struct hash<Vector3> {
    std::size_t operator()(const Vector3& v) const {
        std::size_t h1 = std::hash<int>{}((int)v.x);
        std::size_t h2 = std::hash<int>{}((int)v.y);
        std::size_t h3 = std::hash<int>{}((int)v.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
}  // namespace std

inline float Lerp(float a, float b, float t) { return a + t * (b - a); }

#endif
