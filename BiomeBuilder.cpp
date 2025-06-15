#include "BiomeBuilder.h"
#include "PerlinNoise.h"
#include <iostream>

float BaseHeight(float ValueNoise, int Length, const HeightsDif* Heights)
{
	for (int i = 0; i < Length - 1; i++) {
		if (ValueNoise > ContinentelnessHeight[i].x) {
			std::cout << "Continentalness: " << Lerp(Heights[i].y, Heights[i + 1].y, (ValueNoise + 1 / (Heights[i + 1].x + 1)))  << std::endl;
			return Lerp(Heights[i + 1].y, Heights[i].y, (ValueNoise + 1 / (Heights[i + 1].x + 1)));
		}
	}
	return Heights[Length - 1].y;
}

Biome GetBiome(float Humudity, float Temperature)
{
	Biome TheBiome;

	for (int i = 0; i < 11; i++)
	{
		bool a = (Humudity < Biomes[i].MaxHumidity && Humudity > Biomes[i].MinHumidity);
		bool b = (Temperature < Biomes[i].MaxTemperature && Temperature > Biomes[i].MinTemperature);
		if (a && b) {
			TheBiome = Biomes[i];
		}
	}
	return TheBiome;
}

int GetHeight(float Continentalness, float Errotion, float PeakAndValleys) {
	return (int)(BaseHeight(Continentalness, 8, ContinentelnessHeight));
}

