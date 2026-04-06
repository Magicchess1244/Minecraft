#version 460
layout (location = 0) in vec3 a_position; // block world origin
layout (location = 1) in float a_data_float;

layout (location = 0) out float v_color;
layout (location = 1) out vec2  v_uv;
layout (location = 2) flat out float v_blockID;
layout (location = 3) out vec3  v_pos;
layout (location = 4) out float v_light;

layout(set=1,binding=0,std140) uniform ProjectionBlock { mat4 projection; } projBlock;
layout(set=1,binding=1,std140) uniform ViewBlock       { mat4 view;       } viewBlock;

void main() {
    int corner = gl_VertexIndex; // 0,1,2,3 from index buffer {0,1,2,1,3,2}
    float cx = float(corner & 1);
    float cy = float((corner >> 1) & 1);

    uint  a_data  = floatBitsToUint(a_data_float);
    uint  side    = a_data & 0x7u;

    vec3 origin = floor(a_position + 0.5);
    vec3 offset = vec3(0.0);

    if (side == 0u) { // Front (+Z)
        origin += vec3(0.0, 0.0, 1.0);
        offset = vec3(cx, cy, 0.0);
    } else if (side == 1u) { // Back (-Z)
        origin += vec3(1.0, 0.0, 0.0);
        offset = vec3(-cx, cy, 0.0);
    } else if (side == 2u) { // Right (+X)
        origin += vec3(1.0, 0.0, 1.0);
        offset = vec3(0.0, cy, -cx);
    } else if (side == 3u) { // Left (-X)
        origin += vec3(0.0, 0.0, 0.0);
        offset = vec3(0.0, cy, cx);
    } else if (side == 4u) { // Top (+Y)
        origin += vec3(0.0, 1.0, 1.0);
        offset = vec3(cx, 0.0, -cy);
    } else if (side == 5u) { // Bottom (-Y)
        origin += vec3(0.0, 0.0, 0.0);
        offset = vec3(cx, 0.0, cy);
    }

    vec3 realPos  = origin + offset;
    gl_Position   = projBlock.projection * viewBlock.view * vec4(realPos, 1.0);

    v_color   = float(side);
    v_blockID = float((a_data >> 3u)  & 0xFFFFu);
    v_light   = float((a_data >> 19u) & 0xFu);
    v_uv      = vec2(cx, 1.0 - cy);
    v_pos     = gl_Position.xyz;
}