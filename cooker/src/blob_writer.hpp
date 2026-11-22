#pragma once
#include <cstring>
#include <limits>
#include <span>

#include "euphrosyne/formats/asset.hpp"
#include "euphrosyne/utils/bytes/align.hpp"

namespace cooker
{
using namespace euphrosyne;

inline uint32 ToU32(uintsize value)
{
    if (value > numeric_limits<uint32>::max())
        throw runtime_error("Asset exceeds 4 GiB");
    return static_cast<uint32>(value);
}

/// Builds one asset blob: `Header` (must start with `AssetHeader header;`) followed by 16-byte
/// aligned sections. Append() returns the Range to store in the header. Re-fetch Head() after
/// appending, the buffer may have moved.
template <class Header> class BlobWriter
{
    static_assert(is_trivially_copyable_v<Header>);
    vector<uint8> buffer_;

    void Pad() { buffer_.resize(AlignTo(buffer_.size(), kAssetAlignment)); }

  public:
    BlobWriter(uint32 magic, uint32 version) : buffer_(sizeof(Header))
    {
        Head().header = AssetHeader{.magic = magic, .version = version, .size = 0, .reserved = 0};
    }

    Header &Head() { return *reinterpret_cast<Header *>(buffer_.data()); }

    template <class T> Range Append(span<const T> items)
    {
        static_assert(is_trivially_copyable_v<T>);
        Pad();
        auto at = buffer_.size();
        auto range = Range{.offset = ToU32(at), .count = ToU32(items.size())};
        buffer_.resize(at + items.size_bytes());
        if (!items.empty())
            memcpy(buffer_.data() + at, items.data(), items.size_bytes());
        return range;
    }

    template <class T> Range Append(const vector<T> &items) { return Append(span<const T>(items)); }

    vector<uint8> Finish()
    {
        Pad();
        Head().header.size = ToU32(buffer_.size());
        return std::move(buffer_);
    }
};
} // namespace cooker
