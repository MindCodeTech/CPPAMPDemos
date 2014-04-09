/*----------------------------------------------------------------------------
 * Copyright © Microsoft Corporation. All rights reserved.
 *---------------------------------------------------------------------------*/

struct VertexToPixel
{
    float4 Position     : SV_POSITION;
    float2 TexCoords    : TEXCOORD0;
};

VertexToPixel VS( float4 Pos : POSITION, float2 TexCoords : TEXCOORD0 )
{
    VertexToPixel o;
    o.Position = Pos;
    o.TexCoords = TexCoords;
    return o;
}
