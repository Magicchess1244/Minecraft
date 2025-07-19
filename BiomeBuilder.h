#ifndef __BIOMEBUILDER_HPP__
#define __BIOMEBUILDER_HPP__

#include "common.hpp"

typedef struct {
	int MaxHumidity;
	int MaxTemperature;
	int MinHumidity;
	int MinTemperature;
	int BaseHeight;
	int ChangeAmount;
} Biome;
typedef struct {
	double x;
	double y;
} HeightsDif;

const double ySize = 64;

const Biome Biomes[11] = {
	{ 20, 20 , 0, 0, 20, 6}, // Ice
	{ 40, 20 , 20, 0, 20, 6}, // Tundra
	{ 100, 20 , 40, 0, 20, 7}, // Taiga
	{ 100, 40 , 60, 20, 20, 4}, // Big Taiga
	{ 60, 60 , 20, 20, 20, 3 }, // Plains
	{ 80, 60 , 20, 40, 20, 6 }, // Forest
	{ 80, 60 , 20, 60, 20, 5 }, // Birch
	{ 100, 60 , 20, 80, 20, 5 }, // Dark Forest
	{ 100, 100 , 60, 60, 20, 7 }, // Jungle
	{ 60, 100 , 0, 80, 20, 4 }, // Desert
	{ 40, 80 , 20, 0, 20, 5 }, // Savanna
};
const HeightsDif ContinentelnessHeight[8] = {
	{1, ySize},
	{0.4f, ySize * 0.9f},
	{0.15f, ySize * 0.8f},
	{-0.15f, ySize * 0.45f},
	{-0.35f, ySize * 0.45f},
	{-0.65f, ySize * 0.1f},
	{-0.9f, ySize * 0.09f},
	{-1.0f, ySize}
};
const HeightsDif ErrotionHeight[10] = {
	{1, ySize * 0.05f},
	{0.8f, ySize * 0.1f},
	{0.7f, ySize * 0.35f},
	{0.4f, ySize * 0.35f},
	{0.3f, ySize * 0.11f},
	{-0.2f, ySize * 0.2f},
	{-0.4f, ySize * 0.6f},
	{-0.5f, ySize * 0.5f},
	{0.8f, ySize * 0.9f},
	{0, ySize},
};
const HeightsDif PeaksAndValiesHeight[6] = {
	{1, ySize},
	{0.8f, ySize * 0.9f},
	{0.5f, ySize * 0.95f},
	{0.05f, ySize * 0.35f},
	{-0.4f, ySize * 0.3f},
	{-0.9f, ySize * 0.1f},
};

Biome GetBiome(double Humudity, double Temperature);
int GetHeight(double Continentalness, double Errotion, double PeakAndVallies);

#endif