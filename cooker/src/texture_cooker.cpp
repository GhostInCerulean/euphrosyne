#include "texture_cooker.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <thread>

#include <ktx.h>
#include <vulkan/vulkan_core.h>

// Single translation unit for the stb implementations.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

#include "euphrosyne/formats/texture.hpp"

namespace cooker
{
namespace
{
struct RoleFormat
{
    VkFormat source;            // what we feed libktx
    ktx_transcode_fmt_e target; // block format we end up with
    uint32 components;
    bool srgb;
    bool normal;
};

RoleFormat FormatFor(TextureRole role)
{
    switch (role)
    {
    case TextureRole::BaseColor:
    case TextureRole::Emissive: return {VK_FORMAT_R8G8B8A8_SRGB, KTX_TTF_BC7_RGBA, 4, true, false};
    case TextureRole::Normal: return {VK_FORMAT_R8G8_UNORM, KTX_TTF_BC5_RG, 2, false, true};
    case TextureRole::Occlusion: return {VK_FORMAT_R8_UNORM, KTX_TTF_BC4_R, 1, false, false};
    default: return {VK_FORMAT_R8G8B8A8_UNORM, KTX_TTF_BC7_RGBA, 4, false, false}; // MetallicRoughness
    }
}

void Check(KTX_error_code code, const char *what)
{
    if (code != KTX_SUCCESS)
        throw runtime_error(string("libktx ") + what + " failed: " + ktxErrorString(code));
}

struct KtxDeleter
{
    void operator()(ktxTexture2 *t) const { ktxTexture_Destroy(ktxTexture(t)); }
};

struct Decoded
{
    uint32 width = 0, height = 0;
    vector<uint8> pixels; // tightly packed, `components` per pixel
};

// Always decode to RGBA8, then keep only the channels the role needs.
Decoded Decode(const vector<uint8> &encoded, const RoleFormat &format, const string &name)
{
    int w, h, comp;
    auto *rgba = stbi_load_from_memory(encoded.data(), static_cast<int>(encoded.size()), &w, &h, &comp, 4);
    if (!rgba)
        throw runtime_error("Failed to decode " + name + ": " + stbi_failure_reason());
    auto guard = unique_ptr<stbi_uc, void (*)(void *)>(rgba, stbi_image_free);

    auto out = Decoded{.width = static_cast<uint32>(w), .height = static_cast<uint32>(h)};
    auto count = uintsize(w) * h;
    out.pixels.resize(count * format.components);
    for (uintsize i = 0; i < count; i++)
        for (uint32 c = 0; c < format.components; c++)
            out.pixels[i * format.components + c] = rgba[i * 4 + c]; // RGBA / RG / R
    return out;
}

// Downsampled normals are shorter than unit length; rebuild Z from XY and renormalize.
void RenormalizeXY(vector<uint8> &rg)
{
    for (uintsize i = 0; i + 1 < rg.size(); i += 2)
    {
        auto x = rg[i] / 127.5f - 1.0f, y = rg[i + 1] / 127.5f - 1.0f;
        auto z = std::sqrt(std::max(0.0f, 1.0f - x * x - y * y));
        auto len = std::sqrt(x * x + y * y + z * z);
        if (len > 0.0f)
        {
            x /= len;
            y /= len;
        }
        rg[i] = static_cast<uint8>(std::lround((x * 0.5f + 0.5f) * 255.0f));
        rg[i + 1] = static_cast<uint8>(std::lround((y * 0.5f + 0.5f) * 255.0f));
    }
}

// Every level is resampled from the base image (not from the previous level) for better quality;
// sRGB data is filtered in linear space by stb.
vector<vector<uint8>> BuildMips(Decoded base, const RoleFormat &format)
{
    auto levels = 1u;
    for (auto d = max(base.width, base.height); d > 1; d >>= 1)
        levels++;

    auto layout = format.components == 1   ? STBIR_1CHANNEL
                  : format.components == 2 ? STBIR_2CHANNEL
                                           : STBIR_RGBA;
    auto mips = vector<vector<uint8>>();
    mips.reserve(levels);
    auto width = base.width, height = base.height;
    mips.push_back(std::move(base.pixels));

    for (uint32 level = 1; level < levels; level++)
    {
        auto lw = max(1u, width >> level), lh = max(1u, height >> level);
        auto out = vector<uint8>(uintsize(lw) * lh * format.components);
        auto *result =
            format.srgb
                ? stbir_resize_uint8_srgb(mips[0].data(), width, height, width * format.components, out.data(), lw,
                                          lh, lw * format.components, layout)
                : stbir_resize_uint8_linear(mips[0].data(), width, height, width * format.components, out.data(), lw,
                                            lh, lw * format.components, layout);
        if (!result)
            throw runtime_error("Mip resize failed");
        if (format.normal)
            RenormalizeXY(out);
        mips.push_back(std::move(out));
    }
    return mips;
}

vector<uint8> CookTexture(const TextureRequest &request)
{
    auto format = FormatFor(request.role);
    auto decoded = Decode(*request.encoded, format, request.name);
    auto width = decoded.width, height = decoded.height;
    if (width % 4 != 0 || height % 4 != 0)
        cerr << "[cooker] " << request.name << ": " << width << "x" << height
             << " is not a multiple of 4 (fine on Vulkan, D3D12 may reject BC textures like this)\n";

    auto mips = BuildMips(std::move(decoded), format);

    auto info = ktxTextureCreateInfo();
    info.vkFormat = format.source;
    info.baseWidth = width;
    info.baseHeight = height;
    info.baseDepth = 1;
    info.numDimensions = 2;
    info.numLevels = static_cast<ktx_uint32_t>(mips.size());
    info.numLayers = 1;
    info.numFaces = 1;
    info.isArray = KTX_FALSE;
    info.generateMipmaps = KTX_FALSE;

    ktxTexture2 *raw = nullptr;
    Check(ktxTexture2_Create(&info, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &raw), "create");
    auto texture = unique_ptr<ktxTexture2, KtxDeleter>(raw);
    for (uintsize level = 0; level < mips.size(); level++)
        Check(ktxTexture_SetImageFromMemory(ktxTexture(raw), static_cast<ktx_uint32_t>(level), 0, 0,
                                            mips[level].data(), mips[level].size()),
              "set image");

    // UASTC -> transcode to BCn gives BC7/BC5/BC4 without a separate BCn encoder dependency.
    auto params = ktxBasisParams();
    params.structSize = sizeof(params);
    params.uastc = KTX_TRUE;
    params.uastcFlags = KTX_PACK_UASTC_LEVEL_DEFAULT;
    params.normalMap = format.normal ? KTX_TRUE : KTX_FALSE;
    params.threadCount = max(1u, thread::hardware_concurrency());
    Check(ktxTexture2_CompressBasisEx(raw, &params), "compress");
    Check(ktxTexture2_TranscodeBasis(raw, format.target, 0), "transcode");

    ktx_uint8_t *bytes = nullptr;
    ktx_size_t size = 0;
    Check(ktxTexture_WriteToMemory(ktxTexture(raw), &bytes, &size), "write");
    auto out = vector<uint8>(bytes, bytes + size);
    free(bytes);

    // Anything we emit must be readable by the runtime parser.
    if (!TextureView::Parse(out))
        throw runtime_error("Cooked texture " + request.name + " is not readable by TextureView");
    return out;
}
} // namespace

void TextureCooker::Cook(span<const TextureRequest> requests)
{
    for (const auto &request : requests)
    {
        if (!done_.insert(request.hash).second)
            continue;
        cerr << "[cooker] texture " << request.name << "...\n";
        writer_.Add(request.name, CookTexture(request));
    }
}
} // namespace cooker