#version 460

layout (location = 0) in float v_color;
layout (location = 1) in vec2 v_uv;
layout (location = 2) flat in float v_blockID;
layout (location = 3) in vec3 v_pos;
layout (location = 4) in float v_light;
layout (location = 0) out vec4 FragColor;

struct BlockData {
    uint textures[6];
    uint luminance;
    uint type;
};

layout (set = 2, binding = 0) uniform sampler2D u_texture;
layout (set = 2, binding = 1) uniform sampler2D u_blockProps;

layout(set = 3, binding = 0, std140) uniform Water { uint water; } inWater;

float NearFog = 100.0;
float FarFog = 150.0;
float NearFogInWater = 5.0;
float FarFogInWater = 50.0;
int BlockInAtlasRow = 7;
int BlockInAtlasCol = 32;

const vec3 FaceLight[6] = vec3[6](
    vec3(0.80, 0.80, 0.80),  // Front  (+Z)
    vec3(0.80, 0.80, 0.80),  // Back   (-Z)
    vec3(0.70, 0.70, 0.70),  // Right  (+X)
    vec3(0.70, 0.70, 0.70),  // Left   (-X)
    vec3(1.00, 1.00, 1.00),  // Top    (+Y) — brightest
    vec3(0.60, 0.60, 0.60)   // Bottom (-Y) — darkest
);

void main()
{
  int blockIdx = int(v_blockID + 0.5);
  int side = int(v_color + 0.5);

  vec4 t0 = texelFetch(u_blockProps, ivec2(0, blockIdx), 0);
  vec4 t1 = texelFetch(u_blockProps, ivec2(1, blockIdx), 0);
  vec4 t2 = texelFetch(u_blockProps, ivec2(2, blockIdx), 0);
  vec4 t3 = texelFetch(u_blockProps, ivec2(3, blockIdx), 0);

  uint textures[6];
  textures[0] = uint(t0.r * 255.0 + 0.5) | (uint(t0.g * 255.0 + 0.5) << 8);
  textures[1] = uint(t0.b * 255.0 + 0.5) | (uint(t0.a * 255.0 + 0.5) << 8);
  textures[2] = uint(t1.r * 255.0 + 0.5) | (uint(t1.g * 255.0 + 0.5) << 8);
  textures[3] = uint(t1.b * 255.0 + 0.5) | (uint(t1.a * 255.0 + 0.5) << 8);
  textures[4] = uint(t2.r * 255.0 + 0.5) | (uint(t2.g * 255.0 + 0.5) << 8);
  textures[5] = uint(t2.b * 255.0 + 0.5) | (uint(t2.a * 255.0 + 0.5) << 8);
  uint luminance = uint(t3.r * 255.0 + 0.5) | (uint(t3.g * 255.0 + 0.5) << 8);
  uint type      = uint(t3.b * 255.0 + 0.5) | (uint(t3.a * 255.0 + 0.5) << 8);

  uint tileIndex = textures[side];
  
  vec2 uv = clamp(fract(v_uv), 0.0, 1.0);
  
  // Water effect (Block ID 5)
  float alpha = 1.0;
  
  // Use a small inset to avoid bleeding from neighboring tiles
  vec2 insetUV = uv * 0.98 + 0.01;

  float tileX = float(tileIndex % BlockInAtlasCol);
  float tileY = float(tileIndex / BlockInAtlasCol);

  vec2 atlasUV = (vec2(tileX, tileY) + insetUV) / vec2(float(BlockInAtlasCol), float(BlockInAtlasRow));
  
  vec4 texColor = texture(u_texture, atlasUV);

  vec3 finalColor = FaceLight[side] * texColor.rgb;

  if (v_blockID == 5) {
    // Water
    finalColor = mix(finalColor, vec3(0.0, 0.3, 0.7), 0.5); // Stronger blue
    alpha = 0.6;
  }

  float lightLevel = clamp(v_light / 15.0, 0.06, 1.0);
  finalColor *= lightLevel;
  
  float dist = v_pos.z;
  float fogFactor = clamp((dist - NearFog) / (FarFog - NearFog), 0.0, 0.8);
  vec3 skyColor = vec3(0.9, 0.9, 0.9);
  finalColor = mix(finalColor, skyColor, fogFactor);

  // Underwater tint
  if (inWater.water == 1) {
    float dist = v_pos.z;
    float fogFactor = clamp((dist - NearFogInWater) / (FarFogInWater - NearFogInWater), 0.0, 1.0);
    vec3 skyColor = vec3(0.45, 0.75, 1.0);
    finalColor = mix(finalColor, skyColor, fogFactor);

    finalColor = mix(finalColor, vec3(0.0, 0.1, 0.6), 0.6); // Deep blue tint
    finalColor *= 0.9; // Slightly darken
  }

  FragColor = vec4(finalColor, alpha);
}