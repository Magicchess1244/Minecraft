#include "BiomeBuilder.h"

int BaseHeight(double ValueNoise, int Length, const HeightsDif* Heights)
{
	for (int i = 0; i < Length - 1; i++) {
		if (ValueNoise > ContinentelnessHeight[i].x) {
			std::cout << "Continentalness: " << Lerp(Heights[i].y, Heights[i + 1].y, (ValueNoise + 1 / (Heights[i + 1].x + 1)))  << std::endl;
			return Lerp(Heights[i + 1].y, Heights[i].y, (ValueNoise + 1 / (Heights[i + 1].x + 1)));
		}
	}
	return Heights[Length - 1].y;
}

Biome GetBiome(double Humudity, double Temperature)
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

int GetHeight(double Continentalness, double Errotion, double PeakAndValleys) {
	return (int)(BaseHeight(Continentalness, 8, ContinentelnessHeight));
}

