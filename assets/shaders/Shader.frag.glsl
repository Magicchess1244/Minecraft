#version 460

layout (location = 0) in vec4 v_color;
layout (location = 1) in vec2 v_uv;
layout (location = 2) flat in float v_blockID;
layout (location = 3) in vec3 v_pos;
layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) uniform sampler2D u_texture;

void main()
{
    int tileIndex = int(v_blockID + 0.5); 
    float tileX = float(tileIndex % 4);
    float tileY = float(tileIndex / 4);
    
    vec2 atlasUV = (vec2(tileX, tileY) + fract(v_uv)) * 0.25;
    vec4 texColor = texture(u_texture, atlasUV);
    
    FragColor = v_color * texColor;
    if (v_blockID < 4.5 || v_blockID > 5.5) {
        FragColor.a = 1.0;
    }
}