#include "logging.hpp"
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/FileSink.h>

namespace euphrosyne
{

quill::Logger *NewLogger(string_view name, quill::LogLevel log_level, LogDestination log_destination)
{
    quill::Backend::start();

    auto logger = (quill::Logger *)nullptr;
    switch (log_destination)
    {
    case LogDestination::Console: {
        logger = quill::Frontend::create_or_get_logger(
            string(name), quill::Frontend::create_or_get_sink<quill::ConsoleSink>(string(name) + "_sink"));
        break;
    }
    case LogDestination::File: {
        auto file_sink_config = quill::FileSinkConfig();
        file_sink_config.set_open_mode('w');
        file_sink_config.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
        auto file_sink =
            quill::Frontend::create_or_get_sink<quill::FileSink>(string(name) + ".log", std::move(file_sink_config));
        logger = quill::Frontend::create_or_get_logger(string(name), std::move(file_sink));
        break;
    }
    }

    logger->set_log_level(log_level);

#ifndef NDEBUG
    logger->set_immediate_flush(1u);
#endif

    return logger;
}
} // namespace euphrosyne
