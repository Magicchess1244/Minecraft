#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_color;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in float a_blockID;

layout (location = 0) out vec4 v_color;
layout (location = 1) out vec2 v_uv;
layout (location = 2) flat out float v_blockID;

void main() {
    gl_Position = vec4(a_position, 1.0);
    v_color = vec4(a_color, 1.0);
    v_uv = a_uv;
    v_blockID = a_blockID;
}
