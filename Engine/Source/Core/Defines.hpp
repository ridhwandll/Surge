// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

#ifdef SURGE_DLL_EXPORT
    #define SURGE_API __declspec(dllexport)
#else
    #define SURGE_API __declspec(dllimport)
#endif

// Type defines

typedef unsigned int Uint;

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