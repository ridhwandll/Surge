// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include <optick.h>
#define PROFILE_SURGE 1

#if PROFILE_SURGE
#define SURGE_PROFILE_FRAME(...) OPTICK_FRAME(__VA_ARGS__)
#define SURGE_PROFILE_FUNC(...) OPTICK_EVENT(__VA_ARGS__)
#define SURGE_PROFILE_TAG(NAME, ...) OPTICK_TAG(NAME, __VA_ARGS__)
#define SURGE_PROFILE_THREAD(...) OPTICK_THREAD(__VA_ARGS__)
#else
#define SURGE_PROFILE_FRAME(...)
#define SURGE_PROFILE_FUNC()
#define SURGE_PROFILE_TAG(NAME, ...)
#define SURGE_PROFILE_THREAD(...)
#endif