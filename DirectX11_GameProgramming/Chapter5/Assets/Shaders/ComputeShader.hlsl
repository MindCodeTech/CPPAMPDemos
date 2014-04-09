#include "Parameters.hlsli"

#define N 256

[numthreads(N, 1, 1)]
void main(uint3 DTid  : SV_DispatchThreadID)
{
	float4 color = T[DTid .xy];
	if (PixelState.x == 1)
	{
		//Negative
		color = 1 - color;
	}
	if (PixelState.y == 1)
	{
		//High Lighter
		static const float3 weights = float3(0.05f, 0.1f, 0.2f);
		for(int i = 0; i < 3; i++) 
		{
			color.rgb += color.rgb * weights[i];
		}
	}
	if (PixelState.z == 1)
	{
		//Odd/Even
		const float _CONST0 = 0.5f;
		const float _CONST1 = 10.0f;

		if ( DTid.x % 2 != 0)//if is Odd
		{
			color.rgb += float3(sin(color.r * _CONST1) * _CONST0, sin(color.g * _CONST1) * _CONST0, sin(color.b * _CONST1) * _CONST0);
		}
		else
		{
			color.rgb += float3(cos(color.r * _CONST1) * _CONST0, cos(color.g * _CONST1) * _CONST0, cos(color.b * _CONST1) * _CONST0);
		}
	}
	
	RWT[DTid.xy] = color;
}
