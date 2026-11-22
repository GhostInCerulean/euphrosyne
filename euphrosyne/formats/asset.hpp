#pragma once
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <type_traits>

#include "euphrosyne/pch.hpp"
#include "euphrosyne/utils/bytes/tag.hpp"

namespace euphrosyne
{
/// Every cooked asset (model, material, level, ...) starts with an AssetHeader and every section
/// inside it is 16-byte aligned *relative to the asset start*. Archive entries are 16-byte aligned
/// and the mapping is page aligned, so pointers into an asset are naturally aligned.
/// Assets contain no pointers: sections are referenced by Range (byte offset + element count).
static constexpr uint32 kAssetAlignment = 16;

struct AssetHeader
{
    uint32 magic;
    uint32 version;
    uint32 size; // total asset size in bytes, must match the archive entry size
    uint32 reserved;
};
static_assert(sizeof(AssetHeader) == 16);

struct Range
{
    uint32 offset; // bytes from the start of the asset
    uint32 count;  // number of elements
};
static_assert(sizeof(Range) == 8);

inline bool ValidateAssetHeader(span<const uint8> asset, uint32 magic, uint32 version, uintsize header_size)
{
    if (asset.size() < header_size || reinterpret_cast<uintptr_t>(asset.data()) % kAssetAlignment != 0)
        return false;
    auto header = AssetHeader();
    memcpy(&header, asset.data(), sizeof(header));
    return header.magic == magic && header.version == version && header.size == asset.size();
}

/// Bounds + alignment checked view of a section. Use this for every Range you read.
template <class T> optional<span<const T>> ResolveRange(span<const uint8> asset, Range range)
{
    static_assert(is_trivially_copyable_v<T>);
    if (range.count == 0)
        return span<const T>();
    if (range.offset > asset.size() || range.count > (asset.size() - range.offset) / sizeof(T))
        return nullopt;
    auto *ptr = asset.data() + range.offset;
    if (reinterpret_cast<uintptr_t>(ptr) % alignof(T) != 0)
        return nullopt;
    return span<const T>(reinterpret_cast<const T *>(ptr), range.count);
}
} // namespace euphrosyne
