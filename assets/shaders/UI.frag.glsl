#version 460

layout (location = 0) in vec4 v_color;
layout (location = 1) in vec2 v_uv;
layout (location = 2) flat in float v_blockID;
layout (set = 2, binding = 0) uniform sampler2D texSampler;
layout (set = 2, binding = 1) uniform sampler2D u_blockProps;

layout (location = 0) out vec4 FragColor;

int BlockInAtlasRow = 32;

vec4 sampleAtlas(vec2 atlasUV) {
    return texture(texSampler, atlasUV);
}

void main() {
    if (v_blockID > 0.5) {
        int blockIdx = int(v_blockID + 0.5);
        // Texel 2 contains Textures[4] and Textures[5]
        vec4 t2 = texelFetch(u_blockProps, ivec2(2, blockIdx), 0);
        uint tileIndex = uint(t2.r * 255.0 + 0.5) | (uint(t2.g * 255.0 + 0.5) << 8);
        
        vec2 atlasUV = (vec2(tileIndex % BlockInAtlasRow, tileIndex / BlockInAtlasRow) + fract(v_uv)) / float(BlockInAtlasRow);
        FragColor = v_color * sampleAtlas(atlasUV);
        //FragColor.a = 1.0;
    } else if (v_blockID <= -0.5) {
        FragColor = vec4(0.0, 0.0, 0.0, 0.7);
    } else {
        FragColor = v_color;
    }
}
