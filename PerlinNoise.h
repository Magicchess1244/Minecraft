#ifndef __PERLINNOISE_HPP__
#define __PERLINNOISE_HPP__

typedef struct
{
	float x;
	float y;
} Vector2;

float Lerp(float a, float b, float t);
float PerlinNoise(float xPos, float yPos, int Octaves, int Type, float ConstFrecuancy);
void SetGradients();

#endif