#pragma once
#include "euphrosyne/formats/asset.hpp"

namespace euphrosyne
{
static constexpr uint32 kModelMagic = MakeTag("MODL");
static constexpr uint32 kModelVersion = 0;

enum ModelFlags : uint32
{
    kModelIndex32 = 1u << 0, // indices are uint32, otherwise uint16
};

/// 32 bytes. normal/tangent are octahedral encoded snorm16, bitangent = cross(n, t) * tangent_sign
/// (glTF convention). Plain arrays on purpose: hlslpp types are SIMD-aligned and must not be on disk.
struct ModelVertex
{
    float position[3];
    int16 normal[2];
    int16 tangent[2];
    int16 tangent_sign; // +-32767
    int16 reserved;     // room for packed color / flags
    float uv0[2];
};
static_assert(sizeof(ModelVertex) == 32);
static_assert(is_trivially_copyable_v<ModelVertex>);

struct SubMesh
{
    uint32 first_index;
    uint32 index_count;
    uint64 material; // hash of the material entry in the same archive
    float aabb_min[3];
    float aabb_max[3];
};
static_assert(sizeof(SubMesh) == 40);

struct ModelHeader
{
    AssetHeader header;
    uint32 flags;
    uint32 reserved;
    Range submeshes;
    Range vertices;
    Range indices; // count = number of indices, element size depends on flags
    float aabb_min[3];
    float aabb_max[3];
};
static_assert(sizeof(ModelHeader) == 72);

/// Validated, zero-copy view over a model asset. Copy the spans straight into GPU staging.
class ModelView
{
  public:
    static optional<ModelView> Parse(span<const uint8> asset)
    {
        if (!ValidateAssetHeader(asset, kModelMagic, kModelVersion, sizeof(ModelHeader)))
            return nullopt;

        auto view = ModelView();
        view.header_ = reinterpret_cast<const ModelHeader *>(asset.data());
        const auto &h = *view.header_;

        auto vertices = ResolveRange<ModelVertex>(asset, h.vertices);
        auto submeshes = ResolveRange<SubMesh>(asset, h.submeshes);
        uint64 index_size = (h.flags & kModelIndex32) ? 4 : 2;
        uint64 index_bytes = h.indices.count * index_size;
        if (!vertices || !submeshes || index_bytes > UINT32_MAX)
            return nullopt;
        auto indices = ResolveRange<uint8>(asset, Range{h.indices.offset, static_cast<uint32>(index_bytes)});
        if (!indices)
            return nullopt;

        for (const auto &sub : *submeshes)
            if (uint64(sub.first_index) + sub.index_count > h.indices.count)
                return nullopt;

        view.vertices_ = *vertices;
        view.submeshes_ = *submeshes;
        view.indices_ = *indices;
        return view;
    }

    const ModelHeader &Header() const { return *header_; }
    span<const ModelVertex> Vertices() const { return vertices_; }
    span<const SubMesh> SubMeshes() const { return submeshes_; }
    span<const uint8> IndexBytes() const { return indices_; }
    bool Is32BitIndex() const { return (header_->flags & kModelIndex32) != 0; }
    uint32 IndexCount() const { return header_->indices.count; }

  private:
    const ModelHeader *header_ = nullptr;
    span<const ModelVertex> vertices_;
    span<const SubMesh> submeshes_;
    span<const uint8> indices_;
};
} // namespace euphrosyne
