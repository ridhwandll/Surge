// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <deque>

#ifdef SURGE_DLL_EXPORT
    #define SURGE_API __declspec(dllexport)
#else
    #define SURGE_API __declspec(dllimport)
#endif

// Utilities
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

// Platform detection
#ifdef _WIN32
    #define SURGE_WIN32
#elif __APPLE__
    #define SURGE_APPLE
#elif __linux__
    #define SURGE_LINUX
#endif

namespace Surge
{
    // Type defines
    using String = std::string;
    using Uint = uint32_t;
    using byte = uint8_t;

    template<typename T>
    using Vector = std::vector<T>;

    template<typename T>
    using Deque = std::deque<T>;

    template<typename T>
    using Scope = std::unique_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Scope<T> CreateScope(Args&& ... args) { return std::make_unique<T>(std::forward<Args>(args)...); }

    template<typename T>
    using Ref = std::shared_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Ref<T> CreateRef(Args&& ... args) { return std::make_shared<T>(std::forward<Args>(args)...); }

    template<typename T1, typename T2>
    struct Pair
    {
        T1 Data1;
        T2 Data2;
    };
}
