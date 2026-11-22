#include "material_cooker.hpp"

namespace cooker
{
namespace
{
MaterialData DefaultMaterialData()
{
    auto data = MaterialData();
    memset(&data, 0, sizeof(data));
    data.header = {.magic = kMaterialMagic, .version = kMaterialVersion, .size = sizeof(MaterialData), .reserved = 0};
    for (auto &c : data.base_color)
        c = 1.0f;
    data.metallic = 1.0f;
    data.roughness = 1.0f;
    data.normal_scale = 1.0f;
    data.occlusion_strength = 1.0f;
    data.alpha_cutoff = 0.5f;
    for (auto &slot : data.textures)
        slot.uv_scale[0] = slot.uv_scale[1] = 1.0f;
    return data;
}

vector<uint8> Serialize(const MaterialData &data)
{
    auto bytes = vector<uint8>(sizeof(MaterialData));
    memcpy(bytes.data(), &data, sizeof(MaterialData));
    return bytes;
}

SamplerWrap ConvertWrap(fastgltf::Wrap wrap)
{
    switch (wrap)
    {
    case fastgltf::Wrap::ClampToEdge: return SamplerWrap::Clamp;
    case fastgltf::Wrap::MirroredRepeat: return SamplerWrap::Mirror;
    default: return SamplerWrap::Repeat;
    }
}

// glTF min filters collapse to three states; Nearest/Linear-mip-Linear is approximated as trilinear.
SamplerFilter ConvertFilter(fastgltf::Filter filter)
{
    switch (filter)
    {
    case fastgltf::Filter::Nearest:
    case fastgltf::Filter::NearestMipMapNearest: return SamplerFilter::Nearest;
    case fastgltf::Filter::Linear:
    case fastgltf::Filter::LinearMipMapNearest: return SamplerFilter::Linear;
    default: return SamplerFilter::Trilinear;
    }
}

TextureSlot MakeSlot(CookContext &ctx, const fastgltf::TextureInfo &info, TextureRole role)
{
    auto slot = TextureSlot();
    memset(&slot, 0, sizeof(slot));
    slot.texture = ctx.RequireTexture(info.textureIndex, role);
    slot.uv_set = static_cast<uint8>(info.texCoordIndex);
    slot.uv_scale[0] = slot.uv_scale[1] = 1.0f;
    slot.wrap_u = slot.wrap_v = SamplerWrap::Repeat;
    slot.filter = SamplerFilter::Trilinear;

    if (info.transform)
    {
        const auto &t = *info.transform;
        slot.uv_offset[0] = t.uvOffset[0];
        slot.uv_offset[1] = t.uvOffset[1];
        slot.uv_scale[0] = t.uvScale[0];
        slot.uv_scale[1] = t.uvScale[1];
        slot.uv_rotation = t.rotation;
        if (t.texCoordIndex)
            slot.uv_set = static_cast<uint8>(*t.texCoordIndex);
    }

    const auto &texture = ctx.Gltf().textures[info.textureIndex];
    if (texture.samplerIndex)
    {
        const auto &sampler = ctx.Gltf().samplers[*texture.samplerIndex];
        slot.wrap_u = ConvertWrap(sampler.wrapS);
        slot.wrap_v = ConvertWrap(sampler.wrapT);
        if (sampler.minFilter)
            slot.filter = ConvertFilter(*sampler.minFilter);
    }

    if (slot.uv_set != 0)
        ctx.Warn("material uses UV set " + to_string(slot.uv_set) + ", models only carry uv0");
    return slot;
}
} // namespace

vector<uint8> CookDefaultMaterial()
{
    return Serialize(DefaultMaterialData());
}

vector<uint8> CookMaterial(CookContext &ctx, const fastgltf::Material &m)
{
    auto data = DefaultMaterialData();
    for (uintsize i = 0; i < 4; i++)
        data.base_color[i] = m.pbrData.baseColorFactor[i];
    for (uintsize i = 0; i < 3; i++)
        data.emissive[i] = m.emissiveFactor[i] * m.emissiveStrength;
    data.metallic = m.pbrData.metallicFactor;
    data.roughness = m.pbrData.roughnessFactor;
    data.alpha_cutoff = m.alphaCutoff;
    if (m.doubleSided)
        data.flags |= kMaterialDoubleSided;

    switch (m.alphaMode)
    {
    case fastgltf::AlphaMode::Mask: data.alpha_mode = MaterialAlphaMode::Mask; break;
    case fastgltf::AlphaMode::Blend: data.alpha_mode = MaterialAlphaMode::Blend; break;
    default: data.alpha_mode = MaterialAlphaMode::Opaque; break;
    }

    auto slot = [&](TextureRole role) -> TextureSlot & { return data.textures[static_cast<uintsize>(role)]; };
    if (m.pbrData.baseColorTexture)
        slot(TextureRole::BaseColor) = MakeSlot(ctx, *m.pbrData.baseColorTexture, TextureRole::BaseColor);
    if (m.pbrData.metallicRoughnessTexture)
        slot(TextureRole::MetallicRoughness) =
            MakeSlot(ctx, *m.pbrData.metallicRoughnessTexture, TextureRole::MetallicRoughness);
    if (m.normalTexture)
    {
        slot(TextureRole::Normal) = MakeSlot(ctx, *m.normalTexture, TextureRole::Normal);
        data.normal_scale = m.normalTexture->scale;
    }
    if (m.occlusionTexture)
    {
        slot(TextureRole::Occlusion) = MakeSlot(ctx, *m.occlusionTexture, TextureRole::Occlusion);
        data.occlusion_strength = m.occlusionTexture->strength;
    }
    if (m.emissiveTexture)
        slot(TextureRole::Emissive) = MakeSlot(ctx, *m.emissiveTexture, TextureRole::Emissive);

    return Serialize(data);
}
} // namespace cooker
