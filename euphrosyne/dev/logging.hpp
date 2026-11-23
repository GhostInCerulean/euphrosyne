#pragma once
#include "euphrosyne/pch.hpp"

#define EUPHROSYNE_DECLARE_LOGGER(name) extern quill::Logger *name##_logger
#define EUPHROSYNE_DEFINE_LOGGER(name, level, dest) \
    quill::Logger *name##_logger = NewLogger(#name, level, dest)

#define ELOG_INFO(name, ...)  QUILL_LOG_INFO(name##_logger, __VA_ARGS__)
#define ELOG_WARN(name, ...)  QUILL_LOG_WARNING(name##_logger, __VA_ARGS__)
#define ELOG_ERROR(name, ...) QUILL_LOG_ERROR(name##_logger, __VA_ARGS__)
#define ELOG_DEBUG(name, ...) QUILL_LOG_DEBUG(name##_logger, __VA_ARGS__)

namespace euphrosyne
{
enum class LogDestination
{
    Console,
    File,
};

quill::Logger *NewLogger(string_view name, quill::LogLevel log_level, LogDestination log_destination);

constexpr quill::LogLevel DefaultLogLevel()
{
#ifndef NDEBUG
    return quill::LogLevel::Debug;
#else
    return quill::LogLevel::Info;
#endif
}

constexpr LogDestination DefaultLogDestination()
{
#ifndef NDEBUG
    return LogDestination::Console;
#else
    return LogDestination::File;
#endif
}
} // namespace euphrosyne