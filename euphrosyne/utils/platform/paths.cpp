#include "paths.hpp"

#include "boost/dll/config.hpp"
#include "boost/dll/runtime_symbol_info.hpp"

namespace euphrosyne
{
// These two dont change during a run so they can be cached like that
const filesystem::path &ExecutablePath()
{
    static const auto path = filesystem::path(boost::dll::program_location().native());
    return path;
}

const filesystem::path &ExecutableDirectory()
{
    static const auto path = ExecutablePath().parent_path();
    return path;
}
} // namespace euphrosyne