#include "../../include/common/PerlinNoise.hpp"


int seed = 0;

uint32_t SeededHash(const void* data, size_t length, uint32_t seedVal = 2166136261u) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    uint32_t hash = seedVal ^ seed;  // Mix in the global seed
    for (size_t i = 0; i < length; ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

float Fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }

Vector3 GradientFromAngles(const Vector3& Angle) {
    Vector3 Rad = Angle.AngleToRadians();
    Vector3 dir = {std::sin(Rad.z) * std::cos(Rad.x), std::sin(Rad.z) * std::sin(Rad.x),
                   std::cos(Rad.z)};
    return dir.Normalized();
}

// Convert hash to angle vector
Vector3 HashToAngles(uint32_t hash) {
    // Extract different parts of hash for x, y, z angles
    float x_angle = (hash & 0xFF) / 255.0f * 2.0f * 3.14159f;  // 0 to 2
    float z_angle = ((hash >> 8) & 0xFF) / 255.0f * 3.14159f;  // 0 to 
    return Vector3(x_angle, 0, z_angle);
}

float BasicPerlinNoise(float xPos, float yPos, float zPos) {
    int x0 = static_cast<int>(xPos) / 8;
    int y0 = static_cast<int>(yPos) / 8;
    int z0 = static_cast<int>(zPos) / 8;

    float localX = (xPos - x0 * 8) / 8.0f;
    float localY = (yPos - y0 * 8) / 8.0f;
    float localZ = (zPos - z0 * 8) / 8.0f;

    // Gradients at corners
    Vector3 gradients[8];
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            for (int k = 0; k < 2; ++k) {
                int index = i * 4 + j * 2 + k;
                Vector3 Position = {float(x0 + i), float(y0 + j), float(z0 + k)};

                // Hash the position
                uint32_t hash = SeededHash(&Position, sizeof(Position), seed);

                // Convert hash to angles and then to gradient
                Vector3 angles = HashToAngles(hash);
                gradients[index] = GradientFromAngles(angles);
            }
        }
    }

    // Relative positions to corners
    Vector3 rel[8] = {{localX, localY, localZ},         {localX - 1, localY, localZ},
                      {localX, localY - 1, localZ},     {localX - 1, localY - 1, localZ},
                      {localX, localY, localZ - 1},     {localX - 1, localY, localZ - 1},
                      {localX, localY - 1, localZ - 1}, {localX - 1, localY - 1, localZ - 1}};

    // Dot products
    float dots[8];
    for (int i = 0; i < 8; ++i) {
        dots[i] = gradients[i].Dot(rel[i]);
    }

    // Interpolation weights
    float u = Fade(localX);
    float v = Fade(localY);
    float w = Fade(localZ);

    // Interpolate
    float x00 = Lerp(dots[0], dots[1], u);
    float x01 = Lerp(dots[4], dots[5], u);
    float x10 = Lerp(dots[2], dots[3], u);
    float x11 = Lerp(dots[6], dots[7], u);

    float lerp0 = Lerp(x00, x10, v);
    float lerp1 = Lerp(x01, x11, v);

    return Lerp(lerp0, lerp1, w);
}

float PerlinNoise(Vector3 Pos, int Octaves, float ConstFrequency) {
    float Frequency = ConstFrequency;
    float Amplitude = ConstFrequency;
    float FinalNoise = 0.0f;

    for (int i = 0; i <= Octaves; ++i) {
        FinalNoise +=
            BasicPerlinNoise(Pos.x * Frequency, Pos.y * Frequency, Pos.z * Frequency) * Amplitude;
        Frequency *= 2.0f;
        Amplitude /= 2.0f;
    }

    // Optional contrast control
    FinalNoise *= 1.2f;
    return SDL_clamp(FinalNoise, -1.0f, 1.0f);
}

void SetSeed(int givenSeed) { seed = givenSeed; }