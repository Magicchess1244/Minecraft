#include "PerlinNoise.h"

short xGradients[100][10][100];
short zGradients[100][10][100];

double DotProduct(Vector3 a, Vector3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

double Lerp(double a, double b, double t)
{
	return a + t * (b - a);
}

double Fade(double t)
{
	return t * t * t * (t * (t * 6 - 15) + 10); // Classic Perlin fade function
}

double Clamp(double t, double Min, double Max)
{
	if (t > Max) return Max;
	else if (t < Min) return Min;
	else return t;
}

Vector3 GradientFromAngles(short xAngle, short zAngle)
{
	double theta = AngleToRadians(zAngle); // inclination
	double phi = AngleToRadians(xAngle);  // azimuth 

	return {
		sin(theta) * cos(phi),
		sin(theta) * sin(phi),
		cos(theta)
	};
}

double BasicPerlinNoise(double xPos, double yPos, double zPos)
{
	int x0 = static_cast<int>(xPos) / 8;
	int y0 = static_cast<int>(yPos) / 8;
	int z0 = static_cast<int>(zPos) / 8;

	double localX = (xPos - x0 * 8) / 8.0f;
	double localY = (yPos - y0 * 8) / 8.0f;
	double localZ = (zPos - z0 * 8) / 8.0f;

	// Collect gradients
	Vector3 gradients[8] = { 0 };
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

	int dots[8] = { 0 };
	for (int i = 0; i < 8; ++i)
		dots[i] = DotProduct(gradients[i], rel[i]);

	// Interpolation
	double u = Fade(localX);
	double v = Fade(localY);
	double w = Fade(localZ);

	// Lerp between 8 corners
	double x00 = Lerp(dots[0], dots[1], u);
	double x01 = Lerp(dots[4], dots[5], u);
	double x10 = Lerp(dots[2], dots[3], u);
	double x11 = Lerp(dots[6], dots[7], u);

	double Lerp0 = Lerp(x00, x10, v);
	double Lerp1 = Lerp(x01, x11, v);

	return Lerp(Lerp0, Lerp1, w);
}
double PerlinNoise(Vector3 Pos, int Octaves, double ConstFrequency)
{
	double Frequency = ConstFrequency;
	double Amplitude = ConstFrequency;
	double FinalNoise = 0.0f;

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
