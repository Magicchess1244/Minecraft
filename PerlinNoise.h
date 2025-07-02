#ifndef __PERLINNOISE_HPP__
#define __PERLINNOISE_HPP__

typedef struct
{
	float x, y, z;
} Vector3;

float Lerp(float a, float b, float t);
float PerlinNoise(float xPos, float yPos, int Octaves, int Type, float ConstFrecuancy);
void SetGradients();

#endif