#pragma once
#include <filesystem>
#include <string_view>
#include <vector>


#include "euphrosyne/pch.hpp"
#include "euphrosyne/utils/bytes/tag.hpp"

namespace euphrosyne {
    static constexpr uint32 kArchiveMagic = MakeTag("ARCH"); // btw
    static constexpr uint32 kArchiveVersion = 0;

    struct ArchiveHeader {
        uint32 magic;
        uint32 version;
        uint32 numEntries;
        uint32 reserved;
    };

    struct ArchiveEntry {
        uint64 hash;
        uint64 offset;
        uint64 size;
    };


    /// Multiple files packed as one, all is loaded into memory, made for small files that are
    /// often used in raw form such as shaders or SFX
    class Archive {
    private:
        vector<uint8> data_;
        vector<ArchiveEntry> entries_;
    public:
        void Open(const filesystem::path& path); // TODO async counterpart
        optional<span<const uint8>> Find(string_view name) const;
        bool Contains(string_view name) const;
    };

} // namespace euphrosyne
