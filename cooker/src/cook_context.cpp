#include "cook_context.hpp"

#include <iostream>

#include "euphrosyne/utils/hash/fnv1a.hpp"
#include "euphrosyne/utils/io/file.hpp"
#include "material_cooker.hpp"
#include "model_cooker.hpp"

namespace cooker
{
namespace
{
string ItemName(string_view name, string_view fallback, uintsize index)
{
    return name.empty() ? string(fallback) + to_string(index) : string(name);
}

string_view RoleName(TextureRole role)
{
    switch (role)
    {
    case TextureRole::BaseColor: return "basecolor";
    case TextureRole::MetallicRoughness: return "metalrough";
    case TextureRole::Normal: return "normal";
    case TextureRole::Occlusion: return "occlusion";
    case TextureRole::Emissive: return "emissive";
    default: return "unknown";
    }
}
} // namespace

string CookContext::Name(string_view kind, string_view item) const
{
    return source_ + "/" + string(kind) + "/" + string(item);
}

uint64 CookContext::Emit(string name, vector<uint8> data)
{
    auto hash = FNV1A64(name);
    writer_.Add(std::move(name), std::move(data));
    return hash;
}

void CookContext::Warn(string_view message) const
{
    cerr << "[cooker] " << source_ << ": " << message << '\n';
}

const ModelInfo &CookContext::RequireModel(uintsize mesh)
{
    if (auto it = models_.find(mesh); it != models_.end())
        return it->second;
    if (mesh >= gltf_.meshes.size())
        throw runtime_error("Mesh index out of range");

    auto cooked = CookModel(*this, mesh);
    auto info = ModelInfo();
    memcpy(info.aabb_min, cooked.aabb_min, sizeof(info.aabb_min));
    memcpy(info.aabb_max, cooked.aabb_max, sizeof(info.aabb_max));
    info.hash = Emit(Name("model", ItemName(gltf_.meshes[mesh].name, "mesh", mesh)), std::move(cooked.data));
    return models_.emplace(mesh, info).first->second;
}

uint64 CookContext::RequireMaterial(optional<uintsize> material)
{
    if (!material)
    {
        if (!default_material_)
            default_material_ = Emit(Name("material", "default"), CookDefaultMaterial());
        return *default_material_;
    }
    if (auto it = materials_.find(*material); it != materials_.end())
        return it->second;
    if (*material >= gltf_.materials.size())
        throw runtime_error("Material index out of range");

    const auto &m = gltf_.materials[*material];
    auto hash = Emit(Name("material", ItemName(m.name, "material", *material)), CookMaterial(*this, m));
    materials_.emplace(*material, hash);
    return hash;
}

const CookContext::ImageBytes &CookContext::LoadImage(uintsize index)
{
    if (auto it = images_.find(index); it != images_.end())
        return it->second;
    if (index >= gltf_.images.size())
        throw runtime_error("Image index out of range");

    auto bytes = make_shared<vector<uint8>>();
    auto copy = [&](const std::byte *data, uintsize size) {
        bytes->assign(reinterpret_cast<const uint8 *>(data), reinterpret_cast<const uint8 *>(data) + size);
    };

    // Image data can be a file, a slice of a (GLB/embedded) buffer, or already-decoded base64 bytes.
    std::visit(
        [&](const auto &src) {
            using T = decay_t<decltype(src)>;
            if constexpr (is_same_v<T, fastgltf::sources::URI>)
            {
                if (!src.uri->isLocalPath())
                    throw runtime_error("Remote image URIs are not supported");
                *bytes = ReadFile((directory_ / src.uri->fspath()).string());
            }
            else if constexpr (is_same_v<T, fastgltf::sources::BufferView>)
            {
                const auto &view = gltf_.bufferViews[src.bufferViewIndex];
                std::visit(
                    [&](const auto &buffer) {
                        if constexpr (requires { buffer.bytes.data(); buffer.bytes.size(); })
                        {
                            if (view.byteOffset + view.byteLength > buffer.bytes.size())
                                throw runtime_error("Image buffer view out of range");
                            copy(buffer.bytes.data() + view.byteOffset, view.byteLength);
                        }
                        else
                            throw runtime_error("Image buffer was not loaded");
                    },
                    gltf_.buffers[view.bufferIndex].data);
            }
            else if constexpr (requires { src.bytes.data(); src.bytes.size(); })
                copy(src.bytes.data(), src.bytes.size());
            else
                throw runtime_error("Unsupported image source");
        },
        gltf_.images[index].data);

    // Content hash: the same picture used by different files/materials becomes one texture asset.
    auto hash = FNV1A64(string_view(reinterpret_cast<const char *>(bytes->data()), bytes->size()));
    return images_.emplace(index, ImageBytes{hash, std::move(bytes)}).first->second;
}

uint64 CookContext::RequireTexture(uintsize texture, TextureRole role)
{
    if (texture >= gltf_.textures.size() || !gltf_.textures[texture].imageIndex)
        throw runtime_error("Texture has no usable image (unsupported texture extension?)");

    const auto &image = LoadImage(*gltf_.textures[texture].imageIndex);
    // Same image in two roles = two assets: the compression format depends on the role.
    auto name = TextureName(image.hash, role);
    auto hash = FNV1A64(name);
    if (texture_hashes_.insert(hash).second)
        textures_.push_back({.hash = hash, .name = std::move(name), .role = role, .encoded = image.bytes});
    return hash;
}

string TextureName(uint64 content_hash, TextureRole role)
{
    char hex[17];
    snprintf(hex, sizeof(hex), "%016llx", static_cast<unsigned long long>(content_hash));
    return "texture/" + string(hex) + "/" + string(RoleName(role));
}
} // namespace cooker