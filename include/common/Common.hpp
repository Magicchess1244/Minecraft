#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>
constexpr unsigned int MAX_PLAYERS = 8;
constexpr float PI = 3.1415926535f;
struct Vector2 {
  float x, y;

  float Dot(Vector2 other) { return this->x * other.x + this->y * other.y; }
};
struct Vector3 {
  float x, y, z;

  Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}

  Vector3 operator+(const Vector3 &other) const {
    return {x + other.x, y + other.y, z + other.z};
  }

  Vector3 &operator+=(const Vector3 &other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }

  Vector3 operator-(const Vector3 &other) const {
    return {x - other.x, y - other.y, z - other.z};
  }

  Vector3 operator*(float scalar) const {
    return {x * scalar, y * scalar, z * scalar};
  }
  Vector3 operator*(Vector3 vector) const {
    return {x * vector.x, y * vector.y, z * vector.z};
  }

  Vector3 &operator*=(const Vector3 &other) {
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
  }

  Vector3 operator/(float scalar) const {
    return {x / scalar, y / scalar, z / scalar};
  }

  Vector3 operator%(Vector3 vector) const {
    return {std::fmod(x, vector.x), std::fmod(y, vector.y),
            std::fmod(z, vector.z)};
  }

  bool operator==(const Vector3 &other) const {
    return x == other.x && y == other.y && z == other.z;
  }

  bool operator!=(const Vector3 &other) const { return !(*this == other); }

  bool operator<(const Vector3 &other) const {
    if (x != other.x)
      return x < other.x;
    if (y != other.y)
      return y < other.y;
    return z < other.z;
  }
  bool operator>(const Vector3 &other) const {
    return x > other.x && y > other.y && z > other.z;
  }
  float operator[](int i) const {
    switch (i) {
    case 0:
      return x;
    case 1:
      return y;
    case 2:
      return z;
    default:
      throw std::out_of_range("Vector3 index out of range");
    }
  }

  float Dot(const Vector3 &other) const {
    return x * other.x + y * other.y + z * other.z;
  }

  Vector3 Cross(const Vector3 &other) const {
    return {y * other.z - z * other.y, z * other.x - x * other.z,
            x * other.y - y * other.x};
  }

  float LengthSquared() const { return x * x + y * y + z * z; }

  float Length() const { return std::sqrt(LengthSquared()); }

  Vector3 Normalized() const {
    float len = Length();
    return len != 0 ? *this / len : Vector3();
  }

  Vector3 AngleToRadians() const {
    return {x * float(PI / 180.0), y * float(PI / 180.0),
            z * float(PI / 180.0)};
  }

  Vector3 Round() const {
    return {std::round(x), std::round(y), std::round(z)};
  }
  Vector3 Truncate() const { return {floorf(x), floorf(y), floorf(z)}; }

  Vector3 Forward() const {
    Vector3 rotationRadians = AngleToRadians();
    float pitch = rotationRadians.x;
    float yaw = rotationRadians.y;

    Vector3 forward;
    forward.x = cos(pitch) * sin(yaw);
    forward.y = -sin(pitch);
    forward.z = cos(pitch) * cos(yaw);

    return forward.Normalized();
  }

  Vector3 Right() const {
    Vector3 rotationRadians = AngleToRadians();
    float pitch = rotationRadians.x;
    float yaw = rotationRadians.y;
    float roll = rotationRadians.z;

    Vector3 right;
    right.x = cos(yaw) * cos(roll) + sin(yaw) * sin(pitch) * sin(roll);
    right.y = cos(pitch) * sin(roll);
    right.z = -sin(yaw) * cos(roll) + cos(yaw) * sin(pitch) * sin(roll);

    return right.Normalized();
  }

  Vector3 Up() const { return Right().Cross(Forward()); }

  Vector3 Max(const Vector3 &a) const {
    return {std::max(x, a.x), std::max(y, a.y), std::max(z, a.z)};
  }

  Vector3 Min(const Vector3 &a) const {
    return {std::min(x, a.x), std::min(y, a.y), std::min(z, a.z)};
  }
  Uint16 ToIndex(int dim1, int dim2) {
    return (Uint16)((int)x + (int)y * dim1 + (int)z * dim1 * dim2);
  }
};
struct Matrix {
  size_t rows, cols;
  std::vector<float> data; // row-major storage

  Matrix(size_t r, size_t c, float val = 0.0f)
      : rows(r), cols(c), data(r * c, val) {}

  // Copy constructor and assignment operator (default is fine)
  Matrix(const Matrix &other) = default;
  Matrix &operator=(const Matrix &other) = default;

  // Move constructor and assignment operator for better performance
  Matrix(Matrix &&other) noexcept = default;
  Matrix &operator=(Matrix &&other) noexcept = default;

  // Bounds-checked accessors
  float &operator()(size_t r, size_t c) {
    if (r >= rows || c >= cols) {
      throw std::out_of_range("Matrix index out of bounds");
    }
    return data[r * cols + c];
  }

  float operator()(size_t r, size_t c) const {
    if (r >= rows || c >= cols) {
      throw std::out_of_range("Matrix index out of bounds");
    }
    return data[r * cols + c];
  }

