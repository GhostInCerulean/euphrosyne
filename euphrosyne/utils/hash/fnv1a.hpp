#pragma once
#include <string_view>

#include "euphrosyne/pch.hpp"

namespace euphrosyne
{
constexpr uint64 FNV1A64(const string_view str)
{
    uint64_t h = 1469598103934665603ull;
    for (const unsigned char c : str)
    {
        h ^= c;
        h *= 1099511628211ull;
    }
    return h;
}
} // namespace euphrosyne
