#pragma once

#include <boost/asio/awaitable.hpp>
#include <boost/unordered/unordered_flat_map.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <flux.hpp>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <string>
#include <type_traits>
#include <vector>

namespace euphrosyne
{
using namespace std;
namespace asio = boost::asio;
using boost::unordered_flat_map;
using boost::asio::awaitable;

using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using uintsize = std::size_t;

using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;
using intsize = std::ptrdiff_t;
} // namespace euphrosyne