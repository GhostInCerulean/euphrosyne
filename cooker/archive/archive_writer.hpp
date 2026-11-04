#pragma once
#include <euphrosyne/pch.hpp>
#include <map>

namespace cooker {
    using namespace euphrosyne;

    class ArchiveWriter {
        struct Item {
            string name;
            vector<uint8> data;
        };
        map<uint64, Item> items;
    public:
        void Add(string name, vector<uint8> data);
        uintsize Write(const filesystem::path& path) const;
    };
} // namespace cooker
