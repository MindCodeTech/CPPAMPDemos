#include "..\Shared.hlsli"

cbuffer LightInfo : register(b1)
{
	float4 LPos;
	float3 La;
	float3 Ld;
	float3 Ls;	
	float3 Ka;
	float3 Kd;
	float3 Ks;
	float SpecPower;
	float NOP;
}

cbuffer PixelColor : register(b2)
{
	float4 BaseColor;
}

struct VS_Input
{
    float3 pos     : POSITION0;
    float3 normal  : NORMAL0;
    float4 tangent : TANGENT0;
    float4 color   : COLOR0;
    float2 uv      : TEXCOORD0;
};

struct VS_Out
{
    float4 pos    : SV_POSITION;
    float2 uv     : TEXCOORD0;
	float4 eye    : TEXCOORD1;
	float3x3 BTN  : TEXCOORD2;
};