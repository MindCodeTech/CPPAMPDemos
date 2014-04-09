cbuffer ObjectInfo : register(b0)
{
	matrix WorldViewProjection;
	float4 PixelState;
}

struct VS_Input
{
    float3 pos     : POSITION0;
    float3 normal  : NORMAL0;
    float2 uv      : TEXCOORD0;
};

struct PS_Input
{
    float4 pos    : SV_POSITION;
    float2 uv     : TEXCOORD0;
};


Texture2D T : register(t0);
SamplerState Sampler : register(s0);
RWTexture2D<float4> RWT : register(u0);
