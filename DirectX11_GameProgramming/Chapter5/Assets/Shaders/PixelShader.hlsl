#include "Parameters.hlsli"

float4 main(PS_Input IN) : SV_TARGET
{
	return T.Sample(Sampler, IN.uv);
}