#include "compile.hpp"
#include <dxc/dxcapi.h>

namespace cooker {
    using namespace euphrosyne;

    const wchar_t* ProfileFor(const fs::path& p) {
        const std::string stage = p.stem().extension().string();

        if (stage == ".vs") return L"vs_6_0";
        if (stage == ".ps") return L"ps_6_0";
        if (stage == ".cs") return L"cs_6_0";
        if (stage == ".gs") return L"gs_6_0";
        if (stage == ".hs") return L"hs_6_0";
        if (stage == ".ds") return L"ds_6_0";
        else throw runtime_error("Unknown shader stage for file: " + p.string());
    }
    vector<uint8> CompileHlsl(const fs::path &path) {
        auto utils = CComPtr<IDxcUtils>();
        DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));

        auto compiler = CComPtr<IDxcCompiler3>();
        DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));

        auto file = std::ifstream(path, ios::binary);
        auto ss = std::stringstream();
        ss << file.rdbuf();
        auto source = ss.str();

        auto buf = DxcBuffer{source.data(), source.size(), DXC_CP_UTF8};
        auto wpath = path.wstring();
        auto args = std::vector<LPCWSTR>{
        wpath.c_str(),
        L"-E", L"main",
        L"-T", ProfileFor(path),
        L"-spirv",
        L"-fspv-target-env=vulkan1.3",
        };

        auto result = CComPtr<IDxcResult>();
        compiler->Compile(&buf, args.data(), args.size(), nullptr, IID_PPV_ARGS(&result));

        auto errors = CComPtr<IDxcBlobUtf8>();
        result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
        if (errors && errors->GetStringLength() > 0) {
            throw runtime_error("Shader compilation failed:\n" + string(errors->GetStringPointer(), errors->GetStringLength()));
        }

        auto status = HRESULT();
        result->GetStatus(&status);
        if (FAILED(status)) {
            throw runtime_error("Shader compilation failed with HRESULT: " + to_string(status));
        }

        auto spirv = CComPtr<IDxcBlob>();
        result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spirv), nullptr);
        if (!spirv) {
            throw runtime_error("Shader compilation failed: no SPIR-V output");
        }
        auto p = reinterpret_cast<const uint8_t *>(spirv->GetBufferPointer());
        auto size = spirv->GetBufferSize();
        auto data = vector<uint8_t>(p, p + size);
        return data;
    }
} // namespace cooker
