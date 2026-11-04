#include <algorithm>
#include <fstream>
#include "archive.hpp"

#include "euphrosyne/utils/hash/fnv1a.hpp"
#include "flux.hpp"

namespace euphrosyne {
    void Archive::Open(const filesystem::path &path) {
        if (!entries_.empty()) {
            throw runtime_error("Archive already opened");
        }

        auto file = ifstream(path, ios::binary | ios::ate);
        if (!file) {
            throw runtime_error("Failed to open archive file: " + path.string());
        }

        data_.resize(file.tellg());
        file.seekg(0, ios::beg);
        file.read(reinterpret_cast<char *>(data_.data()), data_.size());

        auto header = ArchiveHeader();
        if (data_.size() < sizeof(header)) {
            throw runtime_error("Archive file too small: " + path.string());
        }

        memcpy(&header, data_.data(), sizeof(header));
        if (header.magic != kArchiveMagic) {
            throw runtime_error("Invalid archive magic: " + path.string());
        }
        if (header.version != kArchiveVersion) {
            throw runtime_error("Unsupported archive version: " + path.string());
        }

        auto tableEnd = sizeof(header) + sizeof(ArchiveEntry) * header.numEntries;
        if (tableEnd > data_.size()) {
            throw runtime_error("Archive file too small for table: " + path.string());
        }

        entries_.resize(header.numEntries);
        std::memcpy(entries_.data(), data_.data() + sizeof(header), sizeof(ArchiveEntry) * header.numEntries);

        for (const auto &entry: entries_) {
            if (entry.offset + entry.size > data_.size()) {
                throw runtime_error("Archive entry out of bounds: " + path.string());
            }
        }
    }

    optional<span<const uint8>> Archive::Find(string_view name) const {
        auto hash = FNV1A64(name);
        auto it = ranges::lower_bound(entries_, hash, {}, &ArchiveEntry::hash);
        if (it == entries_.end() || it->hash != hash) {
            return nullopt;
        } else {
            return span(data_.data() + it->offset, it->size);
        }
    }

    bool Archive::Contains(string_view name) const { return Find(name).has_value(); }
} // namespace euphrosyne
