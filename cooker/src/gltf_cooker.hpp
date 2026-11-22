#pragma once
#include "cook_context.hpp"
#include "level_cooker.hpp"

namespace cooker
{
struct CookOptions
{
    bool cook_level = true;        // cook the scene into a Level (pulls in its models + materials)
    bool cook_all_models = false;  // also cook every mesh (prop libraries)
    optional<uintsize> scene;      // default: glTF default scene, else scene 0
    const LevelCooker *level_cooker = nullptr; // custom hooks; null = built-in behavior only
};

struct CookResult
{
    uint64 level = 0;                  // 0 if no level was cooked
    vector<TextureRequest> textures;   // hand these to the texture cooking pass
};

/// Entry names are "<file stem>/<kind>/<name>"; cooking two files with the same stem into one
/// ArchiveWriter will trip the writer's duplicate check.
CookResult CookGltf(const filesystem::path &file, ArchiveWriter &writer, const CookOptions &options = {});
} // namespace cooker
