#ifndef __PERLINNOISE_HPP__
#define __PERLINNOISE_HPP__

#include "../core/common.hpp"

float PerlinNoise(Vector3 Pos, int Octaves, float ConstFrequency);
void SetSeed(int givenSeed);

#endif
