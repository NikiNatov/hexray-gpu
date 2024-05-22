#pragma once

#include "core/core.h"

#include <spdlog/spdlog.h>

class Logger
{
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static void Initialize();

    static const std::shared_ptr<spdlog::logger>& GetLogger() { return ms_Logger; }
private:
    Logger() = default;
private:
    static std::shared_ptr<spdlog::logger> ms_Logger;
};

#define HEXRAY_TRACE(...)    Logger::GetLogger()->trace(__VA_ARGS__)
#define HEXRAY_INFO(...)     Logger::GetLogger()->info(__VA_ARGS__)
#define HEXRAY_WARNING(...)  Logger::GetLogger()->warn(__VA_ARGS__)
#define HEXRAY_ERROR(...)    Logger::GetLogger()->error(__VA_ARGS__)
#define HEXRAY_CRITICAL(...) Logger::GetLogger()->critical(__VA_ARGS__)