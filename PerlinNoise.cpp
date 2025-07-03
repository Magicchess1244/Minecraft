#include "PerlinNoise.h"
#include <iostream>
#include <cmath>
#include <vector>

#define PI 3.1415926535
#define AngleToRadians(angle) ((angle * 36) * (PI / 180.0f))

short xGradients[500][10][500];
short zGradients[500][10][500];

float DotProduct(Vector3 a, Vector3 b)
{
	return a.x * b.x + a.y * b.y;
}
float Lerp(float a, float b, float t)
{
	return a + t * (b - a);
}
float Fade(float t)
{
	return 6 * pow(t, 5) - 15 * pow(t, 4) + 10 * pow(t, 3);
}
float Clamp(float t, float Min, float Max) {
	if (t > Max) return Max;
	else if (t < Min) return Min;
	else return t;
}
float BasicPerlinNoise(float xPos, float yPos, float zPos)
{
	const int xMinG = ((int)(xPos / 8));
	const int yMinG = ((int)(yPos / 8));
	const int zMinG = ((int)(zPos / 8));
	const int xMin = xMinG * 8;
	const int yMin = yMinG * 8;
	const int zMin = zMinG * 8;
	const int xMax = xMin + 8;
	const int yMax = yMin + 8;
	const int zMax = zMin + 8;

	Vector3 Gra[8] = {};
	int index = 0;

	for (int i = 0; i < 2; ++i)
	{
		int y0 = yMinG + i;
		int y1 = yMinG + i + 1;
		int z0 = zMinG + i;

		// First point
		float theta = AngleToRadians(zGradients[xMinG][y0][z0]); // inclination
		float phi = AngleToRadians(xGradients[xMinG][y0][z0]); // azimuth
		Gra[index] = {
			sinf(theta) * cosf(phi),
			sinf(theta) * sinf(phi),
			cosf(theta)
		};

		// Second point
		theta = AngleToRadians(zGradients[xMinG + 1][y0][z0]);
		phi = AngleToRadians(xGradients[xMinG + 1][y0][z0]);
		Gra[index + 1] = {
			sinf(theta) * cosf(phi),
			sinf(theta) * sinf(phi),
			cosf(theta)
		};

		// Third point
		theta = AngleToRadians(zGradients[xMinG][y1][z0]);
		phi = AngleToRadians(xGradients[xMinG][y1][z0]);
		Gra[index + 2] = {
			sinf(theta) * cosf(phi),
			sinf(theta) * sinf(phi),
			cosf(theta)
		};

		// Fourth point
		theta = AngleToRadians(zGradients[xMinG + 1][y1][z0]);
		phi = AngleToRadians(xGradients[xMinG + 1][y1][z0]);
		Gra[index + 3] = {
			sinf(theta) * cosf(phi),
			sinf(theta) * sinf(phi),
			cosf(theta)
		};

		index += 4;
	}


	Vector3 D0 = { xPos - xMin, yPos - yMin, zPos - zMin };
	Vector3 D1 = { xPos - xMax, yPos - yMin, zPos - zMin };
	Vector3 D2 = { xPos - xMin, yPos - yMax, zPos - zMin };
	Vector3 D3 = { xPos - xMax, yPos - yMax, zPos - zMin };
	Vector3 D4 = { xPos - xMin, yPos - yMin, zPos - zMax };
	Vector3 D5 = { xPos - xMax, yPos - yMin, zPos - zMax };
	Vector3 D6 = { xPos - xMin, yPos - yMax, zPos - zMax };
	Vector3 D7 = { xPos - xMax, yPos - yMax, zPos - zMax };

	std::vector<int> FinalLerp;

	for (int i = 0; i < 5; i += 4)
	{
		float Lerp1 = Lerp(DotProduct(Gra[i], D0), DotProduct(Gra[i + 1], D1), Fade((float)(xPos - xMin) / 8));
		float Lerp2 = Lerp(DotProduct(Gra[i + 2], D2), DotProduct(Gra[i + 3], D3), Fade((float)(xPos - xMin) / 8));
		FinalLerp.push_back(Lerp(Lerp1, Lerp2, Fade((float)(yPos - yMin) / 8)));
	}
	
	return Lerp(FinalLerp[0], FinalLerp[1], Fade((float)(zPos - zMin) / 8));
}
float PerlinNoise(Vector3 Pos, int Octaves, float ConstFrequency)
{
	float Frequency = ConstFrequency;
	float Amplitud = ConstFrequency;
	float FinalNoise = 0.0f;

	for (int i = 0; i <= Octaves; i++){
		FinalNoise += BasicPerlinNoise(Pos.x * Frequency, Pos.y * Frequency, Pos.z * Frequency) * Amplitud;
		Frequency *= 2;
		Amplitud /= 2;
	}

	FinalNoise *= 1.2f;
	return Clamp(FinalNoise, -1.0f, 1.0f);
}
void SetGradients()
{
	for (int x = 0; x < 500; x++) {
		for (int y = 0; y < 10; y++) {
			for (int z = 0; z < 500; z++) {
				xGradients[x][y][z] = rand() % 10;
				zGradients[x][y][z] = rand() % 10;
			}
		}
	}
}