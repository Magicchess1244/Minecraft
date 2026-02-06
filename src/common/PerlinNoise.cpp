#include "../../include/common/PerlinNoise.hpp"
#include <cstdint>
#include <random>

int seed;

int64_t squish_position(float x, float y, float z, float grid_size = 1.0f) {
    int32_t ix = static_cast<int32_t>(x / grid_size);
    int32_t iy = static_cast<int32_t>(y / grid_size);
    int32_t iz = static_cast<int32_t>(z / grid_size);
    
    // Pack into 64-bit int (21 bits per component)
    int64_t packed = (static_cast<int64_t>(ix & 0x1FFFFF)) |
                     (static_cast<int64_t>(iy & 0x1FFFFF) << 21) |
                     (static_cast<int64_t>(iz & 0x1FFFFF) << 42);
    return packed;
}

float Fade(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }

static Vector3 GradientFromAngles(const Vector3 &Angle) {
    Vector3 Rad = Angle.AngleToRadians();
    // Using spherical coordinates: yaw (x), pitch (z)
    Vector3 dir = {std::sin(Rad.z) * std::cos(Rad.x),
                   std::sin(Rad.z) * std::sin(Rad.x), 
                   std::cos(Rad.z)};
    return dir.Normalized();
}

float BasicPerlinNoise(float xPos, float yPos, float zPos) {
    int x0 = static_cast<int>(std::floor(xPos));
    int y0 = static_cast<int>(std::floor(yPos));
    int z0 = static_cast<int>(std::floor(zPos));
    
    float localX = xPos - x0;
    float localY = yPos - y0;
    float localZ = zPos - z0;
    
    // Gradients at corners
    Vector3 gradients[8];
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            for (int k = 0; k < 2; ++k) {
                int index = i * 4 + j * 2 + k;
                
                // Pack the corner position
                int64_t packed = squish_position(
                    static_cast<float>(x0 + i), 
                    static_cast<float>(y0 + j), 
                    static_cast<float>(z0 + k)
                );
                
                // Salt it: combine seed with packed position
                int64_t salted = (static_cast<int64_t>(seed) << 42) | packed;
                
                // Use salted value as random seed
                std::mt19937 rng(salted);
                std::uniform_real_distribution<float> dist1(0.0f, 360.0f);
                std::uniform_real_distribution<float> dist2(0.0f, 180.0f);
                
                float angle1 = dist1(rng);
                float angle2 = dist2(rng);
                
                gradients[index] = GradientFromAngles({angle1, 0, angle2});
            }
        }
    }
    
    // Relative positions to corners
    Vector3 rel[8] = {
        {localX, localY, localZ},         
        {localX - 1, localY, localZ},
        {localX, localY - 1, localZ},     
        {localX - 1, localY - 1, localZ},
        {localX, localY, localZ - 1},     
        {localX - 1, localY, localZ - 1},
        {localX, localY - 1, localZ - 1}, 
        {localX - 1, localY - 1, localZ - 1}
    };
    
    // Dot products
    float dots[8] = {0};
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
    float FinalNoise = 0.0;
    
    for (int i = 0; i <= Octaves; ++i) {
        FinalNoise += BasicPerlinNoise(Pos.x * Frequency, Pos.y * Frequency,
                                       Pos.z * Frequency) * Amplitude;
        Frequency *= 2.0;
        Amplitude /= 2.0;
    }
    
    // Optional contrast control
    FinalNoise *= 2;
    return SDL_clamp(FinalNoise, -1.0, 1.0);
}

void SetSeed(int givenSeed) { seed = givenSeed; }