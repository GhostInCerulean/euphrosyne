#include "archive.hpp"
#include <algorithm>
#include <boost/asio/random_access_file.hpp>
#include <fstream>

#include "euphrosyne/utils/hash/fnv1a.hpp"
#include "flux.hpp"
#include <boost/asio/as_tuple.hpp>
#include <boost/asio/read_at.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace euphrosyne
{
void Archive::Open(const filesystem::path &path)
{
    if (!entries_.empty())
    {
        throw runtime_error("Archive already opened");
    }

    auto file = ifstream(path, ios::binary | ios::ate);
    if (!file)
    {
        throw runtime_error("Failed to open archive file: " + path.string());
    }

    auto data = vector<uint8>(file.tellg());
    file.seekg(0, ios::beg);
    file.read(reinterpret_cast<char *>(data.data()), data.size());

    Load(std::move(data), path);
}

awaitable<void> Archive::OpenAsync(const filesystem::path &path)
{
    if (!entries_.empty())
        throw runtime_error("Archive already opened");

    auto file = asio::random_access_file(co_await asio::this_coro::executor);
    auto error_code = boost::system::error_code();
    file.open(path, asio::file_base::read_only, error_code);
    if (error_code)
        throw runtime_error("Failed to open archive file");

    auto size = file.size(error_code);
    if (error_code)
        throw runtime_error("Failed to query archive size");

    auto data = vector<uint8>(size);
    auto [read_error_code, bytes_red] =
        co_await asio::async_read_at(file, 0, asio::buffer(data), asio::as_tuple(asio::use_awaitable));
    if (read_error_code || bytes_red != size)
        throw runtime_error("Failed to read archive file");

    Load(std::move(data), path);
}

optional<span<const uint8>> Archive::Find(string_view name) const
{
    auto hash = FNV1A64(name);
    auto it = ranges::lower_bound(entries_, hash, {}, &ArchiveEntry::hash);
    if (it == entries_.end() || it->hash != hash)
    {
        return nullopt;
    }
    else
    {
        return span(data_.data() + it->offset, it->size);
    }
}

bool Archive::Contains(string_view name) const
{
    return Find(name).has_value();
}

void Archive::Load(vector<uint8> data, const filesystem::path &path)
{
    auto header = ArchiveHeader();
    if (data.size() < sizeof(header))
        throw runtime_error("Archive file too small");
    memcpy(&header, data.data(), sizeof(ArchiveHeader));

    if (header.magic != kArchiveMagic)
        throw runtime_error("Invalid archive magic");
    if (header.version != kArchiveVersion)
        throw runtime_error("Unsupported archive version");

    auto available = data.size() - sizeof(header);
    if (header.num_entries > available / sizeof(ArchiveEntry))
        throw runtime_error("Archive file too small for table");

    auto entries = vector<ArchiveEntry>(header.num_entries);
    memcpy(entries.data(), data.data() + sizeof(ArchiveHeader), sizeof(ArchiveEntry) * entries.size());

    for (const auto &entry : entries)
        if (entry.offset > data.size() || entry.size > data.size() - entry.offset)
            throw runtime_error("Archive entry out of bounds");

    data_ = std::move(data);
    entries_ = std::move(entries);
}
} // namespace euphrosyne
