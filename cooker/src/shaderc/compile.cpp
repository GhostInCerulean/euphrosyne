#include "compile.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>

#ifdef MAP_TYPE
#undef MAP_TYPE
#endif

#include "Archiver.h"
#include "ArchiverFactory.h"
#include "ArchiverFactoryLoader.h"
#include "DataBlob.h"
#include "GraphicsTypes.h"
#include "PipelineState.h"
#include "RefCntAutoPtr.hpp"
#include "SerializationDevice.h"
#include "Shader.h"

namespace dg = Diligent;

namespace cooker
{
namespace
{
// ---------------------------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------------------------

std::string ReadFile(const fs::path &p)
{
    std::ifstream f(p, std::ios::binary);
    if (!f)
        throw std::runtime_error("Cannot open " + p.string());
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Replaces comments with spaces (keeps newlines so nothing else shifts).
std::string StripComments(const std::string &s)
{
    std::string out = s;
    const size_t n  = s.size();
    for (size_t i = 0; i < n;)
    {
        if (s[i] == '/' && i + 1 < n && s[i + 1] == '/')
        {
            while (i < n && s[i] != '\n')
                out[i++] = ' ';
        }
        else if (s[i] == '/' && i + 1 < n && s[i + 1] == '*')
        {
            out[i++] = ' ';
            out[i++] = ' ';
            while (i < n && !(s[i] == '*' && i + 1 < n && s[i + 1] == '/'))
            {
                if (s[i] != '\n')
                    out[i] = ' ';
                ++i;
            }
            if (i < n)
            {
                out[i++] = ' ';
                if (i < n)
                    out[i++] = ' ';
            }
        }
        else
            ++i;
    }
    return out;
}

struct EntryInfo
{
    dg::SHADER_TYPE type;
    const char     *name;
};

constexpr EntryInfo kEntries[] = {
    {dg::SHADER_TYPE_VERTEX, "mainVS"},   {dg::SHADER_TYPE_PIXEL, "mainPS"},
    {dg::SHADER_TYPE_GEOMETRY, "mainGS"}, {dg::SHADER_TYPE_HULL, "mainHS"},
    {dg::SHADER_TYPE_DOMAIN, "mainDS"},   {dg::SHADER_TYPE_COMPUTE, "mainCS"},
};

// Finds "<identifier> mainXX (" outside comments. Rejects call sites like "return mainPS(".
std::map<dg::SHADER_TYPE, const char *> DetectEntries(const std::string &source)
{
    const std::string code = StripComments(source);

    std::map<dg::SHADER_TYPE, const char *> found;
    for (const auto &e : kEntries)
    {
        const std::regex re(std::string(R"(\b([A-Za-z_]\w*)\s+)") + e.name + R"(\s*\()");
        for (std::sregex_iterator it(code.begin(), code.end(), re), end; it != end; ++it)
        {
            const std::string prev = (*it)[1];
            if (prev == "return" || prev == "else" || prev == "case")
                continue;
            found[e.type] = e.name;
            break;
        }
    }
    return found;
}

// "// @key value" lines (read from the raw source, since comments are the carrier).
std::map<std::string, std::string> ParseDirectives(const std::string &source)
{
    std::map<std::string, std::string> d;
    std::istringstream                 in(source);
    std::string                        line;
    while (std::getline(in, line))
    {
        size_t i = 0;
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])))
            ++i;
        if (line.compare(i, 2, "//") != 0)
            continue;
        i += 2;
        while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i])))
            ++i;
        if (i >= line.size() || line[i] != '@')
            continue;
        ++i;

        size_t k = i;
        while (k < line.size() && !std::isspace(static_cast<unsigned char>(line[k])))
            ++k;
        std::string key = line.substr(i, k - i);

        while (k < line.size() && std::isspace(static_cast<unsigned char>(line[k])))
            ++k;
        std::string value = line.substr(k);
        while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
            value.pop_back();

        if (!key.empty())
            d[key] = value;
    }
    return d;
}

dg::TEXTURE_FORMAT ParseFormat(const std::string &s)
{
    static const std::map<std::string, dg::TEXTURE_FORMAT> m = {
        {"RGBA8", dg::TEX_FORMAT_RGBA8_UNORM},
        {"RGBA8_SRGB", dg::TEX_FORMAT_RGBA8_UNORM_SRGB},
        {"BGRA8", dg::TEX_FORMAT_BGRA8_UNORM},
        {"BGRA8_SRGB", dg::TEX_FORMAT_BGRA8_UNORM_SRGB},
        {"RGBA16F", dg::TEX_FORMAT_RGBA16_FLOAT},
        {"R11G11B10F", dg::TEX_FORMAT_R11G11B10_FLOAT},
        {"D32_FLOAT", dg::TEX_FORMAT_D32_FLOAT},
        {"D24_S8", dg::TEX_FORMAT_D24_UNORM_S8_UINT},
        {"NONE", dg::TEX_FORMAT_UNKNOWN},
    };
    auto it = m.find(s);
    if (it == m.end())
        throw std::runtime_error("Unknown format directive value: " + s);
    return it->second;
}

