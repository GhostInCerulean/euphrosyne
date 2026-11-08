#pragma once
#include "euphrosyne/pch.hpp"

namespace euphrosyne
{
constexpr uint32 MakeTag(const char (&s)[5])
{
    return (uint32_t)(unsigned char)s[0] | ((uint32_t)(unsigned char)s[1] << 8) |
           ((uint32_t)(unsigned char)s[2] << 16) | ((uint32_t)(unsigned char)s[3] << 24);
}

constexpr uint64 MakeTag(const char (&s)[9])
{
    uint64_t v = 0;
    for (int i = 0; i < 8; i++)
        v |= (uint64_t)(unsigned char)s[i] << (8 * i);
    return v;
}
} // namespace euphrosyne