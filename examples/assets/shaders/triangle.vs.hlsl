struct VSOutput {
    float4 position : SV_Position;
    float3 color    : COLOR0;
};

VSOutput main(uint vertexId : SV_VertexID) {
    static const float2 pos[3] = {
        float2( 0.0, -0.6),
        float2( 0.6,  0.6),
        float2(-0.6,  0.6)
    };
    static const float3 col[3] = {
        float3(1, 0, 0),
        float3(0, 1, 0),
        float3(0, 0, 1)
    };

    VSOutput o;
    o.position = float4(pos[vertexId], 0.0, 1.0);
    o.color    = col[vertexId];
    return o;
}