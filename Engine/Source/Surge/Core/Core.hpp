// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Application.hpp"
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Window/Window.hpp"
#include "Surge/Graphics/RenderContext.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"

namespace Surge
{
    struct CoreData
    {
        Application* SurgeApplication = nullptr; // Provided by the User

        Window* SurgeWindow = nullptr;
        RenderContext* SurgeRenderContext = nullptr;
        Renderer* SurgeRenderer = nullptr;
        bool mRunning = false;

        Vector<std::function<void()>> FrameEndCallbacks;
    };

    class SurgeCore
    {
    public:
        static void Initialize(Application* application);
        static void Run();
        static void Shutdown();
        static void Close();
        static void* GetApplication();

        // FrameEndCallbacks are a way to accomplish some task at the very end of a frame
        static void AddFrameEndCallback(const std::function<void()>& func);

        static Window* GetWindow();
        static RenderContext* GetRenderContext();
        static Renderer* GetRenderer();
        static CoreData* GetData();
    };

} // namespace Surge
