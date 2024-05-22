#include "logger.h"

#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>

std::shared_ptr<spdlog::logger> Logger::ms_Logger;

// ------------------------------------------------------------------------------------------------------------------------------------
void Logger::Initialize()
{
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sink->set_pattern("%^[%T] %n: %v%$");

    ms_Logger = std::make_shared<spdlog::logger>("HexRay", sink);
    ms_Logger->set_level(spdlog::level::trace);
}
