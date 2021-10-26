// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Logger/Logger.hpp"
#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Platform detection
#ifdef _WIN32
#define SURGE_WINDOWS
#elif __APPLE__
#define SURGE_APPLE
#elif __linux__
#define SURGE_LINUX
#endif

#ifdef SURGE_DEBUG
#define ASSERT() __debugbreak()
#define SG_ASSERT(condition, ...)                            \
    {                                                        \
        if (!(condition))                                    \
        {                                                    \
            Surge::Log<Surge::Severity::Fatal>(__VA_ARGS__); \
            ASSERT();                                        \
        }                                                    \
    }
#define SG_ASSERT_NOMSG(condition)                                   \
    {                                                                \
        if (!(condition))                                            \
        {                                                            \
            Surge::Log<Surge::Severity::Fatal>("Assertion Failed!"); \
            ASSERT();                                                \
        }                                                            \
    }
#define SG_ASSERT_INTERNAL(...)                          \
    {                                                    \
        Surge::Log<Surge::Severity::Fatal>(__VA_ARGS__); \
        ASSERT();                                        \
    }
#define SCOPED_TIMER(...) Timer tImEr(fmt::format(__VA_ARGS__), true)
#else
#define ASSERT()
#define SG_ASSERT(...)
#define SG_ASSERT_NOMSG(...)
#define SG_ASSERT_INTERNAL(...)
#define SCOPED_TIMER(...) Timer tImEr(fmt::format(__VA_ARGS__), true)
#endif

#define BIT(x) (1 << x)
#define FORCEINLINE __forceinline

#define MAKE_BIT_ENUM(type)                                                                                                    \
    FORCEINLINE type operator|(type a, type b) { return static_cast<type>(static_cast<int>(a) | static_cast<int>(b)); }        \
    FORCEINLINE type& operator|=(type& a, type b) { return a = static_cast<type>(static_cast<int>(a) | static_cast<int>(b)); } \
    FORCEINLINE bool operator&(type a, type b) { return static_cast<bool>(static_cast<int>(a) & static_cast<int>(b)); }

namespace Surge
{
    // Type defines
    using String = std::string;
    using Uint = uint32_t;
    using byte = uint8_t;

    // TODO(Rid): Have a dedicated path class
    using Path = std::string;

    template <typename T>
    using Vector = std::vector<T>;

    template <typename T>
    using Deque = std::deque<T>;

    template <typename T>
    using Scope = std::unique_ptr<T>;
    template <typename T, typename... Args>
    constexpr Scope<T> CreateScope(Args&&... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template <typename T1, typename T2>
    using HashMap = std::unordered_map<T1, T2>;

    template <typename T1, typename T2>
    struct Pair
    {
        T1 Data1;
        T2 Data2;
    };
} // namespace Surge
