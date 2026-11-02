#pragma once
#include <euphrosyne/pch.hpp>
#include <map>

namespace cooker {
    using namespace euphrosyne;

    class ShaderArchiveWriter {
        struct Item {
            string name;
            vector<uint8> data;
        };
        map<uint64, Item> items;
    public:
        void Add(string name, vector<uint8> data);
        void Write(const filesystem::path& path) const;
    };
} // namespace cooker
