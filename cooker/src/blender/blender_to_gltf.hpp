#pragma once
#include <euphrosyne/pch.hpp>

namespace cooker
{
using namespace euphrosyne;

void BlenderToGltf(asio::io_context& ioc, const filesystem::path& in, const filesystem::path& out);
}