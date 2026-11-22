#include "gltf_cooker.hpp"

#include <iostream>
#include <set>

#include "archive/archive_writer.hpp"
#include "blender/blender_to_gltf.hpp"
#include "shaderc/compile.hpp"
#include "texture_cooker.hpp"
#include <CLI/App.hpp>
#include <boost/asio/io_context.hpp>

#include "euphrosyne/dev/logging.hpp"
#include "euphrosyne/utils/io/file.hpp"

using namespace euphrosyne;
using namespace cooker;

namespace fs = std::filesystem;

int main(int argc, char **argv)
{
    auto ioc = asio::io_context();
    auto app = CLI::App{"Euphrosyne asset cooker"};
    argv = app.ensure_utf8(argv);

    // levels command
    auto levels = app.add_subcommand("levels", "Cook glTF/Blender files into levels (plus models, materials, textures)");
    auto level_files = vector<string>();
    auto level_out_dir = string();
    auto level_texture_out = string();
    auto level_all_models = false;
    auto level_scene = optional<uintsize>();

    levels->add_option("files", level_files, "glTF/GLB/.blend files to cook")
        ->required()
        ->expected(1, -1)
        ->check(CLI::ExistingFile);
    levels->add_option("-o,--out", level_out_dir, "Directory for the per-level archives (<name>.arc)")->required();
    levels->add_option("-t,--textures", level_texture_out, "Output path for the shared texture archive")->required();
    levels->add_flag("--all-models", level_all_models, "Also cook meshes the scene doesn't use (prop libraries)");
    levels->add_option("--scene", level_scene, "Scene index (default: the glTF's default scene)");

    // shaders command
    auto shaders = app.add_subcommand("shaders", "Compile and pack shaders");
    auto shader_roots = vector<string>();
    auto shader_out = string();

    shaders->add_option("shader_roots", shader_roots, "Root directories of shader source files")
        ->required()
        ->expected(1, -1) // one or more
        ->check(CLI::ExistingDirectory);

    shaders->add_option("-o,--out", shader_out, "Output path for the shader archive")->required();

    // archive command
    auto archive = app.add_subcommand("archive", "Pack assets into a single archive");
    auto archive_files = vector<string>();
    auto archive_out = string();

    archive->add_option("files", archive_files, "Paths to assets to pack")->expected(1, -1)->required();

    archive->add_option("-o,--out", archive_out, "Output path for the archive");

    app.require_subcommand();

    CLI11_PARSE(app, argc, argv);

    auto cooker_logger = NewLogger("cooker", quill::LogLevel::Info, LogDestination::Console);

    if (*shaders)
    {
        auto writer = ArchiveWriter();

        for (const auto &root : shader_roots)
        {
            for (const auto &entry : fs::recursive_directory_iterator(root))
            {
                if (!entry.is_regular_file() || entry.path().extension() != ".hlsl")
                {
                    continue;
                }

                auto entry_path = entry.path();
                entry_path.make_preferred();
                auto key = fs::relative(entry_path, root).make_preferred().generic_string();
                LOG_INFO(cooker_logger, "Compiling shader {}...", key);
                auto spirv = CompileHlsl(entry.path());
                LOG_INFO(cooker_logger, "Compiled shader {} to {} bytes of SPIR-V", key, spirv.size());
                writer.Add(std::move(key), std::move(spirv));
            }
        }

        auto out_path = fs::path(shader_out);
        if (out_path.has_parent_path())
            fs::create_directories(out_path.parent_path());

        auto written = writer.Write(shader_out);
        LOG_INFO(cooker_logger, "Wrote shader archive {} of {} bytes", shader_out, written);
    }
    else if (*archive)
    {
        auto writer = ArchiveWriter();
        for (const auto &file : archive_files)
        {
            auto data = ReadFile(file);
            writer.Add(file, std::move(data));
        }

        auto out_path = fs::path(archive_out);
        if (out_path.has_parent_path())
            fs::create_directories(out_path.parent_path());

        auto written = writer.Write(archive_out);
        LOG_INFO(cooker_logger, "Wrote archive {} of {} bytes", archive_out, written);
    }
    else if (*levels)
    {
        fs::create_directories(level_out_dir);
        auto texture_out_path = fs::path(level_texture_out);
        if (texture_out_path.has_parent_path())
            fs::create_directories(texture_out_path.parent_path());

        // Textures are shared between every level cooked in this run: one writer, one cooker, so an
        // image used by several files is encoded and stored once.
        auto texture_writer = ArchiveWriter();
        auto texture_cooker = TextureCooker(texture_writer);

        // .blend files are exported to a glTF named after the blend file, so entry names and the
        // output archive use the real name instead of a shared "temp".
        auto temp_dir = fs::temp_directory_path() / "euphrosyne_cooker";
        auto options = CookOptions{.cook_all_models = level_all_models, .scene = level_scene};
        auto names = set<string>();

        for (const auto &input : level_files)
        {
            auto input_path = fs::path(input);
            auto name = input_path.stem().string();
            if (!names.insert(name).second)
                throw runtime_error("Two inputs share the name '" + name + "', their archives would collide");

            auto gltf_path = input_path;
            if (input_path.extension() == ".blend")
            {
                LOG_INFO(cooker_logger, "Exporting blend file {} to glTF", input);
                fs::create_directories(temp_dir);
                gltf_path = temp_dir / (name + ".gltf");
                BlenderToGltf(ioc, input, gltf_path);

                // Blender picks the extension from its export format, so a requested .gltf can come out as .glb.
                if (!fs::exists(gltf_path))
                {
                    auto glb_path = fs::path(gltf_path).replace_extension(".glb");
                    if (fs::exists(glb_path))
                        gltf_path = glb_path;
                    else
                    {
                        auto found = string();
                        for (const auto &entry : fs::directory_iterator(temp_dir))
                            found += " " + entry.path().filename().string();
                        throw runtime_error("Blender export produced no " + gltf_path.string() + " (temp dir contains:" +
                                            (found.empty() ? " nothing" : found) + ")");
                    }
                }
            }

            LOG_INFO(cooker_logger, "Cooking level file {}", gltf_path.string());
            auto writer = ArchiveWriter();
            auto result = CookGltf(gltf_path, writer, options);
            LOG_INFO(cooker_logger, "Cooked {}: level hash {:#x}, {} textures requested", name, result.level,
                     result.textures.size());

            // Texture requests carry the encoded image bytes, so this is safe before temp files are removed.
            texture_cooker.Cook(result.textures);

            auto out_path = fs::path(level_out_dir) / name;
            auto written = writer.Write(out_path);
            LOG_INFO(cooker_logger, "Wrote level archive {} of {} bytes", out_path.string(), written);
        }

        auto written = texture_writer.Write(texture_out_path);
        LOG_INFO(cooker_logger, "Wrote texture archive {} of {} bytes ({} textures)", level_texture_out, written,
                 texture_cooker.Count());

        auto ec = std::error_code();
        fs::remove_all(temp_dir, ec);
    }
    return 0;
}
