#include "shader_archive_writer.hpp"

#include <fstream>

#include "euphrosyne/formats/shader_archive.hpp"
#include "euphrosyne/utils/bytes/align.hpp"
#include "euphrosyne/utils/hash/fnv1a.hpp"

namespace cooker {
    void ShaderArchiveWriter::Add(string name, vector<uint8> data) {
        auto [it, inserted] = items.emplace(FNV1A64(name), Item{std::move(name), std::move(data)});
        if (!inserted) {
            throw runtime_error("ShaderArchiveWriter: Duplicate shader name or hash collision: " + name);
        }
    }

    void ShaderArchiveWriter::Write(const filesystem::path &path) const {
        auto file = ofstream(path, ios::binary | ios::trunc);
        if (!file) {
            throw runtime_error("Failed to open shader archive file for writing: " + path.string());
        }

        auto entries = vector<ShaderArchiveEntry>();
        auto cursor = AlignTo(sizeof(ShaderArchiveHeader) + sizeof(ShaderArchiveEntry) * items.size(), 16);
        for (const auto &[hash, item]: items) {
            entries.push_back({.hash = hash, .offset = cursor, .size = item.data.size()});
            cursor += AlignTo(item.data.size(), 16);
        }

        auto header = ShaderArchiveHeader{
            .magic = kShaderArchiveMagic,
            .version = kShaderArchiveVersion,
            .numShaders = static_cast<uint32>(items.size()),
            .reserved = 0,
        };
        file.write(reinterpret_cast<const char *>(&header), sizeof(header));
        file.write(reinterpret_cast<const char *>(entries.data()), sizeof(ShaderArchiveEntry) * entries.size());

        static const char zeros[16] = {};
        auto pos = sizeof(header) + sizeof(ShaderArchiveEntry) * entries.size();
        auto i = uintsize(0);

        for (const auto& [hash, item] : items) {
            file.write(zeros, streamsize(entries[i].offset - pos));
            file.write(reinterpret_cast<const ostream::char_type *>(item.data.data()), streamsize(item.data.size()));
            pos = entries[i].offset + item.data.size();
            i++;
        }

        if (!file) {
            throw runtime_error("Failed to write shader archive file: " + path.string());
        }
    }
} // namespace cooker
