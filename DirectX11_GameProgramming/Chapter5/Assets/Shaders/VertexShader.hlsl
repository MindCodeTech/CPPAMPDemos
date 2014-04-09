#include "Parameters.hlsli"

PS_Input main( VS_Input IN )
{
	PS_Input OUT = (PS_Input)0;

	OUT.pos  = mul(float4(IN.pos, 1.0f), WorldViewProjection);
	OUT.uv = IN.uv;

	return OUT;
}
