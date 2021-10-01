// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Application.hpp"
#include "Surge/Core/Window/Window.hpp"
#include "Surge/Graphics/RenderContext.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"

namespace Surge
{
    // Initializes the whole Engine, along with it's subsystems, everything! - only call this once, at startup
    void Initialize(Application* application);

    // Runs the engine, performs a GIANT while loop
    void Run();

    // Cleans up all allocated resources, shutdowns internal systems - only call it at the program shutdown
    void Shutdown();

    // Ends the engine loop, returns from "Run"
    void Close();

    // Core Getters
    Scope<Window>& CoreGetWindow();

    Scope<RenderContext>& CoreGetRenderContext();

    Scope<Renderer>& CoreGetRenderer();
}
