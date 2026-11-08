#include <iostream>

#include "archive/archive_writer.hpp"
#include "shaderc/compile.hpp"
#include <CLI/App.hpp>

#include "euphrosyne/dev/logging.hpp"

using namespace euphrosyne;
using namespace cooker;

namespace fs = std::filesystem;

int main(int argc, char **argv)
{
    auto app = CLI::App{"Euphrosyne asset cooker"};
    argv = app.ensure_utf8(argv);

    auto sfxpack = app.add_subcommand("sfxpack", "Convert and pack sound effects into a single archive");
    auto sfx_root = sfxpack->add_option("sfx_root", "Root directory of sound effect source files")->required();
    auto sfx_out = sfxpack->add_option("sfx_out", "Output path for the sound effect archive")->required();

    auto shaderc = app.add_subcommand("shaderc", "Compile and pack shaders");

    auto shader_roots = vector<string>();
    auto shader_out = string();

    shaderc->add_option("shader_roots", shader_roots, "Root directories of shader source files")
        ->required()
        ->expected(1, -1) // one or more
        ->check(CLI::ExistingDirectory);

    shaderc->add_option("-o,--out", shader_out, "Output path for the shader archive")->required();

    app.require_subcommand();

    CLI11_PARSE(app, argc, argv);

    auto cooker_logger = NewLogger("cooker", quill::LogLevel::Info, LogDestination::Console);

    if (*shaderc)
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

                auto key = fs::relative(entry.path(), root).generic_string();
                auto spirv = CompileHlsl(entry.path());

                LOG_INFO(cooker_logger, "Compiled shader {} to {} bytes of SPIR-V", key, spirv.size());
                writer.Add(std::move(key), std::move(spirv));
            }
        }

        auto written = writer.Write(shader_out);
        LOG_INFO(cooker_logger, "Wrote shader archive {} of {} bytes", shader_out, written);
    }
    else if (*sfxpack)
    {
    }
}