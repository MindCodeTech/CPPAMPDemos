cbuffer ObjectInfo : register(b0)
{
	matrix world;
	matrix view;
	matrix projection;
	float3 eye;
	float  time;
	float  TessEdge;
	float  TessInside;
	float2 ONOP;
}
