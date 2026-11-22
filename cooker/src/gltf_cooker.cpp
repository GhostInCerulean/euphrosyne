#include "gltf_cooker.hpp"

#include "archive/archive_writer.hpp"

namespace cooker
{
CookResult CookGltf(const filesystem::path &file, ArchiveWriter &writer, const CookOptions &options)
{
    constexpr auto extensions = fastgltf::Extensions::KHR_texture_transform |
                                fastgltf::Extensions::KHR_materials_emissive_strength |
                                fastgltf::Extensions::KHR_mesh_quantization |
                                fastgltf::Extensions::KHR_lights_punctual; // parsed now, cooked later
    auto parser = fastgltf::Parser(extensions);

    auto buffer = fastgltf::GltfDataBuffer::FromPath(file);
    if (buffer.error() != fastgltf::Error::None)
        throw runtime_error("Failed to read " + file.string() + ": " + string(fastgltf::getErrorMessage(buffer.error())));

    auto loaded = parser.loadGltf(buffer.get(), file.parent_path(), fastgltf::Options::LoadExternalBuffers);
    if (loaded.error() != fastgltf::Error::None)
        throw runtime_error("Failed to parse " + file.string() + ": " + string(fastgltf::getErrorMessage(loaded.error())));
    auto &gltf = loaded.get();

    // Unsupported content is reported instead of silently dropped.
    auto ctx = CookContext(gltf, file.parent_path(), file.stem().string(), writer);
    if (!gltf.skins.empty())
        ctx.Warn("skins are not supported yet and will be ignored");
    if (!gltf.animations.empty())
        ctx.Warn("animations are not supported yet and will be ignored");

    auto result = CookResult();
    if (options.cook_all_models)
        for (uintsize i = 0; i < gltf.meshes.size(); i++)
            ctx.RequireModel(i);

    if (options.cook_level)
    {
        if (gltf.scenes.empty())
            throw runtime_error("glTF has no scenes: " + file.string());
        auto scene = options.scene.value_or(gltf.defaultScene.value_or(0));
        auto fallback = LevelCooker();
        result.level = (options.level_cooker ? *options.level_cooker : fallback).Cook(ctx, scene);
    }

    result.textures = ctx.Textures();
    return result;
}
} // namespace cooker