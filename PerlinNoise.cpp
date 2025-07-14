#include "PerlinNoise.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>

#define PI 3.1415926535f
#define AngleToRadians(angle) ((angle) * (PI / 180.0f))

short xGradients[100][10][100];
short zGradients[100][10][100];

float DotProduct(Vector3 a, Vector3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
float Lerp(float a, float b, float t)
{
	return a + t * (b - a);
}
float Fade(float t)
{
	return t * t * t * (t * (t * 6 - 15) + 10); // Classic Perlin fade function
}
float Clamp(float t, float Min, float Max)
{
	if (t > Max) return Max;
	else if (t < Min) return Min;
	else return t;
}
Vector3 GradientFromAngles(short xAngle, short zAngle)
{
	float theta = AngleToRadians(zAngle); // inclination
	float phi = AngleToRadians(xAngle);  // azimuth

	return {
		sinf(theta) * cosf(phi),
		sinf(theta) * sinf(phi),
		cosf(theta)
	};
}
float BasicPerlinNoise(float xPos, float yPos, float zPos)
{
	int x0 = static_cast<int>(xPos) / 8;
	int y0 = static_cast<int>(yPos) / 8;
	int z0 = static_cast<int>(zPos) / 8;

	float localX = (xPos - x0 * 8) / 8.0f;
	float localY = (yPos - y0 * 8) / 8.0f;
	float localZ = (zPos - z0 * 8) / 8.0f;

	// Collect gradients
	Vector3 gradients[8];
	for (int i = 0; i < 2; ++i)
	{
		for (int j = 0; j < 2; ++j)
		{
			for (int k = 0; k < 2; ++k)
			{
				int index = i * 4 + j * 2 + k;
				gradients[index] = GradientFromAngles(
					xGradients[x0 + i][y0 + j][z0 + k],
					zGradients[x0 + i][y0 + j][z0 + k]
				);
			}
		}
	}

	// Relative positions
	Vector3 rel[8] = {
		{localX,     localY,     localZ},
		{localX - 1, localY,     localZ},
		{localX,     localY - 1, localZ},
		{localX - 1, localY - 1, localZ},
		{localX,     localY,     localZ - 1},
		{localX - 1, localY,     localZ - 1},
		{localX,     localY - 1, localZ - 1},
		{localX - 1, localY - 1, localZ - 1}
	};

	float dots[8];
	for (int i = 0; i < 8; ++i)
		dots[i] = DotProduct(gradients[i], rel[i]);

	// Interpolation
	float u = Fade(localX);
	float v = Fade(localY);
	float w = Fade(localZ);

	// Lerp between 8 corners
	float x00 = Lerp(dots[0], dots[1], u);
	float x01 = Lerp(dots[4], dots[5], u);
	float x10 = Lerp(dots[2], dots[3], u);
	float x11 = Lerp(dots[6], dots[7], u);

	float Lerp0 = Lerp(x00, x10, v);
	float Lerp1 = Lerp(x01, x11, v);

	return Lerp(Lerp0, Lerp1, w);
}
float PerlinNoise(Vector3 Pos, int Octaves, float ConstFrequency)
{
	float Frequency = ConstFrequency;
	float Amplitude = ConstFrequency;
	float FinalNoise = 0.0f;

	for (int i = 0; i <= Octaves; i++) {
		FinalNoise += BasicPerlinNoise(Pos.x * Frequency, Pos.y * Frequency, Pos.z * Frequency) * Amplitude;
		Frequency *= 2.0f;
		Amplitude /= 2.0f;
	}

	FinalNoise *= 1.2f;
	return Clamp(FinalNoise, -1.0f, 1.0f);
}
void SetGradients()
{
	for (int x = 0; x < 100; x++) {
		for (int y = 0; y < 10; y++) {
			for (int z = 0; z < 100; z++) {
				xGradients[x][y][z] = rand() % 360;
				zGradients[x][y][z] = rand() % 180;
			}
		}
	}
}
