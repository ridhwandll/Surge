// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Time/Clock.hpp"
#include <fmt/core.h>
#include <fmt/color.h>
#include <mutex>

namespace Surge
{
    enum class Severity
    {
        Trace = 0,
        Info,
        Debug,
        Warn,
        Error,
        Fatal
    };

    static std::mutex sLogMutex;

    //TODO (Rid): Support for logging in Files, getting the last 'x' number of messages, store messages in a buffer etc.
    template <Severity severity = Severity::Trace, typename... Args>
    void Log(const char* format, const Args&... args)
    {
        sLogMutex.lock();
        time_t now = time(0);
        tm* ltm = localtime(&now);
        switch (severity)
        {
        case Surge::Severity::Trace:
        {
            fmt::print("[TRACE - {0}:{1}:{2}] ", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
            fmt::print(format, args...);
            break;
        }
        case Surge::Severity::Info:
            fmt::print(fg(fmt::color::lawn_green), "[INFO  - {0}:{1}:{2}] ", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
            fmt::print(fg(fmt::color::lawn_green) | fmt::emphasis::bold, format, args...);
            break;
        case Surge::Severity::Debug:
            fmt::print(fg(fmt::color::aqua), "[DEBUG - {0}:{1}:{2}] ", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
            fmt::print(fg(fmt::color::aqua) | fmt::emphasis::bold, format, args...);
            break;
        case Surge::Severity::Warn:
            fmt::print(fg(fmt::color::yellow), "[WARN  - {0}:{1}:{2}] ", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
            fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold | fmt::emphasis::italic, format, args...);
            break;
        case Surge::Severity::Error:
            fmt::print(fg(fmt::color::red), "[ERROR - {0}:{1}:{2}] ", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
            fmt::print(fg(fmt::color::red) | fmt::emphasis::bold | fmt::emphasis::italic, format, args...);
            break;
        case Surge::Severity::Fatal:
            fmt::print(bg(fmt::color::red), "[FATAL - {0}:{1}:{2}] ", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
            fmt::print(fg(fmt::color::antique_white) | bg(fmt::color::red) | fmt::emphasis::underline | fmt::emphasis::italic, format, args...);
            break;
        default:
            break;
        }
        std::putc('\n', stdout);
        sLogMutex.unlock();
    }
}