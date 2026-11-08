#pragma once
#include <euphrosyne/pch.hpp>
#include <filesystem>

namespace cooker
{
using namespace euphrosyne;
namespace fs = std::filesystem;
vector<uint8> CompileHlsl(const fs::path &path);
} // namespace cooker