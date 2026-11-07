#include "common.hlsli"

VSOut mainVS(VSIn i)
{
    VSOut o;
    o.pos         = mul(float4(i.pos, 1), worldViewProj);
    o.worldNormal = mul(i.normal, (float3x3)world); 
    return o;
}