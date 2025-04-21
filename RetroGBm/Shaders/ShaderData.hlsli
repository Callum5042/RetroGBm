// Vertex input
struct VertexInput
{
    float3 position : POSITION;
    float2 tex : TEXTURE;
};

// Vertex output / pixel input structure
struct VertexOutput
{
    float4 position : SV_POSITION;
    float2 tex : TEXTURE;
};

// World constant buffer
cbuffer WorldBuffer : register(b0)
{
    matrix cWorld;
    matrix cCameraProjection;
}

// Texture sampler
SamplerState gSampler : register(s0);

// Textures
Texture2D gTextureDiffuse : register(t0);