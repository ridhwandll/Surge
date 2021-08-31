// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Time/Clock.hpp"
#include "Surge/Core/Window/Window.hpp"
#include "Surge/Core/Input/Input.hpp"

namespace Surge
{
    struct CoreData
    {
        Application* SurgeApplication = nullptr;
        Scope<Window> SurgeWindow = nullptr;
        Scope<RenderContext> SurgeRenderContext = nullptr;
        bool mRunning = false;
    };

    static CoreData sCoreData;

    void CoreOnEvent(Event& e)
    {
        sCoreData.SurgeApplication->OnEvent(e);

        Surge::EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Surge::WindowResizeEvent>([](Surge::WindowResizeEvent& e)
            {
                sCoreData.SurgeRenderContext->OnResize(e.GetWidth(), e.GetHeight());
            });
    }

    void Initialize(Application* application)
    {
        Clock::Start();
        sCoreData.SurgeApplication = application;

        sCoreData.SurgeWindow = Window::Create({ 1280, 720, "Surge", WindowFlags::CreateDefault });
        sCoreData.SurgeWindow->RegisterEventCallback(Surge::CoreOnEvent);

        sCoreData.SurgeRenderContext = RenderContext::Create();
        sCoreData.SurgeRenderContext->Initialize(sCoreData.SurgeWindow.get());

        sCoreData.SurgeApplication->OnInitialize();
        sCoreData.mRunning = true;
    }

    void Run()
    {
        while (sCoreData.mRunning)
        {
            Clock::Update();
            if (sCoreData.SurgeWindow->GetWindowState() != WindowState::Minimized)
            {
                sCoreData.SurgeApplication->OnUpdate();
            }
            sCoreData.SurgeWindow->Update();
        }
    }

    void Shutdown()
    {
        sCoreData.SurgeApplication->OnShutdown();
        sCoreData.SurgeRenderContext->Shutdown();
    }

    void Close()
    {
        sCoreData.mRunning = false;
    }

    SURGE_API Scope<RenderContext>& GetRenderContext()
    {
        return sCoreData.SurgeRenderContext;
    }

    SURGE_API Scope<Surge::Window>& GetWindow()
    {
        return sCoreData.SurgeWindow;
    }
}
