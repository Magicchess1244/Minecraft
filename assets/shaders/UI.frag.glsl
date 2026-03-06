#version 460

layout (location = 0) in vec4 v_color;
layout (location = 1) in vec2 v_uv;
layout (location = 2) flat in float v_blockID;

layout (set = 2, binding = 0) uniform sampler2D texSampler;

layout (location = 0) out vec4 FragColor;

int BlockInAtlasRow = 32;

void main() {
    if (v_blockID > 0.5) {
        int blockID = int(v_blockID + 0.5);
        vec2 atlasUV = (vec2(blockID % BlockInAtlasRow, blockID / BlockInAtlasRow) + fract(v_uv)) / BlockInAtlasRow;
        FragColor = v_color * texture(texSampler, atlasUV);
        //FragColor.a = 1.0;
    } else if (v_blockID <= -0.5) {
        FragColor = vec4(0.0, 0.0, 0.0, 0.7);
    } else {
        FragColor = v_color;
    }
}
