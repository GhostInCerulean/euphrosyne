#pragma once
#include "euphrosyne/formats/asset.hpp"

namespace euphrosyne
{
static constexpr uint32 kMaterialMagic = MakeTag("MATL");
static constexpr uint32 kMaterialVersion = 0;

/// Doubles as the texture role: the texture cooker picks format/colorspace from it
/// (BaseColor/Emissive -> BC7 sRGB, Normal -> BC5, MetallicRoughness/Occlusion -> linear).
enum class TextureRole : uint32
{
    BaseColor,
    MetallicRoughness,
    Normal,
    Occlusion,
    Emissive,
    Count,
};

enum class MaterialAlphaMode : uint32
{
    Opaque,
    Mask,
    Blend,
};

enum class SamplerWrap : uint8
{
    Repeat,
    Mirror,
    Clamp,
};

enum class SamplerFilter : uint8
{
    Nearest,
    Linear,
    Trilinear,
};

enum MaterialFlags : uint32
{
    kMaterialDoubleSided = 1u << 0,
};

struct TextureSlot
{
    uint64 texture; // hash of the KTX2 entry, 0 = no texture
    uint8 uv_set;
    SamplerWrap wrap_u;
    SamplerWrap wrap_v;
    SamplerFilter filter;
    float uv_offset[2]; // KHR_texture_transform
    float uv_scale[2];
    float uv_rotation;
};
static_assert(sizeof(TextureSlot) == 32);

/// Fixed size, no sections. New features go into `reserved` first, then a version bump.
struct MaterialData
{
    AssetHeader header;
    float base_color[4];
    float emissive[3]; // already multiplied by emissive_strength
    float metallic;
    float roughness;
    float normal_scale;
    float occlusion_strength;
    float alpha_cutoff;
    MaterialAlphaMode alpha_mode;
    uint32 flags;
    uint32 reserved[2];
    TextureSlot textures[static_cast<uintsize>(TextureRole::Count)];
};
static_assert(is_trivially_copyable_v<MaterialData>);
static_assert(sizeof(MaterialData) == 240);

inline const MaterialData *ParseMaterial(span<const uint8> asset)
{
    if (!ValidateAssetHeader(asset, kMaterialMagic, kMaterialVersion, sizeof(MaterialData)))
        return nullptr;
    return reinterpret_cast<const MaterialData *>(asset.data());
}
} // namespace euphrosyne
