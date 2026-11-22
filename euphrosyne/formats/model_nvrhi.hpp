#pragma once
#include <array>
#include <cstddef>
#include <nvrhi/nvrhi.h>

#include "euphrosyne/formats/model.hpp"

namespace euphrosyne
{
/// Attributes for nvrhi::Device::createInputLayout(). Index buffer format: R16_UINT or R32_UINT
/// depending on ModelView::Is32BitIndex().
/// HLSL decode:
///   float3 OctDecode(float2 e) {
///       float3 n = float3(e.x, e.y, 1.0 - abs(e.x) - abs(e.y));
///       if (n.z < 0) n.xy = (1.0 - abs(n.yx)) * float2(n.x >= 0 ? 1 : -1, n.y >= 0 ? 1 : -1);
///       return normalize(n);
///   }
inline array<nvrhi::VertexAttributeDesc, 5> ModelVertexAttributes()
{
    auto make = [](const char *name, nvrhi::Format format, size_t offset) {
        return nvrhi::VertexAttributeDesc()
            .setName(name)
            .setFormat(format)
            .setOffset(static_cast<uint32_t>(offset))
            .setElementStride(sizeof(ModelVertex));
    };
    return {
        make("POSITION", nvrhi::Format::RGB32_FLOAT, offsetof(ModelVertex, position)),
        make("NORMAL", nvrhi::Format::RG16_SNORM, offsetof(ModelVertex, normal)),
        make("TANGENT", nvrhi::Format::RG16_SNORM, offsetof(ModelVertex, tangent)),
        make("TANGENT_SIGN", nvrhi::Format::R16_SNORM, offsetof(ModelVertex, tangent_sign)),
        make("TEXCOORD", nvrhi::Format::RG32_FLOAT, offsetof(ModelVertex, uv0)),
    };
}
} // namespace euphrosyne
