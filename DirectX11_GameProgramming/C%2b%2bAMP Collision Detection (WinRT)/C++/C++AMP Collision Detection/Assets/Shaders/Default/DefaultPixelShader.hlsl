#include "Default.hlsli"

Texture2D T : register(t0);
SamplerState ST : register(s0);

Texture2D N : register(t1);
SamplerState SN : register(s1);

Texture2D S : register(t2);
SamplerState SS : register(s2);

float3 Phong( float4 specColor, float4 eye, float3 normal)
{
	float3 sunVec = normalize(eye.xyz - LPos);
	float3 viewVec = normalize(-eye.xyz);
	float3 refVec = reflect( normal , -sunVec );

	//(Light ambient) * (Material ambient)
	float3 ambient = La * Ka;

	//(Light ambient) * (Material ambient) * (Sun vector)dot(Normal)
	float sDotN = saturate( dot(sunVec, normal) );
	float3 diffuse = Ld * Kd * sDotN;

	float3 spec = float3(0,0,0);
	if( sDotN > 0.0 )
	{
		//(Light specular) * (Material specular) * power ( maximum of (reflect)dot(view) and 0.0f ) to SpecPower
		spec = Ls * Ks * pow( max( dot(refVec,viewVec), 0.0 ), SpecPower );
	}

	return ambient + diffuse + spec * specColor.rgb;
}

float4 main(VS_Out IN) : SV_TARGET
{
	float4 texColor = T.Sample(ST, IN.uv);
	float4 bumpColor = N.Sample( SN, IN.uv );
	float4 specColor = S.Sample( SS, IN.uv );
	
	float3 normalRange = (bumpColor - 0.5f) * 2.0f;
	float3 normalWorld = mul(normalRange, IN.BTN);
    float LFactor = dot(-normalize(normalWorld), normalize(LPos));

	float3 phongColor = Phong(specColor, IN.eye, IN.BTN[2]);
	float4 color = BaseColor + texColor + float4(LFactor * phongColor , texColor.a);

	if(color.a == 0) discard;

	return color;
}