#ifndef __PERLINNOISE_HPP__
#define __PERLINNOISE_HPP__

typedef struct
{
	float x, y, z;
} Vector3;

float Lerp(float a, float b, float t);
float PerlinNoise(Vector3 Pos, int Octaves, float ConstFrequency);
void SetGradients();

#endif