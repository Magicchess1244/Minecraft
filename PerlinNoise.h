#ifndef __PERLINNOISE_HPP__
#define __PERLINNOISE_HPP__

#include "common.hpp"

float PerlinNoise(Vector3 Pos, int Octaves, float ConstFrequency);
void SetSeed(int givenSeed);

#endif