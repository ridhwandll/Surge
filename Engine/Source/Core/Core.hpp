// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Defines.hpp"
#include "Application.hpp"

namespace Surge
{
    // Initializes the whole Engine, along with it's subsystems, everything! - only call this once, at startup
    SURGE_API void Initialize(Application* application);

    // Runs the engine, performes a GIANT while loop
    SURGE_API void Run();

    // Cleans up all allocated resources, shutdowns internal systems - only call it at the program shutdown
    SURGE_API void Shutdown();

    // Ends the engine loop, returns from "Run"
    SURGE_API void Close();

    // Very important function
    SURGE_API void Crash();
}

#define SURGE_BASIC_APP(app)   \
::Surge::Initialize(&app);     \
::Surge::Run();                \
::Surge::Shutdown();           \
