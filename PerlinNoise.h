#ifndef __PERLINNOISE_HPP__
#define __PERLINNOISE_HPP__

#include "common.hpp"

double PerlinNoise(Vector3 Pos, int Octaves, double ConstFrequency);
void SetSeed(int givenSeed);

#endif