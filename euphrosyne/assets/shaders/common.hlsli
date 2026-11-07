#ifndef COMMON_HLSLI
#define COMMON_HLSLI

cbuffer PerObject : register(b0)
{
    float4x4 worldViewProj;
    float4x4 world;
    float4   lightDir;
};

struct VSIn
{
    float3 pos    : POSITION;
    float3 normal : NORMAL;
    float2 uv     : TEXCOORD0;
};

struct VSOut
{
    float4 pos         : SV_Position;
    float3 worldNormal : NORMAL;
    float2 uv          : TEXCOORD0;
};

#endif