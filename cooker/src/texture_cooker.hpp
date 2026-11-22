#pragma once
#include <span>
#include <unordered_set>

#include "cook_context.hpp"

namespace cooker
{
/// Turns TextureRequests into KTX2 archive entries: decode -> mip chain -> UASTC -> transcode to BCn.
/// Formats by role: BaseColor/Emissive BC7 sRGB, MetallicRoughness BC7, Normal BC5 (RG, rebuild Z in
/// the shader), Occlusion BC4. Output has no supercompression, so the runtime uploads straight from mmap.
///
/// Keep one instance alive across all glTF files you cook into the same archive: it remembers what it
/// already emitted, so a texture shared by several files is encoded and stored once.
class TextureCooker
{
public:
    explicit TextureCooker(ArchiveWriter &writer) : writer_(writer) {}

    void Cook(span<const TextureRequest> requests);
    uintsize Count() const { return done_.size(); }

private:
    ArchiveWriter &writer_;
    unordered_set<uint64> done_;
};
} // namespace cooker