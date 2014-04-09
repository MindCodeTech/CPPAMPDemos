/*----------------------------------------------------------------------------
 * Copyright © Microsoft Corporation. All rights reserved.
 *---------------------------------------------------------------------------*/

 Texture2D tex0 : register( t0 );

struct VertexToPixel
{
    float4 Position     : SV_POSITION;
    float2 TexCoords    : TEXCOORD0;
};

float4 PS( VertexToPixel input ) : SV_Target
{
    int3 loc = int3(input.TexCoords.x, input.TexCoords.y, 0);
    float v = tex0.Load(loc);
    float3 c = float3(v*v,v,sqrt(v));
    return float4(saturate(c),1);
}
