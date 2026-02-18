#version 460

layout (location = 0) in vec4 v_color;
layout (location = 1) in vec2 v_uv;
layout (location = 2) flat in float v_blockID;
layout (location = 3) in vec3 v_pos;
layout (location = 4) in float v_light;
layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) uniform sampler2D u_texture;

layout(set = 3, binding = 0, std140) uniform Water { uint water; } inWater;


float NearFog = 50.0;
float FarFog = 150.0;
float NearFogInWater = 5.0;
float FarFogInWater = 25.0;

void main()
{
  int tileIndex = int(v_blockID + 0.5); 
  float tileX = float(tileIndex % 4);
  float tileY = float(tileIndex / 4);
  
  vec2 uv = clamp(fract(v_uv), 0.0, 1.0);
  
  // Water effect (Block ID 5)
  float alpha = 1.0;
  
  // Use a small inset to avoid bleeding from neighboring tiles
  vec2 insetUV = uv * 0.98 + 0.01;
  vec2 atlasUV = (vec2(tileX, tileY) + insetUV) * 0.25;
  vec4 texColor = texture(u_texture, atlasUV);
  
  // If texture is black/transparent, use a fallback to see if it's there
  if (texColor.rgb == vec3(0.0, 0.0, 0.0) && texColor.a < 0.1) {
      texColor = vec4(1.0, 0.0, 1.0, 1.0); // Magenta fallback
  }

  vec3 finalColor = v_color.rgb * texColor.rgb;

  float lightLevel = clamp(v_light / 14.0, 0.2, 1.0);
  finalColor *= lightLevel;
  if (abs(v_blockID - 5.0) < 0.1) {
    // Water
    finalColor = mix(finalColor, vec3(0.0, 0.3, 0.7), 0.5); // Stronger blue
    alpha = 0.6;
  }
  
  // DISTANCE FOG (User's custom logic)
  float dist = v_pos.z;
  float fogFactor = clamp((dist - NearFog) / (FarFog - NearFog), 0.0, 1.0);
  vec3 skyColor = vec3(0.45, 0.75, 1.0);
  finalColor = mix(finalColor, skyColor, fogFactor);

  // Underwater tint
  if (inWater.water == 1) {
    // DISTANCE FOG (User's custom logic)
    float dist = v_pos.z;
    float fogFactor = clamp((dist - NearFogInWater) / (FarFogInWater - NearFogInWater), 0.0, 1.0);
    vec3 skyColor = vec3(0.45, 0.75, 1.0);
    finalColor = mix(finalColor, skyColor, fogFactor);

    finalColor = mix(finalColor, vec3(0.0, 0.1, 0.6), 0.6); // Deep blue tint
    finalColor *= 0.9; // Slightly darken
  }

  FragColor = vec4(finalColor, alpha);
}