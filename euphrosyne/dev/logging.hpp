#pragma once
#include "euphrosyne/pch.hpp"

namespace euphrosyne
{
enum class LogDestination
{
    Console,
    File,
};

quill::Logger *NewLogger(string_view name, quill::LogLevel log_level, LogDestination log_destination);
} // namespace euphrosyne