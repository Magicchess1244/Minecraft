#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4 ModelViewProj;
} ubo;

in layout(location = 0) vec3 inPosition;
in layout(location = 1) vec3 inColor;

out layout(location = 0) vec4 fragColor;

void main() {
    gl_Position = ubo.ModelViewProj * vec4(inPosition, 1.0);
    fragColor = vec4(inColor, 1);
}
