#pragma once

#include "Common.hpp"

// Entity type definitions
enum class EntityType {
  PLAYER,
  // Add more entity types here as needed
  ENTITY_COUNT
};

struct EntityDefinition {
  char Name[20];
  Vector3 Model[6][4];
};

constexpr float ENTITY_PLAYER_RADIUS = 0.4f;

const EntityDefinition EntityDefinitions[] = {
    // PLAYER
    {"Player",
     {
         {                                                       // Front (+Z)
          {ENTITY_PLAYER_RADIUS, -1.62f, ENTITY_PLAYER_RADIUS},  // bottom-right
          {-ENTITY_PLAYER_RADIUS, -1.62f, ENTITY_PLAYER_RADIUS}, // bottom-left
          {ENTITY_PLAYER_RADIUS, 0.18f, ENTITY_PLAYER_RADIUS},   // top-right
          {-ENTITY_PLAYER_RADIUS, 0.18f, ENTITY_PLAYER_RADIUS}}, // top-left

         {                                                        // Back (-Z)
          {-ENTITY_PLAYER_RADIUS, -1.62f, -ENTITY_PLAYER_RADIUS}, // bottom-left
          {ENTITY_PLAYER_RADIUS, -1.62f, -ENTITY_PLAYER_RADIUS}, // bottom-right
          {-ENTITY_PLAYER_RADIUS, 0.18f, -ENTITY_PLAYER_RADIUS}, // top-left
          {ENTITY_PLAYER_RADIUS, 0.18f, -ENTITY_PLAYER_RADIUS}}, // top-right

         {                                                      // Right (+X)
          {ENTITY_PLAYER_RADIUS, 0.18f, ENTITY_PLAYER_RADIUS},  // front-top
          {ENTITY_PLAYER_RADIUS, 0.18f, -ENTITY_PLAYER_RADIUS}, // back-top
          {ENTITY_PLAYER_RADIUS, -1.62f, ENTITY_PLAYER_RADIUS}, // front-bottom
          {ENTITY_PLAYER_RADIUS, -1.62f, -ENTITY_PLAYER_RADIUS}}, // back-bottom

         {                                                        // Left (-X)
          {-ENTITY_PLAYER_RADIUS, 0.18f, -ENTITY_PLAYER_RADIUS},  // back-top
          {-ENTITY_PLAYER_RADIUS, 0.18f, ENTITY_PLAYER_RADIUS},   // front-top
          {-ENTITY_PLAYER_RADIUS, -1.62f, -ENTITY_PLAYER_RADIUS}, // back-bottom
          {-ENTITY_PLAYER_RADIUS, -1.62f,
           ENTITY_PLAYER_RADIUS}}, // front-bottom

         {                                                       // Top (+Y)
          {-ENTITY_PLAYER_RADIUS, 0.18f, -ENTITY_PLAYER_RADIUS}, // back-left
          {ENTITY_PLAYER_RADIUS, 0.18f, -ENTITY_PLAYER_RADIUS},  // back-right
          {-ENTITY_PLAYER_RADIUS, 0.18f, ENTITY_PLAYER_RADIUS},  // front-left
          {ENTITY_PLAYER_RADIUS, 0.18f, ENTITY_PLAYER_RADIUS}},  // front-right

         {                                                        // Bottom (-Y)
          {ENTITY_PLAYER_RADIUS, -1.62f, ENTITY_PLAYER_RADIUS},   // front-right
          {ENTITY_PLAYER_RADIUS, -1.62f, -ENTITY_PLAYER_RADIUS},  // back-right
          {-ENTITY_PLAYER_RADIUS, -1.62f, ENTITY_PLAYER_RADIUS},  // front-left
          {-ENTITY_PLAYER_RADIUS, -1.62f, -ENTITY_PLAYER_RADIUS}} // back-left
     }}};

#define EntityDef EntityDefinitions
