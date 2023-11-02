#include "ShaderData.hlsli"

// Entry point for the vertex shader
float4 main(VertexOutput input) : SV_TARGET
{
	float4 diffuse_texture = gTextureDiffuse.Sample(gSampler, input.tex);
	return diffuse_texture;
}