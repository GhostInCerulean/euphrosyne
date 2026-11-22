#pragma once
#include <algorithm>
#include <cmath>

#include "euphrosyne/pch.hpp"

namespace cooker
{
inline int16 PackSnorm16(float v)
{
    return static_cast<int16>(std::lround(std::clamp(v, -1.0f, 1.0f) * 32767.0f));
}

/// Octahedral encoding of a unit vector into two snorm16 (decode: see model_nvrhi.hpp).
inline void OctEncode(float x, float y, float z, int16 out[2])
{
    auto l1 = std::fabs(x) + std::fabs(y) + std::fabs(z);
    auto inv = l1 > 0.0f ? 1.0f / l1 : 0.0f;
    x *= inv;
    y *= inv;
    z *= inv;
    auto ox = x, oy = y;
    if (z < 0.0f)
    {
        ox = (1.0f - std::fabs(y)) * (x >= 0.0f ? 1.0f : -1.0f);
        oy = (1.0f - std::fabs(x)) * (y >= 0.0f ? 1.0f : -1.0f);
    }
    out[0] = PackSnorm16(ox);
    out[1] = PackSnorm16(oy);
}
} // namespace cooker
