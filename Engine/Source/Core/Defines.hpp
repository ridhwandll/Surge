// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

#ifdef SURGE_DLL_EXPORT
    #define SURGE_API __declspec(dllexport)
#else
    #define SURGE_API __declspec(dllimport)
#endif

// Type defines

typedef char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef long long i64;
typedef unsigned long long u64;

typedef float f32;
typedef double f64;

typedef bool b32;

// Utilities

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))