bool ParseBool(const std::string &s)
{
    return s == "on" || s == "true" || s == "1";
}

dg::ARCHIVE_DEVICE_DATA_FLAGS ToDiligentFlags(Backend b)
{
    auto has = [&](Backend f) { return (static_cast<uint32_t>(b) & static_cast<uint32_t>(f)) != 0; };

    dg::ARCHIVE_DEVICE_DATA_FLAGS flags = dg::ARCHIVE_DEVICE_DATA_FLAG_NONE;
    if (has(Backend::D3D11))
        flags |= dg::ARCHIVE_DEVICE_DATA_FLAG_D3D11;
    if (has(Backend::D3D12))
        flags |= dg::ARCHIVE_DEVICE_DATA_FLAG_D3D12;
    if (has(Backend::Vulkan))
        flags |= dg::ARCHIVE_DEVICE_DATA_FLAG_VULKAN;
    if (has(Backend::GL))
        flags |= dg::ARCHIVE_DEVICE_DATA_FLAG_GL;
    if (has(Backend::GLES))
        flags |= dg::ARCHIVE_DEVICE_DATA_FLAG_GLES;
    if (has(Backend::MetalMac))
        flags |= dg::ARCHIVE_DEVICE_DATA_FLAG_METAL_MACOS;
    if (has(Backend::MetalIOS))
        flags |= dg::ARCHIVE_DEVICE_DATA_FLAG_METAL_IOS;
    return flags;
}
} // namespace

// ---------------------------------------------------------------------------------------------
// Impl
// ---------------------------------------------------------------------------------------------

struct ShaderArchiver::Impl
{
    Options                                     options;
    dg::ARCHIVE_DEVICE_DATA_FLAGS               flags = dg::ARCHIVE_DEVICE_DATA_FLAG_NONE;
    dg::IArchiverFactory                       *factory = nullptr;
    dg::RefCntAutoPtr<dg::ISerializationDevice> serDevice;
    dg::RefCntAutoPtr<dg::IArchiver>            archiver;

    explicit Impl(const Options &opt) : options(opt), flags(ToDiligentFlags(opt.backends))
    {
        if (flags == dg::ARCHIVE_DEVICE_DATA_FLAG_NONE)
            throw std::runtime_error("No backends selected for the archive");

#ifdef EXPLICITLY_LOAD_ARCHIVER_FACTORY_DLL
        auto getFactory = dg::LoadArchiverFactory(options.library.c_str());
        if (!getFactory)
            throw std::runtime_error("Failed to load archiver library: " + options.library);
        factory = getFactory();
#else
        factory = dg::GetArchiverFactory();
#endif
        if (!factory)
            throw std::runtime_error("Archiver factory is null");

        dg::SerializationDeviceCreateInfo ci;
        // Replaces the old -fspv-target-env=vulkan1.3. Verify field names in SerializationDevice.h.
        ci.Vulkan.ApiVersion = dg::Version{1, 3};
        factory->CreateSerializationDevice(ci, &serDevice);
        if (!serDevice)
            throw std::runtime_error("CreateSerializationDevice failed");

        factory->CreateArchiver(serDevice, &archiver);
        if (!archiver)
            throw std::runtime_error("CreateArchiver failed");
    }

