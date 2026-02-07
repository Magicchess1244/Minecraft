#include "../../include/common/PerlinNoise.hpp"
#include <cstdint>
#include <cmath>

int seed;

// Hash function for generating pseudorandom values
uint32_t hash(int32_t x, int32_t y, int32_t z, int32_t seed) {
    uint32_t h = seed;
    h ^= x * 1619;
    h ^= y * 31337;
    h ^= z * 6971;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

// Generate a random gradient vector from hash
Vector3 GetGradient(int32_t x, int32_t y, int32_t z, int32_t seed) {
    uint32_t h = hash(x, y, z, seed);
    
    // Use hash to select from a set of 12 gradient directions
    // These are the edge directions of a cube (normalized)
    static const Vector3 gradients[12] = {
        {1, 1, 0},  {-1, 1, 0},  {1, -1, 0},  {-1, -1, 0},
        {1, 0, 1},  {-1, 0, 1},  {1, 0, -1},  {-1, 0, -1},
        {0, 1, 1},  {0, -1, 1},  {0, 1, -1},  {0, -1, -1}
    };
    
    return gradients[h % 12];
}

float Fade(float t) { 
    return t * t * t * (t * (t * 6 - 15) + 10); 
}

float BasicPerlinNoise(float xPos, float yPos, float zPos) {
    int x0 = static_cast<int>(std::floor(xPos));
    int y0 = static_cast<int>(std::floor(yPos));
    int z0 = static_cast<int>(std::floor(zPos));
    
    float localX = xPos - x0;
    float localY = yPos - y0;
    float localZ = zPos - z0;
    
    // Get gradients at 8 corners
    Vector3 g000 = GetGradient(x0,     y0,     z0,     seed);
    Vector3 g100 = GetGradient(x0 + 1, y0,     z0,     seed);
    Vector3 g010 = GetGradient(x0,     y0 + 1, z0,     seed);
    Vector3 g110 = GetGradient(x0 + 1, y0 + 1, z0,     seed);
    Vector3 g001 = GetGradient(x0,     y0,     z0 + 1, seed);
    Vector3 g101 = GetGradient(x0 + 1, y0,     z0 + 1, seed);
    Vector3 g011 = GetGradient(x0,     y0 + 1, z0 + 1, seed);
    Vector3 g111 = GetGradient(x0 + 1, y0 + 1, z0 + 1, seed);
    
    // Compute dot products with relative position vectors
    float d000 = g000.Dot({localX,     localY,     localZ});
    float d100 = g100.Dot({localX - 1, localY,     localZ});
    float d010 = g010.Dot({localX,     localY - 1, localZ});
    float d110 = g110.Dot({localX - 1, localY - 1, localZ});
    float d001 = g001.Dot({localX,     localY,     localZ - 1});
    float d101 = g101.Dot({localX - 1, localY,     localZ - 1});
    float d011 = g011.Dot({localX,     localY - 1, localZ - 1});
    float d111 = g111.Dot({localX - 1, localY - 1, localZ - 1});
    
    // Fade curves
    float u = Fade(localX);
    float v = Fade(localY);
    float w = Fade(localZ);
    
    // Trilinear interpolation
    float x00 = Lerp(d000, d100, u);
    float x01 = Lerp(d001, d101, u);
    float x10 = Lerp(d010, d110, u);
    float x11 = Lerp(d011, d111, u);
    
    float Lerp0 = Lerp(x00, x10, v);
    float Lerp1 = Lerp(x01, x11, v);
    
    return Lerp(Lerp0, Lerp1, w);
}

float PerlinNoise(Vector3 Pos, int Octaves, float ConstFrequency) {
    float Frequency = ConstFrequency;
    float Amplitude = 1.0f;  // Start with amplitude 1.0
    float FinalNoise = 0.0f;
    float MaxValue = 0.0f;  // For normalization
    
    for (int i = 0; i < Octaves; ++i) {
        FinalNoise += BasicPerlinNoise(
            Pos.x * Frequency, 
            Pos.y * Frequency,
            Pos.z * Frequency
        ) * Amplitude;
        
        MaxValue += Amplitude;
        Frequency *= 2.0f;
        Amplitude *= 0.5f;
    }
    
    // Normalize to [-1, 1] range
    FinalNoise /= MaxValue;
    
    return SDL_clamp(FinalNoise, -1.0f, 1.0f);
}

void SetSeed(int givenSeed) { 
    seed = givenSeed; 
}