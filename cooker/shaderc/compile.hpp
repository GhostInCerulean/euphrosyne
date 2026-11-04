#pragma once
#include <filesystem>
#include <euphrosyne/pch.hpp>

namespace cooker {
using namespace euphrosyne;
namespace fs = std::filesystem;
    vector<uint8> CompileHlsl(const fs::path& path);
}