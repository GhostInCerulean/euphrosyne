#include "shader_archive.hpp"
#include <algorithm>
#include <fstream>

#include "euphrosyne/utils/hash/fnv1a.hpp"
#include "flux.hpp"

namespace euphrosyne {
    void ShaderArchive::Open(const filesystem::path &path) {
        if (!entries_.empty()) {
            throw runtime_error("ShaderArchive already opened");
        }

        auto file = ifstream(path, ios::binary | ios::ate);
        if (!file) {
            throw runtime_error("Failed to open shader archive file: " + path.string());
        }

        data_.resize(file.tellg());
        file.seekg(0, ios::beg);
        file.read(reinterpret_cast<char *>(data_.data()), data_.size());

        auto header = ShaderArchiveHeader();
        if (data_.size() < sizeof(header)) {
            throw runtime_error("Shader archive file too small: " + path.string());
        }

        memcpy(&header, data_.data(), sizeof(header));
        if (header.magic != kShaderArchiveMagic) {
            throw runtime_error("Invalid shader archive magic: " + path.string());
        }
        if (header.version != kShaderArchiveVersion) {
            throw runtime_error("Unsupported shader archive version: " + path.string());
        }

        auto tableEnd = sizeof(header) + sizeof(ShaderArchiveEntry) * header.numShaders;
        if (tableEnd > data_.size()) {
            throw runtime_error("Shader archive file too small for table: " + path.string());
        }

        entries_.resize(header.numShaders);
        std::memcpy(entries_.data(), data_.data() + sizeof(header), sizeof(ShaderArchiveEntry) * header.numShaders);

        for (const auto &entry: entries_) {
            if (entry.offset + entry.size > data_.size()) {
                throw runtime_error("Shader archive entry out of bounds: " + path.string());
            }
        }
    }

    optional<span<const uint8>> ShaderArchive::Find(string_view name) const {
        auto hash = FNV1A64(name);
        auto it = ranges::lower_bound(entries_, hash, {}, &ShaderArchiveEntry::hash);
        if (it == entries_.end() || it->hash != hash) {
            return nullopt;
        } else {
            return span(data_.data() + it->offset, it->size);
        }
    }

    bool ShaderArchive::Contains(string_view name) const { return Find(name).has_value(); }
} // namespace euphrosyne
