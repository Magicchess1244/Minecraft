//----------------------
// Pixel Shader
//----------------------

struct PSInput
{
    float4 pos : SV_POSITION; // output position (clip space)
    float3 col : COLOR; // pass color to pixel shader
};

float4 PSMain(PSInput input) : SV_TARGET
{
    return float4(input.col, 1.0); // output final pixel color
}