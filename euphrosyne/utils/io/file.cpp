#include "file.hpp"

#include <fstream>

namespace euphrosyne
{
vector<uint8_t> ReadFile(const filesystem::path &path)
{
    auto file = ifstream(path);
    if (!file.is_open())
        throw runtime_error("Failed to open file");

    return vector<uint8_t>(istreambuf_iterator(file), {});
}
} // euphrosyne