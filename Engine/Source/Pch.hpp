// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Logger/Logger.hpp"
#include "Surge/Core/Time/Timer.hpp"
#include "Surge/Debug/Profiler.hpp"

#include <string>
#include <memory>
#include <vector>
#include <array>

#ifdef SURGE_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif