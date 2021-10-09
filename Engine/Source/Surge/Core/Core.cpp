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

namespace Surge
{
    static CoreData sCoreData;

    void OnEvent(Event& e)
    {
        SurgeCore::GetData()->SurgeApplication->OnEvent(e);
        Surge::EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Surge::WindowResizeEvent>([](Surge::WindowResizeEvent& e) {
            if (SurgeCore::GetWindow()->GetWindowState() != WindowState::Minimized)
                SurgeCore::GetData()->SurgeRenderContext->OnResize();
        });
    }

    void SurgeCore::Initialize(Application* application)
    {
        SCOPED_TIMER("Initialization");
        Clock::Start();
        sCoreData.SurgeApplication = application;
        const ApplicationOptions& appOptions = sCoreData.SurgeApplication->GetAppOptions();

        // Window
        sCoreData.SurgeWindow = new WindowsWindow({1280, 720, "Surge", WindowFlags::CreateDefault});
        sCoreData.SurgeWindow->RegisterEventCallback(OnEvent);

        // Render Context
        sCoreData.SurgeRenderContext = new VulkanRenderContext();
        sCoreData.SurgeRenderContext->Initialize(sCoreData.SurgeWindow, appOptions.EnableImGui);

        // Renderer
        sCoreData.SurgeRenderer = new VulkanRenderer();
        sCoreData.SurgeRenderer->Initialize();

        // Reflection Engine
        SurgeReflect::Registry::Initialize();

        sCoreData.mRunning = true;
        sCoreData.SurgeApplication->OnInitialize();
    }

    void SurgeCore::Run()
    {
        while (sCoreData.mRunning)
        {
            SURGE_PROFILE_FRAME("Frame");
            Clock::Update();
            sCoreData.SurgeWindow->Update();

            if (sCoreData.SurgeWindow->GetWindowState() != WindowState::Minimized)
            {
                sCoreData.SurgeRenderContext->BeginFrame();

                sCoreData.SurgeApplication->OnUpdate();
                if (sCoreData.SurgeApplication->GetAppOptions().EnableImGui)
                    sCoreData.SurgeApplication->OnImGuiRender();

                sCoreData.SurgeRenderContext->EndFrame();
            }
        }
    }

    void SurgeCore::Shutdown()
    {
        SCOPED_TIMER("Shutdown");

        SurgeReflect::Registry::Shutdown();

        sCoreData.SurgeApplication->OnShutdown();
        delete sCoreData.SurgeApplication;
        sCoreData.SurgeApplication = nullptr;

        sCoreData.SurgeRenderer->Shutdown();
        sCoreData.SurgeRenderContext->Shutdown();

        delete sCoreData.SurgeWindow;
        delete sCoreData.SurgeRenderer;
        delete sCoreData.SurgeRenderContext;
    }

    void SurgeCore::Close() { sCoreData.mRunning = false; }

    Window* SurgeCore::GetWindow() { return sCoreData.SurgeWindow; }
    RenderContext* SurgeCore::GetRenderContext() { return sCoreData.SurgeRenderContext; }
    Renderer* SurgeCore::GetRenderer() { return sCoreData.SurgeRenderer; }

    Surge::CoreData* SurgeCore::GetData()
    {
        return &sCoreData;
    }

} // namespace Surge
