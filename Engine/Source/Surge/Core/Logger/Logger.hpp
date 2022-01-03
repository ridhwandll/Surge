// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include <fmt/color.h>
#include <fmt/core.h>
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

    // TODO (Rid): Support for logging in Files, getting the last 'x' number of messages, store messages in a buffer
    // etc.
    template <Severity severity = Severity::Trace, typename... Args>
    void Log(const char* format, const Args&... args)
    {
        sLogMutex.lock();
        time_t now = time(0);
        tm* ltm = localtime(&now);
        if constexpr (severity == Severity::Trace)
        {
            fmt::print("[{0}:{1}:{2}] ", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
            fmt::print(format, args...);
        }
        if constexpr (severity == Severity::Info)
        {
            fmt::print(fg(fmt::color::lawn_green), "[{0}:{1}:{2}] ", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
            fmt::print(fg(fmt::color::lawn_green) | fmt::emphasis::bold, format, args...);
        }
        if constexpr (severity == Severity::Debug)
        {
            fmt::print(fg(fmt::color::aqua), "[{0}:{1}:{2}] ", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
            fmt::print(fg(fmt::color::aqua) | fmt::emphasis::bold, format, args...);
        }
        if constexpr (severity == Severity::Warn)
        {
            fmt::print(fg(fmt::color::yellow), "[{0}:{1}:{2}] ", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
            fmt::print(fg(fmt::color::yellow) | fmt::emphasis::bold | fmt::emphasis::italic, format, args...);
        }
        if constexpr (severity == Severity::Error)
        {
            fmt::print(fg(fmt::color::red), "[{0}:{1}:{2}] ", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
            fmt::print(fg(fmt::color::red) | fmt::emphasis::bold | fmt::emphasis::italic, format, args...);
        }
        if constexpr (severity == Severity::Fatal)
        {
            fmt::print(bg(fmt::color::red), "[{0}:{1}:{2}] ", ltm->tm_hour, ltm->tm_min, ltm->tm_sec);
            fmt::print(fg(fmt::color::antique_white) | bg(fmt::color::red) | fmt::emphasis::underline | fmt::emphasis::italic, format, args...);
        }
        std::putc('\n', stdout);
        sLogMutex.unlock();
    }

} // namespace Surge
