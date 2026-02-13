#version 460

layout (location = 0) in vec4 v_color;
layout (location = 1) in vec2 v_uv;
layout (location = 2) flat in float v_blockID;
layout (location = 3) in vec3 PixelPos;
layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) uniform sampler2D u_texture;

float FogStart = 20;
float FogEnd = 75;
vec3 FogColor = {0.75f, 0.75f, 0.75f};
bool AddFog = true;

float Fog(){
    float FogRange = FogEnd - FogStart;
    float FogDist = FogEnd - PixelPos.z;
    float FogFactor = FogDist / FogRange;
    FogFactor = clamp(FogFactor, 0, 1);
    return FogFactor;
}

void main()
{
    const float tileSize = 16.0;
    const float atlasSize = 64.0;
    const float tilesPerRow = 4.0;
    
    // Use flat block ID
    int tileIndex = int(v_blockID + 0.5); 
    
    // Calculate tile coordinates in the atlas (0,0 is top-left)
    float tileX = float(tileIndex % 4);
    float tileY = float(tileIndex / 4);
    
    // Tiling within the face: v_uv goes from 0..fw, 0..fh
    vec2 localUV = fract(v_uv);
    
    // Invert Y because our vertex data mapping (Bottom=1, Top=0) is inverted 
    // relative to standard top-left UV origins in many texture maps.
    // If textures look upside down, we toggle this.
    // localUV.y = 1.0 - localUV.y;

    // To prevent bleeding: nudge the local UVs slightly inward
    // This samples from the 15.8x15.8 center of each 16x16 tile
    const float nudge = 0.1 / tileSize; 
    vec2 nudgedUV = localUV * (1.0 - 2.0 * nudge) + nudge;

    // Final atlas coordinates
    vec2 atlasUV = (vec2(tileX, tileY) * tileSize + nudgedUV * tileSize) / atlasSize;
    vec4 FinalColor = v_color * texture(u_texture, atlasUV);

    if(AddFog){
        FinalColor = mix(vec4(FogColor, 1), FinalColor, Fog());
    }
    FragColor = FinalColor;
}