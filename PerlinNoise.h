#ifndef __PERLINNOISE_HPP__
#define __PERLINNOISE_HPP__

#include "common.hpp"

double DotProduct(Vector3 a, Vector3 b);
double Lerp(double a, double b, double t);
double PerlinNoise(Vector3 Pos, int Octaves, double ConstFrequency);
void SetGradients();

#endif