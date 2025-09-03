//----------------------
// Vertex Shader
//----------------------
#define REG(reg) register(reg)

cbuffer UBO : REG(b0)
{
    float4x4 ModelViewProj;
};

struct VSInput
{
    float3 pos : TEXCOORD0; // input vertex position
    float3 col : TEXCOORD1; // input vertex color
};

struct PSOutput
{
	float4 pos : SV_POSITION; // output position (clip space)
	float3 col : COLOR; // pass color to pixel shader
};

PSOutput VSMain(VSInput input)
{
	PSOutput output;
    output.pos = mul(ModelViewProj, float4(input.pos, 1.0)); // convert to 4D
	output.col = input.col; // pass color through
	return output;
}