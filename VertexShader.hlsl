//----------------------
// Vertex Shader
//----------------------
struct VSInput
{
	float3 pos : POSITION; // input vertex position
	float3 col : COLOR; // input vertex color
};

struct PSOutput
{
	float4 pos : SV_POSITION; // output position (clip space)
	float3 col : COLOR; // pass color to pixel shader
};

PSOutput VSMain(VSInput input)
{
	PSOutput output;
	output.pos = float4(input.pos, 1.0); // convert to 4D
	output.col = input.col; // pass color through
	return output;
}