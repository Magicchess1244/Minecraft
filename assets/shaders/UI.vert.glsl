#version 460

layout (location = 0) in vec3 a_pos_uv;
layout (location = 1) in vec4 a_color;
layout (location = 2) in float a_blockID;

layout (location = 0) out vec4 v_color;
layout (location = 1) out vec2 v_uv;
layout (location = 2) flat out float v_blockID;

void main() {
    gl_Position = vec4(a_pos_uv.xy, 0.0, 1.0);
    v_color = a_color;
    
    // Unpack UV from Z: 2x16 bit
    uint packedUV = floatBitsToUint(a_pos_uv.z);
    float u = float((packedUV >> 16u) & 0xFFFFu) / 65535.0;
    float v = float(packedUV & 0xFFFFu) / 65535.0;
    v_uv = vec2(u, v);
    
    v_blockID = a_blockID;
}
