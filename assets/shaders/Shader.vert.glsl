#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in float a_data_float;

layout (location = 0) out float v_color;
layout (location = 1) out vec2 v_uv;
layout (location = 2) flat out float v_blockID;
layout (location = 3) out vec3 v_pos;
layout (location = 4) out float v_light;

layout(set = 1, binding = 0, std140) uniform ProjectionBlock { mat4 projection; } projBlock;
layout(set = 1, binding = 1, std140) uniform ViewBlock { mat4 view; } viewBlock;

void main() {
    // Reconstruct position by removing fractional bits (U and V)
    // Adding 0.05 to ensure correct floor even with floating point errors
    vec3 realPos = floor(a_position + 0.05);
    gl_Position = projBlock.projection * viewBlock.view * vec4(realPos, 1.0);
    
    // Unpack Data: side(3), blockID(16), light(4)
    uint a_data = floatBitsToUint(a_data_float);
    v_color = float(a_data & 0x7u);
    v_blockID = float((a_data >> 3u) & 0xFFFFu);
    v_light = float((a_data >> 19u) & 0xFu);
    
    // Unpack UV: fracture of X and Y
    // Use step to find if the fraction is significant (0.1)
    v_uv.x = step(0.05, fract(a_position.x));
    v_uv.y = step(0.05, fract(a_position.y));
    
    v_pos = gl_Position.xyz;
}
