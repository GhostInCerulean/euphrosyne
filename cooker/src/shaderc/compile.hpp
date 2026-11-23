#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

// Forward declaration so this header doesn't pull in Diligent.
namespace Diligent
{
struct GraphicsPipelineStateCreateInfo;
}

namespace cooker
{
namespace fs = std::filesystem;

// Which backends to bake into the archive. Keep this to one target per archive for web builds.
enum class Backend : uint32_t
{
    None     = 0,
    D3D11    = 1u << 0,
    D3D12    = 1u << 1,
    Vulkan   = 1u << 2,
    GL       = 1u << 3,
    GLES     = 1u << 4,
    MetalMac = 1u << 5,
    MetalIOS = 1u << 6,
};

constexpr Backend operator|(Backend a, Backend b)
{
    return static_cast<Backend>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

struct Options
{
    Backend     backends = Backend::Vulkan;
    std::string library  = "Diligent-Archiver-shared"; // only used with EXPLICITLY_LOAD_ARCHIVER_FACTORY_DLL
    int         hlslMajor = 6;
    int         hlslMinor = 0;
    uint32_t    contentVersion = 0; // stored in the archive; bump it when your content format changes
};

// Called once per graphics PSO after defaults + // @directives are applied.
// Use it for input layouts, resource signatures, MRT setups, etc.
using PsoHook = std::function<void(const std::string &psoName, Diligent::GraphicsPipelineStateCreateInfo &ci)>;

// Convention:
//   * One .hlsl per pass. Define any of mainVS/mainPS/mainGS/mainHS/mainDS/mainCS in it.
//   * .hlsli (and anything else) is ignored, so use it for shared headers.
//   * mainCS           -> compute PSO
//   * mainVS (+ others)-> graphics PSO
//   * PSO name = path relative to the root, without extension, e.g. "passes/mesh"
//   * Optional render-state directives in comments:
//       // @rtv RGBA8_SRGB | BGRA8_SRGB | RGBA8 | RGBA16F | R11G11B10F | NONE
//       // @dsv D32_FLOAT | D24_S8 | NONE
//       // @cull none | front | back
//       // @depth_test on|off
//       // @depth_write on|off
//       // @blend on|off         (src-alpha / inv-src-alpha)
//       // @topology triangles | lines | points | strip
class ShaderArchiver
{
  public:
    explicit ShaderArchiver(const Options &options = {});
    ~ShaderArchiver();

    ShaderArchiver(const ShaderArchiver &)            = delete;
    ShaderArchiver &operator=(const ShaderArchiver &) = delete;

    // Cooks every .hlsl under root. Throws std::runtime_error after processing
    // everything if any file failed (errors are printed to stderr as they happen).
    void CookDirectory(const fs::path &root, const PsoHook &hook = {});

    std::vector<uint8_t> Serialize();
    void                 SerializeToFile(const fs::path &outFile);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace cooker