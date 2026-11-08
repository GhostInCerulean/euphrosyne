#include "compile.hpp"
#include <dxc/dxcapi.h>
#include <iostream>

namespace cooker
{
using namespace euphrosyne;

const wchar_t *ProfileFor(const fs::path &p)
{
    const std::string stage = p.stem().extension().string();

    if (stage == ".vs")
        return L"vs_6_0";
    if (stage == ".ps")
        return L"ps_6_0";
    if (stage == ".cs")
        return L"cs_6_0";
    if (stage == ".gs")
        return L"gs_6_0";
    if (stage == ".hs")
        return L"hs_6_0";
    if (stage == ".ds")
        return L"ds_6_0";
    throw runtime_error("Unknown shader stage for file: " + p.string());
}

const wchar_t *EntryFor(const fs::path &p)
{
    const std::string stage = p.stem().extension().string();

    if (stage == ".vs")
        return L"mainVS";
    if (stage == ".ps")
        return L"mainPS";
    if (stage == ".cs")
        return L"mainCS";
    if (stage == ".gs")
        return L"mainGS";
    if (stage == ".hs")
        return L"mainHS";
    if (stage == ".ds")
        return L"mainDS";
    throw runtime_error("Unknown shader stage for file: " + p.string());
}

vector<uint8> CompileHlsl(const fs::path &path)
{
    auto utils = CComPtr<IDxcUtils>();
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));

    auto compiler = CComPtr<IDxcCompiler3>();
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

    auto includeHandler = CComPtr<IDxcIncludeHandler>();
    utils->CreateDefaultIncludeHandler(&includeHandler);

    auto file = std::ifstream(path, ios::binary);
    auto ss = std::stringstream();
    ss << file.rdbuf();
    auto source = ss.str();

    auto buf = DxcBuffer{source.data(), source.size(), DXC_CP_UTF8};

    // These must outlive the Compile() call, since args stores raw pointers
    auto wpath = path.wstring();
    auto dir = path.parent_path();
    auto wdir = (dir.empty() ? fs::path(".") : dir).wstring();

    auto args = std::vector<LPCWSTR>{
        wpath.c_str(),
        L"-I",
        wdir.c_str(),
        L"-E",
        EntryFor(path),
        L"-T",
        ProfileFor(path),
        L"-spirv",
        L"-fspv-target-env=vulkan1.3",
        // Match NVRHI's default Vulkan binding offsets
        L"-fvk-t-shift",
        L"0",
        L"all",
        L"-fvk-s-shift",
        L"128",
        L"all",
        L"-fvk-b-shift",
        L"256",
        L"all",
        L"-fvk-u-shift",
        L"384",
        L"all",
    };

    auto result = CComPtr<IDxcResult>();
    compiler->Compile(&buf, args.data(), (UINT32)args.size(), includeHandler, IID_PPV_ARGS(&result));

    auto errors = CComPtr<IDxcBlobUtf8>();
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    const bool hasMessages = errors && errors->GetStringLength() > 0;

    auto status = HRESULT();
    result->GetStatus(&status);
    if (FAILED(status))
    {
        string msg = hasMessages ? string(errors->GetStringPointer(), errors->GetStringLength()) : "unknown error";
        throw runtime_error("Shader compilation failed for " + path.string() + ":\n" + msg);
    }

    // Succeeded, but there may be warnings
    if (hasMessages)
    {
        std::cerr << "Shader warnings for " << path.string() << ":\n"
                  << string(errors->GetStringPointer(), errors->GetStringLength()) << "\n";
    }

    auto spirv = CComPtr<IDxcBlob>();
    result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirv), nullptr);
    if (!spirv)
    {
        throw runtime_error("Shader compilation failed: no SPIR-V output");
    }
    auto p = reinterpret_cast<const uint8_t *>(spirv->GetBufferPointer());
    auto size = spirv->GetBufferSize();
    return vector<uint8_t>(p, p + size);
}
} // namespace cooker