#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_color;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in float a_blockID;
layout (location = 4) in float a_light;

layout (location = 0) out vec4 v_color;
layout (location = 1) out vec2 v_uv;
layout (location = 2) flat out float v_blockID;
layout (location = 3) out vec3 v_pos;
layout (location = 4) out float v_light;

layout(set = 1, binding = 0, std140) uniform ProjectionBlock { mat4 projection; } projBlock;
layout(set = 1, binding = 1, std140) uniform ViewBlock { mat4 view; } viewBlock;

void main() {
    gl_Position = projBlock.projection * viewBlock.view * vec4(a_position, 1.0);
    v_color = vec4(a_color, 1.0);
    v_uv = a_uv;
    v_blockID = a_blockID;
    v_pos = gl_Position.xyz;
    v_light = a_light;
}