  // Matrix multiplication (fixed order)
  Matrix operator*(const Matrix &other) const {
    if (cols != other.rows) {
      throw std::invalid_argument(
          "Matrix dimensions do not match for multiplication");
    }
    Matrix result(rows, other.cols, 0.0f);
    for (size_t i = 0; i < rows; ++i) {
      for (size_t j = 0; j < other.cols; ++j) {
        float sum = 0.0f;
        for (size_t k = 0; k < cols; ++k) {
          sum += (*this)(i, k) * other(k, j);
        }
        result(i, j) = sum;
      }
    }
    return result;
  }

  static Matrix Identity(size_t n) {
    Matrix I(n, n, 0.0f);
    for (size_t i = 0; i < n; ++i)
      I(i, i) = 1.0f;
    return I;
  }

  // Fixed print method with better formatting
  void print() const {
    std::cout << "Matrix " << rows << "x" << cols << ":" << std::endl;
    for (size_t i = 0; i < rows; ++i) {
      for (size_t j = 0; j < cols; ++j) {
        std::cout << std::setw(12) << std::setprecision(6) << std::fixed
                  << (*this)(i, j);
        if (j < cols - 1)
          std::cout << " ";
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }

  // Convert to column-major for GPU (if needed)
  std::vector<float> getColumnMajorData() const {
    std::vector<float> colMajor(data.size());
    for (size_t i = 0; i < rows; ++i) {
      for (size_t j = 0; j < cols; ++j) {
        colMajor[j * rows + i] = (*this)(i, j);
      }
    }
    return colMajor;
  }
};
enum class PlayerColor {
  RED,
  GREEN,
  BLUE,
  YELLOW,
  MAGENTA,
  CYAN,
  ORANGE,
  PURPLE,
  COUNT
};

struct Color {
  Uint8 r, g, b;

  static Color GetColor(PlayerColor pc) {
    switch (pc) {
    case PlayerColor::RED:
      return {255, 0, 0};
    case PlayerColor::GREEN:
      return {0, 255, 0};
    case PlayerColor::BLUE:
      return {0, 0, 255};
    case PlayerColor::YELLOW:
      return {255, 255, 0};
    case PlayerColor::MAGENTA:
      return {255, 0, 255};
    case PlayerColor::CYAN:
      return {0, 255, 255};
    case PlayerColor::ORANGE:
      return {255, 165, 0};
    case PlayerColor::PURPLE:
      return {128, 0, 128};
    default:
      return {255, 255, 255};
    }
  }

  Color operator+(const Color &other) const {
    return {static_cast<Uint8>(SDL_clamp(
                static_cast<int>(r) + static_cast<int>(other.r), 0, 255)),
            static_cast<Uint8>(SDL_clamp(
                static_cast<int>(g) + static_cast<int>(other.g), 0, 255)),
            static_cast<Uint8>(SDL_clamp(
                static_cast<int>(b) + static_cast<int>(other.b), 0, 255))};
  }

  Color operator-(const Color &other) const {
    return {static_cast<Uint8>(SDL_clamp(
                static_cast<int>(r) - static_cast<int>(other.r), 0, 255)),
            static_cast<Uint8>(SDL_clamp(
                static_cast<int>(g) - static_cast<int>(other.g), 0, 255)),
            static_cast<Uint8>(SDL_clamp(
                static_cast<int>(b) - static_cast<int>(other.b), 0, 255))};
  }

  bool operator==(const Color &other) const {
    return r == other.r && g == other.g && b == other.b;
  }

  SDL_FColor ToSDL() const {
    return SDL_FColor{static_cast<float>(r) / 255.0f,
                      static_cast<float>(g) / 255.0f,
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
struct BlockMod {
  Vector3 pos;
  Uint8 type;
};

struct Slot {
  short Amount;
  short Type;
};

struct Player {
  int id;
  std::string name;
  Vector3 Position;
  Vector3 Rotation;
  Color color;
  bool Inwater = false;
  std::vector<Slot> inventory;

  Player()
      : id(-1), Position(0, 0, 0), Rotation(0, 0, 0), color{255, 255, 255} {
    inventory.assign(8 * 4, {0, 0});
  }
};

namespace std {
template <> struct hash<Vector3> {
  std::size_t operator()(const Vector3 &v) const {
    // FNV-1a hash for better performance and distribution
    constexpr std::size_t FNV_prime = 1099511628211ULL;
    constexpr std::size_t FNV_offset = 14695981039346656037ULL;

    std::size_t hash = FNV_offset;
    hash ^= static_cast<std::size_t>(v.x);
    hash *= FNV_prime;
    hash ^= static_cast<std::size_t>(v.y);
    hash *= FNV_prime;
    hash ^= static_cast<std::size_t>(v.z);
    hash *= FNV_prime;

    return hash;
  }
};
} // namespace std

// Debug logging macros - only enabled in debug builds
#ifdef DEBUG_CHUNKS
#define CHUNK_LOG(msg) std::cout << msg << std::endl
#else
#define CHUNK_LOG(msg) ((void)0)
#endif

inline float Lerp(float a, float b, float t) { return a + t * (b - a); }