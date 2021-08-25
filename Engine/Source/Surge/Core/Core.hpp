// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Application.hpp"
#include "Surge/Core/Window/Window.hpp"

namespace Surge
{
    // Initializes the whole Engine, along with it's subsystems, everything! - only call this once, at startup
    SURGE_API void Initialize(Application* application);

    // Runs the engine, performs a GIANT while loop
    SURGE_API void Run();

    // Cleans up all allocated resources, shutdowns internal systems - only call it at the program shutdown
    SURGE_API void Shutdown();

    // Ends the engine loop, returns from "Run"
    SURGE_API void Close();

    // Ends the engine loop, returns from "Run"
    SURGE_API Scope<Window>& GetWindow();
}
