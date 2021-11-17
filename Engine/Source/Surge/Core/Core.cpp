// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Core/Core.hpp"
#include "Pch.hpp"
#include "Surge/Core/Input/Input.hpp"
#include "Surge/Core/Time/Clock.hpp"
#include "Surge/Core/Window/Window.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderer.hpp"
#include "Surge/Platform/Windows/WindowsWindow.hpp"
#include "Surge/Debug/Profiler.hpp"
#include "SurgeReflect/SurgeReflect.hpp"
#include "Thread/ThreadPool.hpp"

namespace Surge::Core
{
    struct CoreData
    {
        Client* SurgeApplication = nullptr; // Provided by the User

        Window* SurgeWindow = nullptr;
        RenderContext* SurgeRenderContext = nullptr;
        Renderer* SurgeRenderer = nullptr;
        bool Running = false;

        Vector<std::function<void()>> FrameEndCallbacks;
    };
    static CoreData GCoreData;

    void OnEvent(Event& e)
    {
        GCoreData.SurgeApplication->OnEvent(e);
        Surge::EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Surge::WindowResizeEvent>([](Surge::WindowResizeEvent& e) { if (GetWindow()->GetWindowState() != WindowState::Minimized) GCoreData.SurgeRenderContext->OnResize(); });
        dispatcher.Dispatch<Surge::AppClosedEvent>([](Surge::AppClosedEvent& e) { GCoreData.Running = false; });
        dispatcher.Dispatch<Surge::WindowClosedEvent>([](Surge::WindowClosedEvent& e) { GCoreData.Running = false; });
    }

    void Initialize(Client* application)
    {
        SCOPED_TIMER("Core::Initialize");
        SG_ASSERT(application, "Invalid Application!");

        Clock::Start();
        GCoreData.SurgeApplication = application;
        const ClientOptions& appOptions = GCoreData.SurgeApplication->GetAppOptions();

        // Window
        GCoreData.SurgeWindow = new WindowsWindow({1280, 720, "Surge", WindowFlags::CreateDefault});
        GCoreData.SurgeWindow->RegisterEventCallback(OnEvent);

        // Render Context
        GCoreData.SurgeRenderContext = new VulkanRenderContext();
        GCoreData.SurgeRenderContext->Initialize(GCoreData.SurgeWindow, appOptions.EnableImGui);

        // Renderer
        GCoreData.SurgeRenderer = new VulkanRenderer();
        GCoreData.SurgeRenderer->Initialize();

        // Reflection Engine
        SurgeReflect::Registry::Initialize();

        GCoreData.Running = true;
        GCoreData.SurgeApplication->OnInitialize();
    }

    void Core::Run()
    {
        while (GCoreData.Running)
        {
            SURGE_PROFILE_FRAME("Core::Frame");
            Clock::Update();
            GCoreData.SurgeWindow->Update();

            if (GCoreData.SurgeWindow->GetWindowState() != WindowState::Minimized)
            {
                GCoreData.SurgeRenderContext->BeginFrame();
                GCoreData.SurgeApplication->OnUpdate();
                if (GCoreData.SurgeApplication->GetAppOptions().EnableImGui)
                    GCoreData.SurgeApplication->OnImGuiRender();
                GCoreData.SurgeRenderContext->EndFrame();

                if (!GCoreData.FrameEndCallbacks.empty())
                {
                    for (std::function<void()>& function : GCoreData.FrameEndCallbacks)
                        function();

                    GCoreData.FrameEndCallbacks.clear();
                }
            }
        }
    }

    void Core::Shutdown()
    {
        SCOPED_TIMER("Core::Shutdown");

        // NOTE(Rid): Order Matters here
        GCoreData.SurgeApplication->OnShutdown();
        delete GCoreData.SurgeApplication;
        GCoreData.SurgeApplication = nullptr;

        GCoreData.SurgeRenderer->Shutdown();
        delete GCoreData.SurgeRenderer;
        delete GCoreData.SurgeWindow;
        GCoreData.SurgeRenderContext->Shutdown();
        delete GCoreData.SurgeRenderContext;
        SurgeReflect::Registry::Shutdown();
    }

    void Core::AddFrameEndCallback(const std::function<void()>& func)
    {
        GCoreData.FrameEndCallbacks.push_back(func);
    }

    Window* GetWindow() { return GCoreData.SurgeWindow; }
    RenderContext* GetRenderContext() { return GCoreData.SurgeRenderContext; }
    Renderer* GetRenderer() { return GCoreData.SurgeRenderer; }
    CoreData* GetData() { return &GCoreData; }
    Surge::Client* GetClient() { return GCoreData.SurgeApplication; }
} // namespace Surge::Core