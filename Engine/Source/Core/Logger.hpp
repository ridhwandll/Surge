// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Core/Clock.hpp"
#include <fmt/core.h>
#include <fmt/color.h>

namespace Surge
{
    enum class LogSeverity
    {
        TRACE = 0,
        INFO,
        DEBUG,
        WARN,
        ERROR,
        FATAL
    };

    //TODO (Rid): Support for logging in Files, getting the last 'x' number of messages, store messages in a buffer etc.
    template <LogSeverity severity, typename... Args>
    void Log(const char* format, const Args&... args)
    {
        switch (severity)
        {
        case Surge::LogSeverity::TRACE:
            fmt::print("[TRACE - {0}] ", Clock::GetLife());
            fmt::print(format, args...);
            break;
        case Surge::LogSeverity::INFO:
            fmt::print(fg(fmt::color::lawn_green), "[INFO  - {0}] ", Clock::GetLife());
            fmt::print(fg(fmt::color::lawn_green) | fmt::emphasis::bold, format, args...);
            break;
        case Surge::LogSeverity::DEBUG:
            fmt::print(fg(fmt::color::aqua), "[DEBUG - {0}] ", Clock::GetLife());
            fmt::print(fg(fmt::color::aqua) | fmt::emphasis::bold, format, args...);
            break;
        case Surge::LogSeverity::WARN:
            fmt::print(fg(fmt::color::yellow), "[WARN  - {0}] ", Clock::GetLife());
            fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold | fmt::emphasis::italic, format, args...);
            break;
        case Surge::LogSeverity::ERROR:
            fmt::print(fg(fmt::color::red), "[ERROR - {0}] ", Clock::GetLife());
            fmt::print(fg(fmt::color::red) | fmt::emphasis::bold | fmt::emphasis::italic, format, args...);
            break;
        case Surge::LogSeverity::FATAL:
            fmt::print(bg(fmt::color::red), "[FATAL - {0}] ", Clock::GetLife());
            fmt::print(fg(fmt::color::antique_white) | bg(fmt::color::red) | fmt::emphasis::underline | fmt::emphasis::italic, format, args...);
            break;
        default:
            break;
        }
        std::putc('\n', stdout);
    }
}