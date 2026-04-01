#include "../../include/common/PerlinNoise.hpp"
#include <cstdint>
#include <cmath>
#include <string>

int seed = 5;

uint32_t hash(int32_t x, int32_t y, int32_t z) {
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

uint32_t hash2D(int32_t x, int32_t y) {
    uint32_t h = seed;
    h ^= x * 1619;
    h ^= y * 31337;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

Vector3 GetGradient(int32_t x, int32_t y, int32_t z, int32_t seed) {
    uint32_t h = hash(x, y, z);
    
    Vector3 grad;
    grad.x = (h & 1) ? 1.0f : -1.0f;
    grad.y = (h & 2) ? 1.0f : -1.0f;
    grad.z = (h & 4) ? 1.0f : -1.0f;
    
    switch ((h >> 3) & 3) {
        case 0: grad.x = 0; break;
        case 1: grad.y = 0; break;
        case 2: grad.z = 0; break;
        default: break; 
    }
    
    return grad;
}

Vector2 GetGradient2D(int32_t x, int32_t y, int32_t seed) {
    uint32_t h = hash2D(x, y);
    
    Vector2 grad;
    grad.x = (h & 1) ? 1.0f : -1.0f;
    grad.y = (h & 2) ? 1.0f : -1.0f;
    
    if ((h >> 2) & 1) {
        if ((h >> 3) & 1) {
            grad.x = 0;  // (0, ±1)
        } else {
            grad.y = 0;  // (±1, 0)
        }
    }
    
    return grad;
}
float Fade(float t) { 
    return t * t * t * (t * (t * 6 - 15) + 10); 
}

float BasicPerlinNoise2D(float xPos, float yPos) {
    int x0 = static_cast<int>(std::floor(xPos));
    int y0 = static_cast<int>(std::floor(yPos));
    
    float localX = xPos - x0;
    float localY = yPos - y0;
    
    Vector2 g00 = GetGradient2D(x0,     y0,     seed);
    Vector2 g10 = GetGradient2D(x0 + 1, y0,     seed);
    Vector2 g01 = GetGradient2D(x0,     y0 + 1, seed);
    Vector2 g11 = GetGradient2D(x0 + 1, y0 + 1, seed);
    
    float d00 = g00.Dot({localX,     localY});
    float d10 = g10.Dot({localX - 1, localY});
    float d01 = g01.Dot({localX,     localY - 1});
    float d11 = g11.Dot({localX - 1, localY - 1});
    
    float u = Fade(localX);
    float v = Fade(localY);
    
    float x0_interp = Lerp(d00, d10, u);
    float x1_interp = Lerp(d01, d11, u);
    
    return Lerp(x0_interp, x1_interp, v);
}

float BasicPerlinNoise(float xPos, float yPos, float zPos) {
    int x0 = static_cast<int>(std::floor(xPos));
    int y0 = static_cast<int>(std::floor(yPos));
    int z0 = static_cast<int>(std::floor(zPos));
    
    float localX = xPos - x0;
    float localY = yPos - y0;
    float localZ = zPos - z0;
    
    Vector3 g000 = GetGradient(x0,     y0,     z0,     seed);
    Vector3 g100 = GetGradient(x0 + 1, y0,     z0,     seed);
    Vector3 g010 = GetGradient(x0,     y0 + 1, z0,     seed);
    Vector3 g110 = GetGradient(x0 + 1, y0 + 1, z0,     seed);
    Vector3 g001 = GetGradient(x0,     y0,     z0 + 1, seed);
    Vector3 g101 = GetGradient(x0 + 1, y0,     z0 + 1, seed);
    Vector3 g011 = GetGradient(x0,     y0 + 1, z0 + 1, seed);
    Vector3 g111 = GetGradient(x0 + 1, y0 + 1, z0 + 1, seed);
    
    float d000 = g000.Dot({localX,     localY,     localZ});
    float d100 = g100.Dot({localX - 1, localY,     localZ});
    float d010 = g010.Dot({localX,     localY - 1, localZ});
    float d110 = g110.Dot({localX - 1, localY - 1, localZ});
    float d001 = g001.Dot({localX,     localY,     localZ - 1});
    float d101 = g101.Dot({localX - 1, localY,     localZ - 1});
    float d011 = g011.Dot({localX,     localY - 1, localZ - 1});
    float d111 = g111.Dot({localX - 1, localY - 1, localZ - 1});
    
    float u = Fade(localX);
    float v = Fade(localY);
    float w = Fade(localZ);
    
    float x00 = Lerp(d000, d100, u);
    float x01 = Lerp(d001, d101, u);
    float x10 = Lerp(d010, d110, u);
    float x11 = Lerp(d011, d111, u);
    
    float Lerp0 = Lerp(x00, x10, v);
    float Lerp1 = Lerp(x01, x11, v);
    
    return Lerp(Lerp0, Lerp1, w);
}

float PerlinNoise2D(Vector2 Pos, int Octaves, float ConstFrequency) {
    float Frequency = ConstFrequency;
    float Amplitude = 1.0f;
    float FinalNoise = 0.0f;
    float MaxValue = 0.0f; 
    
    for (int i = 0; i < Octaves; ++i) {
        FinalNoise += BasicPerlinNoise2D(
            Pos.x * Frequency, 
            Pos.y * Frequency
        ) * Amplitude;
        
        MaxValue += Amplitude;
        Frequency *= 2.0f;
        Amplitude *= 0.5f;
    }
    
    FinalNoise /= MaxValue;
    return SDL_clamp(FinalNoise, -1.0f, 1.0f);
}

float PerlinNoise(Vector3 Pos, int Octaves, float ConstFrequency) {
    float Frequency = ConstFrequency;
    float Amplitude = 1.0f; 
    float FinalNoise = 0.0f;
    float MaxValue = 0.0f;
    
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
    
    FinalNoise /= MaxValue;
    return SDL_clamp(FinalNoise, -1.0f, 1.0f);
}

void SetSeed(int givenSeed) { 
    seed = givenSeed; 
    PrintLog("Seed: " + std::to_string(seed));
}