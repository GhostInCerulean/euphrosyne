#include "common.hlsli"

Texture2D    albedoTex  : register(t0);
SamplerState linearSamp : register(s0);

float4 mainPS(VSOut i) : SV_Target
{
    float3 albedo = albedoTex.Sample(linearSamp, i.uv).rgb;
    float3 n      = normalize(i.worldNormal);
    float  ndl    = saturate(dot(n, normalize(lightDir.xyz)));
    float3 color  = albedo * (ndl + 0.1);
    return float4(color, 1);
}