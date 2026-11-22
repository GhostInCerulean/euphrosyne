#include "blender_to_gltf.hpp"
#include <boost/process/v2/environment.hpp>
#include <boost/process/v2/process.hpp>

namespace cooker
{
// is this tuff in south east asia?
namespace bp = boost::process::v2;

void BlenderToGltf(asio::io_context &ioc, const filesystem::path &in, const filesystem::path &out)
{
    auto blender = bp::environment::find_executable("blender");
    if (blender.empty())
        throw runtime_error("blender not found");

    auto args = vector<string>{"--background", in.string(), "--python-expr",
                               "import bpy; bpy.ops.export_scene.gltf(filepath='" + out.string() + "')"};

    auto process = bp::process(ioc, blender, args);
    auto exit_code = process.wait();
    if (exit_code != 0)
        throw runtime_error("blender failed");
}
} // namespace cooker