    // Compile one entry point from one file into a serialization-device shader.
    dg::RefCntAutoPtr<dg::IShader> Compile(const fs::path &file, const fs::path &root, const char *entry,
                                           dg::SHADER_TYPE type, const std::string &shaderName)
    {
        const std::string rel = fs::relative(file, root).generic_string();

        // Search the root (for "foo/bar.hlsli" style includes) and the file's own directory.
        std::string searchDirs = root.string();
        const auto  parent     = file.parent_path();
        if (!parent.empty() && parent != root)
            searchDirs += ";" + parent.string();

        dg::RefCntAutoPtr<dg::IShaderSourceInputStreamFactory> streams;
        factory->CreateDefaultShaderSourceStreamFactory(searchDirs.c_str(), &streams);

        dg::RefCntAutoPtr<dg::IDataBlob> compilerOutput;

        dg::ShaderCreateInfo ci;
        ci.Desc.Name                  = shaderName.c_str();
        ci.Desc.ShaderType            = type;
        ci.FilePath                   = rel.c_str();
        ci.EntryPoint                 = entry;
        ci.SourceLanguage             = dg::SHADER_SOURCE_LANGUAGE_HLSL;
        ci.ShaderCompiler             = dg::SHADER_COMPILER_DXC;
        ci.HLSLVersion                = dg::ShaderVersion{static_cast<dg::Uint8>(options.hlslMajor),
                                                           static_cast<dg::Uint8>(options.hlslMinor)};
        ci.pShaderSourceStreamFactory = streams;

        dg::ShaderArchiveInfo archiveInfo;
        archiveInfo.DeviceFlags = flags;

        dg::RefCntAutoPtr<dg::IShader> shader;
        serDevice->CreateShader(ci, archiveInfo, &shader, &compilerOutput);

        if (compilerOutput && compilerOutput->GetSize() > 0)
        {
            std::cerr << "[" << shaderName << "] compiler output:\n"
                      << std::string(static_cast<const char *>(compilerOutput->GetConstDataPtr()),
                                     compilerOutput->GetSize())
                      << "\n";
        }
        if (!shader)
            throw std::runtime_error("Shader compilation failed: " + shaderName);
        return shader;
    }

    void AddCompute(const std::string &name, dg::IShader *cs)
    {
        dg::ComputePipelineStateCreateInfo ci;
        ci.PSODesc.Name         = name.c_str();
        ci.PSODesc.PipelineType = dg::PIPELINE_TYPE_COMPUTE;
        ci.pCS                  = cs;

        dg::PipelineStateArchiveInfo archiveInfo;
        archiveInfo.DeviceFlags = flags;

        dg::RefCntAutoPtr<dg::IPipelineState> pso;
        serDevice->CreateComputePipelineState(ci, archiveInfo, &pso);
        if (!pso)
            throw std::runtime_error("Failed to create compute PSO: " + name);
        archiver->AddPipelineState(pso);
    }

    void AddGraphics(const std::string &name, dg::GraphicsPipelineStateCreateInfo &ci)
    {
        dg::PipelineStateArchiveInfo archiveInfo;
        archiveInfo.DeviceFlags = flags;

        dg::RefCntAutoPtr<dg::IPipelineState> pso;
        serDevice->CreateGraphicsPipelineState(ci, archiveInfo, &pso);
        if (!pso)
            throw std::runtime_error("Failed to create graphics PSO: " + name);
        archiver->AddPipelineState(pso);
    }

