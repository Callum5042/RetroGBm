#include "ShaderData.hlsli"

// Entry point for the vertex shader
VertexOutput main(VertexInput input)
{
	VertexOutput output;

	// Pass the vertex data to pixel shader
	output.position = float4(input.position, 1.0f);
	output.tex = input.tex;

	return output;
}