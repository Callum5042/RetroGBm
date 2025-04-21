#include "ShaderData.hlsli"

// Entry point for the vertex shader
VertexOutput main(VertexInput input)
{
	VertexOutput output;

	// Pass the vertex data to pixel shader
    output.position = mul(float4(input.position, 1.0f), cWorld);
    //output.position = mul(output.position, cCameraView);
    output.position = mul(output.position, cCameraProjection);
	
	// Set texture
	output.tex = input.tex;

	return output;
}