# version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_color;
layout (location = 2) in vec2 a_uv;

layout (location = 0) out vec4 v_color;
layout (location = 1) out vec2 v_uv;

layout(set = 1, binding = 0) uniform MatrixBlock {
    mat4 MVP;
} u_matrices;

void main() {
    gl_Position = u_matrices.MVP * vec4(a_position, 1.0);
    v_color = vec4(a_color, 1.0);
    v_uv = a_uv;
}
