#pragma once

#include "Common.hpp"

struct BoundingBox {
  Vector3 min;
  Vector3 max;
};

struct RecipeItem {
  int blockID;
  int amount;
};

struct Block {
  char Name[20];
  short Textures[6]; // Front, Back, Right, Left, Top, Bottom
  short Luminance;
  short Drop;
  bool hasUI;
  bool isFunctional;
  bool isTransparent;
  bool isWater;
  bool isSolid;
  bool isFullBlock;

  RecipeItem *recipe;
  BoundingBox *collisionBoxes;
  std::pair<int, int> *Storage;
};

const Block BlockDefinitions[] = {
    // 0: Air
    {"Air",
     {0, 0, 0, 0, 0, 0},
     0,
     0,
     false,
     false,
     true,
     false,
     false,
     false,
     nullptr,
     nullptr,
     nullptr},
    // 1: Grass
    {"Grass",
     {1, 1, 1, 1, 0, 2},
     0,
     2,
     false,
     false,
     false,
     false,
     true,
     true,
     nullptr,
     nullptr,
     nullptr},
    // 2: Dirt
    {"Dirt",
     {2, 2, 2, 2, 2, 2},
     0,
     2,
     false,
     false,
     false,
     false,
     true,
     true,
     nullptr,
     nullptr,
     nullptr},
    // 3: Stone
    {"Stone",
     {3, 3, 3, 3, 3, 3},
     0,
     3,
     false,
     false,
     false,
     false,
     true,
     true,
     nullptr,
     nullptr,
     nullptr},
    // 4: Bedrock
    {"Bedrock",
     {4, 4, 4, 4, 4, 4},
     0,
     1,
     false,
     false,
     false,
     false,
     true,
     true,
     nullptr,
     nullptr},
    // 5: Water
    {"Water",
     {5, 5, 5, 5, 5, 5},
     0,
     0,
     false,
     false,
     true,
     true,
     false,
     false,
     nullptr,
     nullptr,
     nullptr},
    // 6: Wood
    {"Wood",
     {6, 6, 6, 6, 10, 10},
     0,
     6,
     false,
     false,
     false,
     false,
     true,
     true,
     nullptr,
     nullptr,
     nullptr},
    // 7: Leaves
    {"Leaves",
     {7, 7, 7, 7, 7, 7},
     0,
     0,
     false,
     false,
     true,
     false,
     true,
     true,
     nullptr,
     nullptr,
     nullptr},
    // 8: Sand
    {"Sand",
     {8, 8, 8, 8, 8, 8},
     0,
     8,
     false,
     false,
     false,
     false,
     true,
     true,
     nullptr,
     nullptr,
     nullptr},
    // 9: Glowstone
    {"Glowstone",
     {9, 9, 9, 9, 9, 9},
     15,
     9,
     false,
     false,
     false,
     false,
     true,
     true,
     nullptr,
     nullptr,
     nullptr},
    {"Diamond",
     {11, 11, 11, 11, 11, 11},
     0,
     10,
     false,
     false,
     false,
     false,
     true,
     true,
     nullptr,
     nullptr,
     nullptr},
    {"Coal",
     {12, 12, 12, 12, 12, 12},
     0,
     11,
     false,
     false,
     false,
     false,
     true,
     true,
     nullptr,
     nullptr,
     nullptr},
    {"Redstone",
     {13, 13, 13, 13, 13, 13},
     0,
     12,
     false,
     false,
     false,
     false,
     true,
     true,
     nullptr,
     nullptr,
     nullptr},
    {"Iron",
     {14, 14, 14, 14, 14, 14},
     0,
     13,
     false,
     false,
     false,
     false,
     true,
     true,
     nullptr,
     nullptr,
     nullptr},
};

constexpr unsigned int BlockDefinitionsAmount =
    sizeof(BlockDefinitions) / sizeof(BlockDefinitions[0]);
#define BlockDef BlockDefinitions
#define BlockNum BlockDefinitionsAmount
