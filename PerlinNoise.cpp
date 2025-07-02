#include "PerlinNoise.h"
#include <iostream>
#include <cmath>
#define PI 3.1415926535

short (*Gradients)[10];
short CaveGra[1000][10]{};
short ContinentalnesGra[1000][2]{};
short PeaksAndValliesGra[1000][2]{};
short ErrotionGra[1000][2]{};
short TemperatureGra[1000][2]{};
short HumidGra[1000][2]{};

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
float BasicPerlinNoise(float xPos, float yPos)
{
	const int xMinG = ((int)(xPos / 8));
	const int xMin = xMinG * 8;
	const int yMinG = ((int)(yPos / 8));
	const int yMin = yMinG * 8;
	const int xMax = xMin + 8;
	const int yMax = yMin + 8;

	Vector3 G0 = { cos((Gradients[xMinG][yMinG] * 36) * PI / 180), sin((Gradients[xMinG][yMinG] * 36) * PI / 180) };
	Vector3 G1 = { cos((Gradients[xMinG + 1][yMinG] * 36) * PI / 180), sin((Gradients[xMinG + 1][yMinG] * 36) * PI / 180) };
	Vector3 G2 = { cos((Gradients[xMinG][yMinG + 1] * 36) * PI / 180), sin((Gradients[xMinG][yMinG + 1] * 36) * PI / 180) };
	Vector3 G3 = { cos((Gradients[xMinG + 1][yMinG + 1] * 36) * PI / 180), sin((Gradients[xMinG + 1][yMinG + 1] * 36) * PI / 180) };

	Vector3 D0 = { xPos - xMin, yPos - yMin };
	Vector3 D1 = { xPos - xMax, yPos - yMin };
	Vector3 D2 = { xPos - xMin, yPos - yMax };
	Vector3 D3 = { xPos - xMax, yPos - yMax };

	float Lerp1 = Lerp(DotProduct(G0, D0), DotProduct(G1, D1), Fade((float)(xPos - xMin) / 8));
	float Lerp2 = Lerp(DotProduct(G2, D2), DotProduct(G3, D3), Fade((float) (xPos - xMin) / 8));
	float FinalLerp = Lerp(Lerp1, Lerp2, Fade((float) (yPos - yMin) / 8));
	
	return FinalLerp;
}
float PerlinNoise(float xPos, float yPos, int Octaves, int Type, float ConstFrequency)
{
	float Frequency = ConstFrequency;
	float Amplitud = ConstFrequency;
	float FinalNoise = 0.0f;

	if (Type == 0) {
		Gradients = (short (*)[10]) ContinentalnesGra;	
	}
	else if (Type == 1)
	{
		Gradients = (short (*)[10]) HumidGra;
	}
	else if (Type == 2)
	{
		Gradients = (short (*)[10]) TemperatureGra;
	}
	else if (Type == 3)
	{
		Gradients = CaveGra;
	}
	else if (Type == 4)
	{
		Gradients = (short (*)[10])ErrotionGra;
	}
	else if (Type == 5)
	{
		Gradients = (short (*)[10])PeaksAndValliesGra;
	}

	for (int i = 0; i <= Octaves; i++){
		FinalNoise += BasicPerlinNoise(xPos * Frequency, yPos * Frequency) * Amplitud;
		Frequency *= 2;
		Amplitud /= 2;
	}

	FinalNoise *= 1.2f;
	return Clamp(FinalNoise, -1.0f, 1.0f);
}
void SetGradients()
{
	for (int i = 0; i < 1000; i++) {
		HumidGra[i][0] = rand() % 10;
		HumidGra[i][1] = rand() % 10;

		TemperatureGra[i][0] = rand() % 10;
		TemperatureGra[i][1] = rand() % 10;

		ContinentalnesGra[i][0] = rand() % 10;
		ContinentalnesGra[i][1] = rand() % 10;

		for (int j = 0; j < 10; j++) {
			CaveGra[i][j] = rand() % 10;
			//std::cout << "Gradient[" << i << "][" << j << "]: " << CaveGra[i][j] << "\n";
		}
	}
	//std::cout << "Gradient: " << CaveGra[0][0];
}