    void CookFile(const fs::path &path, const fs::path &root, const PsoHook &hook)
    {
        const std::string source  = ReadFile(path);
        const auto        entries = DetectEntries(source);
        if (entries.empty())
        {
            std::cerr << "Skipping (no mainXX entry): " << path.string() << "\n";
            return;
        }

        const std::string name = fs::relative(path, root).replace_extension().generic_string();

        // Compile each entry once; shader names are "<pso>:<entry>".
        std::map<dg::SHADER_TYPE, dg::RefCntAutoPtr<dg::IShader>> shaders;
        for (const auto &[type, entry] : entries)
            shaders[type] = Compile(path, root, entry, type, name + ":" + entry);

        if (auto cs = shaders.find(dg::SHADER_TYPE_COMPUTE); cs != shaders.end())
        {
            if (shaders.size() > 1)
                std::cerr << path.string() << ": mixes mainCS with graphics stages, cooking compute only\n";
            AddCompute(name, cs->second);
            return;
        }

        if (!shaders.count(dg::SHADER_TYPE_VERTEX))
            throw std::runtime_error(path.string() + ": graphics stages found but no mainVS");

        dg::GraphicsPipelineStateCreateInfo ci;
        ci.PSODesc.Name                               = name.c_str();
        ci.PSODesc.PipelineType                       = dg::PIPELINE_TYPE_GRAPHICS;
        ci.PSODesc.ResourceLayout.DefaultVariableType = dg::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

        auto &g                             = ci.GraphicsPipeline;
        g.PrimitiveTopology                 = dg::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        g.NumRenderTargets                  = 1;
        g.RTVFormats[0]                     = dg::TEX_FORMAT_RGBA8_UNORM_SRGB;
        g.DSVFormat                         = dg::TEX_FORMAT_D32_FLOAT;
        g.RasterizerDesc.CullMode           = dg::CULL_MODE_BACK;
        g.DepthStencilDesc.DepthEnable      = true;
        g.DepthStencilDesc.DepthWriteEnable = true;
        g.DepthStencilDesc.DepthFunc        = dg::COMPARISON_FUNC_LESS;

        const auto d = ParseDirectives(source);
        auto       get = [&](const char *k) -> const std::string * {
            auto it = d.find(k);
            return it == d.end() ? nullptr : &it->second;
        };

        if (auto v = get("rtv"))
        {
            g.RTVFormats[0]    = ParseFormat(*v);
            g.NumRenderTargets = g.RTVFormats[0] == dg::TEX_FORMAT_UNKNOWN ? 0 : 1;
        }
        if (auto v = get("dsv"))
        {
            g.DSVFormat = ParseFormat(*v);
            if (g.DSVFormat == dg::TEX_FORMAT_UNKNOWN)
                g.DepthStencilDesc.DepthEnable = false;
        }
        if (auto v = get("cull"))
        {
            g.RasterizerDesc.CullMode = *v == "none"    ? dg::CULL_MODE_NONE
                                        : *v == "front" ? dg::CULL_MODE_FRONT
                                                        : dg::CULL_MODE_BACK;
        }
        if (auto v = get("depth_test"))
            g.DepthStencilDesc.DepthEnable = ParseBool(*v);
        if (auto v = get("depth_write"))
            g.DepthStencilDesc.DepthWriteEnable = ParseBool(*v);
        if (auto v = get("blend"); v && ParseBool(*v))
        {
            auto &rt        = g.BlendDesc.RenderTargets[0];
            rt.BlendEnable  = true;
            rt.SrcBlend     = dg::BLEND_FACTOR_SRC_ALPHA;
            rt.DestBlend    = dg::BLEND_FACTOR_INV_SRC_ALPHA;
            rt.BlendOp      = dg::BLEND_OPERATION_ADD;
            rt.SrcBlendAlpha  = dg::BLEND_FACTOR_ONE;
            rt.DestBlendAlpha = dg::BLEND_FACTOR_INV_SRC_ALPHA;
            rt.BlendOpAlpha   = dg::BLEND_OPERATION_ADD;
        }
        if (auto v = get("topology"))
        {
            g.PrimitiveTopology = *v == "lines"   ? dg::PRIMITIVE_TOPOLOGY_LINE_LIST
                                  : *v == "points" ? dg::PRIMITIVE_TOPOLOGY_POINT_LIST
                                  : *v == "strip"  ? dg::PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP
                                                   : dg::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }

        ci.pVS = shaders[dg::SHADER_TYPE_VERTEX];
        if (shaders.count(dg::SHADER_TYPE_PIXEL))
            ci.pPS = shaders[dg::SHADER_TYPE_PIXEL];
        if (shaders.count(dg::SHADER_TYPE_GEOMETRY))
            ci.pGS = shaders[dg::SHADER_TYPE_GEOMETRY];
        if (shaders.count(dg::SHADER_TYPE_HULL))
            ci.pHS = shaders[dg::SHADER_TYPE_HULL];
        if (shaders.count(dg::SHADER_TYPE_DOMAIN))
            ci.pDS = shaders[dg::SHADER_TYPE_DOMAIN];

        if (hook)
            hook(name, ci);

        AddGraphics(name, ci);
    }
};

// ---------------------------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------------------------

ShaderArchiver::ShaderArchiver(const Options &options) : impl_(std::make_unique<Impl>(options)) {}
ShaderArchiver::~ShaderArchiver() = default;

void ShaderArchiver::CookDirectory(const fs::path &root, const PsoHook &hook)
{
    std::vector<fs::path> files;
    for (const auto &e : fs::recursive_directory_iterator(root))
        if (e.is_regular_file() && e.path().extension() == ".hlsl")
            files.push_back(e.path());
    std::sort(files.begin(), files.end()); // deterministic archive contents

    size_t failures = 0;
    for (const auto &f : files)
    {
        try
        {
            impl_->CookFile(f, root, hook);
        }
        catch (const std::exception &ex)
        {
            ++failures;
            std::cerr << "ERROR: " << f.string() << ": " << ex.what() << "\n";
        }
    }
    if (failures)
        throw std::runtime_error(std::to_string(failures) + " shader file(s) failed to cook");
}

std::vector<uint8_t> ShaderArchiver::Serialize()
{
    dg::RefCntAutoPtr<dg::IDataBlob> blob;
    impl_->archiver->SerializeToBlob(impl_->options.contentVersion, &blob);
    if (!blob)
        throw std::runtime_error("Archive serialization failed");

    const auto *p = static_cast<const uint8_t *>(blob->GetConstDataPtr());
    return std::vector<uint8_t>(p, p + blob->GetSize());
}

void ShaderArchiver::SerializeToFile(const fs::path &outFile)
{
    const auto bytes = Serialize();
    if (!outFile.parent_path().empty())
        fs::create_directories(outFile.parent_path());

    std::ofstream f(outFile, std::ios::binary);
    if (!f)
        throw std::runtime_error("Cannot write " + outFile.string());
    f.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}
} // namespace cooker