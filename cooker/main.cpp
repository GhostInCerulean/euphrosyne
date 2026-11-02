#include <iostream>

#include "shaders/shader_archive_writer.hpp"
#include <CLI/App.hpp>

using namespace euphrosyne;
using namespace cooker;

int main(int argc, char** argv) {
    auto app = CLI::App{"Euphrosyne asset cooker"};
    argv = app.ensure_utf8(argv);

    auto shaderc = app.add_subcommand("shaderc", "Compile and pack shaders");
    auto shader_root = shaderc->add_option("shader_root", "Root directory of shader source files")->required();
    auto shader_out = shaderc->add_option("shader_out", "Output path for the shader archive")->required();

    app.require_subcommand();

    CLI11_PARSE(app, argc, argv);

    if (*shaderc) {

    }
}