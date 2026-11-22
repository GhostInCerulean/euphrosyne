#pragma once
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <unordered_map>
#include <unordered_set>

#include "archive/archive_writer.hpp"
#include "euphrosyne/formats/material.hpp"

namespace cooker
{
using namespace euphrosyne;

/// A texture the cooked materials refer to. CookContext only records these; TextureCooker (decode,
/// mips, BCn, KTX2) consumes them and emits the entry under `hash`.
struct TextureRequest
{
    uint64 hash; // FNV1A64(name), what materials store
    string name; // "texture/<content hash>/<role>", independent of the glTF file so it dedups across files
    TextureRole role;
    shared_ptr<const vector<uint8>> encoded; // PNG/JPEG/... file bytes
};

string TextureName(uint64 content_hash, TextureRole role);

struct ModelInfo
{
    uint64 hash = 0;
    float aabb_min[3] = {};
    float aabb_max[3] = {};
};

/// Shared state for cooking one glTF file. Models, materials and the level never cook each other
/// directly: they ask the context (Require*), which cooks on first use, caches, and emits into the
/// archive. That is what makes a standalone prop and a whole level go through the same code.
class CookContext
{
  public:
    /// `directory` is where relative image URIs are resolved (the glTF's folder).
    CookContext(fastgltf::Asset &gltf, filesystem::path directory, string source, ArchiveWriter &writer)
        : gltf_(gltf), directory_(std::move(directory)), source_(std::move(source)), writer_(writer)
    {
    }

    fastgltf::Asset &Gltf() { return gltf_; }
    const string &Source() const { return source_; }

    /// "<source>/<kind>/<item>", the string whose FNV1A64 is the archive hash.
    string Name(string_view kind, string_view item) const;
    uint64 Emit(string name, vector<uint8> data);

    const ModelInfo &RequireModel(uintsize mesh);
    uint64 RequireMaterial(optional<uintsize> material); // nullopt = default material
    uint64 RequireTexture(uintsize texture, TextureRole role);

    const vector<TextureRequest> &Textures() const { return textures_; }
    void Warn(string_view message) const;

  private:
    struct ImageBytes
    {
        uint64 hash;
        shared_ptr<const vector<uint8>> bytes;
    };
    const ImageBytes &LoadImage(uintsize image);

    fastgltf::Asset &gltf_;
    filesystem::path directory_;
    string source_;
    ArchiveWriter &writer_;
    unordered_map<uintsize, ModelInfo> models_;
    unordered_map<uintsize, uint64> materials_;
    optional<uint64> default_material_;
    unordered_map<uintsize, ImageBytes> images_;
    unordered_set<uint64> texture_hashes_;
    vector<TextureRequest> textures_;
};
} // namespace cooker