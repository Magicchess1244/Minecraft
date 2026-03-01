#version 460

layout (location = 0) in vec4 v_color;
layout (location = 1) in vec2 v_uv;

layout (set = 2, binding = 0) uniform sampler2D texSampler;

layout (location = 0) out vec4 FragColor;

void main() {
    float alpha = texture(texSampler, v_uv).a;
    
    // Sample neighbors for a 1px outline
    vec2 texSize = textureSize(texSampler, 0);
    vec2 step = 1.0 / texSize; 
    
    float outline = 0.0;
    outline = max(outline, texture(texSampler, v_uv + vec2(step.x, 0.0)).a);
    outline = max(outline, texture(texSampler, v_uv - vec2(step.x, 0.0)).a);
    outline = max(outline, texture(texSampler, v_uv + vec2(0.0, step.y)).a);
    outline = max(outline, texture(texSampler, v_uv - vec2(0.0, step.y)).a);
    
    vec4 textColor = vec4(v_color.rgb, v_color.a * alpha);
    vec4 outlineColor = vec4(0.0, 0.0, 0.0, v_color.a * outline);
    
    // Mix based on center alpha - if center is empty but neighbor has content, show outline
    FragColor = mix(outlineColor, textColor, alpha);
    
    if (FragColor.a < 0.1) discard;
}
