#include "Default.hlsli"

VS_Out main( VS_Input IN )
{
	VS_Out OUT = (VS_Out)0;

	float4 modelWorld = mul(float4(IN.pos,1), world);
    float4 WorldView = mul(modelWorld, view);
    OUT.pos = mul(WorldView, projection);
	OUT.eye = WorldView * OUT.pos;	

	float3 Binormal = normalize(cross(IN.tangent ,IN.normal));

	float3x3 tangentToObject;
	tangentToObject[0] = normalize(Binormal);
	tangentToObject[1] = normalize(IN.tangent);
	tangentToObject[2] = normalize(IN.normal);
	//tangentToWorld
	OUT.BTN = mul(tangentToObject, (float3x3)world);

	//Flip UV
	OUT.uv = float2(IN.uv.x, -IN.uv.y);

	return OUT;
}
