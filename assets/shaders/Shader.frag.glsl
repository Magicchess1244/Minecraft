#version 460

layout (location = 0) in vec4 v_color;
layout (location = 1) in vec2 v_uv;

layout (location = 0) out vec4 FragColor;

layout(set = 2, binding = 0) uniform sampler2D texSampler;

void main()
{
    FragColor = texture(texSampler, v_uv) * v_color;
}