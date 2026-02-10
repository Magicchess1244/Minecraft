#ifndef __PERLINNOISE_HPP__
#define __PERLINNOISE_HPP__

#include "Common.hpp"

float PerlinNoise(Vector3 Pos, int Octaves, float ConstFrequency);
float PerlinNoise2D(Vector2 Pos, int Octaves, float ConstFrequency);
void SetSeed(int givenSeed);

#endif