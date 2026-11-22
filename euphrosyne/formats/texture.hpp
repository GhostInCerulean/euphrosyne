#pragma once
#include <array>
#include <cstring>
#include <optional>
#include <span>

#include "euphrosyne/pch.hpp"

namespace euphrosyne
{
/// Textures are plain KTX2 files stored as archive entries: BCn, no supercompression, so every mip
/// can be uploaded straight from the mapped pages. This parser is all the runtime needs, libktx is
/// only used by the cooker. Values are the VkFormat numbers KTX2 stores.
enum class TextureFormat : uint32
{
    BC4_UNORM = 139,
    BC5_UNORM = 141,
    BC7_UNORM = 145,
    BC7_SRGB = 146,
};

inline uint32 TextureBlockBytes(TextureFormat format)
{
    return format == TextureFormat::BC4_UNORM ? 8 : 16;
}

struct TextureLevel
{
    span<const uint8> data; // points into the mapped file
    uint32 width;
    uint32 height;
    uint32 row_pitch; // bytes per row of 4x4 blocks
};

class TextureView
{
  public:
    static constexpr uint32 kMaxLevels = 16;

    static optional<TextureView> Parse(span<const uint8> file)
    {
        static constexpr uint8 identifier[12] = {0xAB, 'K', 'T', 'X', ' ', '2', '0', 0xBB, '\r', '\n', 0x1A, '\n'};
        constexpr uintsize kHeaderSize = 12 + 9 * 4 + 4 * 4 + 2 * 8; // identifier + header + index
        constexpr uintsize kLevelIndexOffset = kHeaderSize;
        if (file.size() < kHeaderSize || memcmp(file.data(), identifier, sizeof(identifier)) != 0)
            return nullopt;

        auto u32 = [&](uintsize offset) {
            uint32 v;
            memcpy(&v, file.data() + offset, sizeof(v));
            return v;
        };
        auto u64 = [&](uintsize offset) {
            uint64 v;
            memcpy(&v, file.data() + offset, sizeof(v));
            return v;
        };

        auto vk_format = u32(12);
        auto width = u32(20), height = u32(24), depth = u32(28);
        auto layers = u32(32), faces = u32(36), level_count = u32(40), supercompression = u32(44);

        if (vk_format != 139 && vk_format != 141 && vk_format != 145 && vk_format != 146)
            return nullopt;
        if (width == 0 || height == 0 || depth > 1 || layers > 1 || faces != 1 || supercompression != 0)
            return nullopt;
        if (level_count == 0 || level_count > kMaxLevels)
            return nullopt;
        if (file.size() < kLevelIndexOffset + uintsize(level_count) * 24)
            return nullopt;

        auto view = TextureView();
        view.format_ = static_cast<TextureFormat>(vk_format);
        view.width_ = width;
        view.height_ = height;
        view.level_count_ = level_count;

        auto block_bytes = TextureBlockBytes(view.format_);
        for (uint32 i = 0; i < level_count; i++)
        {
            auto entry = kLevelIndexOffset + uintsize(i) * 24;
            auto offset = u64(entry), length = u64(entry + 8);
            auto w = max(1u, width >> i), h = max(1u, height >> i);
            auto blocks_x = (w + 3) / 4, blocks_y = (h + 3) / 4;
            if (offset > file.size() || length > file.size() - offset || length < uint64(blocks_x) * blocks_y * block_bytes)
                return nullopt;
            view.levels_[i] = TextureLevel{
                .data = file.subspan(offset, length),
                .width = w,
                .height = h,
                .row_pitch = blocks_x * block_bytes,
            };
        }
        return view;
    }

    TextureFormat Format() const { return format_; }
    uint32 Width() const { return width_; }
    uint32 Height() const { return height_; }
    uint32 LevelCount() const { return level_count_; }
    const TextureLevel &Level(uint32 i) const { return levels_[i]; }

  private:
    TextureFormat format_ = TextureFormat::BC7_UNORM;
    uint32 width_ = 0, height_ = 0, level_count_ = 0;
    array<TextureLevel, kMaxLevels> levels_ = {};
};
} // namespace euphrosyne