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
        Application* SurgeApplication = nullptr; // Provided by the User

        Scope<Window> SurgeWindow = nullptr;
        Scope<RenderContext> SurgeRenderContext = nullptr;
        Scope<Renderer> SurgeRenderer = nullptr;
        bool mRunning = false;
    };

    static CoreData sCoreData;

    void CoreOnEvent(Event& e)
    {
        sCoreData.SurgeApplication->OnEvent(e);
        Surge::EventDispatcher dispatcher(e);
        dispatcher.Dispatch<Surge::WindowResizeEvent>([](Surge::WindowResizeEvent& e)
            {
                if (CoreGetWindow()->GetWindowState() != WindowState::Minimized)
                    sCoreData.SurgeRenderContext->OnResize();
            });
    }

    void Initialize(Application* application)
    {
        SCOPED_TIMER("Initialization");
        Clock::Start();
        sCoreData.SurgeApplication = application;

        sCoreData.SurgeWindow = Window::Create({ 1280, 720, "Surge", WindowFlags::CreateDefault });
        sCoreData.SurgeWindow->RegisterEventCallback(Surge::CoreOnEvent);

        // Render Context
        sCoreData.SurgeRenderContext = RenderContext::Create();
        sCoreData.SurgeRenderContext->Initialize(sCoreData.SurgeWindow.get());

        // Renderer
        sCoreData.SurgeRenderer = CreateScope<Renderer>();
        sCoreData.SurgeRenderer->Initialize();

        sCoreData.SurgeApplication->OnInitialize();
        sCoreData.mRunning = true;
    }

    void Run()
    {
        while (sCoreData.mRunning)
        {
            Clock::Update();
            sCoreData.SurgeWindow->Update();

            if (sCoreData.SurgeWindow->GetWindowState() != WindowState::Minimized)
            {
                sCoreData.SurgeRenderContext->BeginFrame();
                sCoreData.SurgeApplication->OnImGuiRender();
                sCoreData.SurgeApplication->OnUpdate();
                sCoreData.SurgeRenderContext->EndFrame();
            }
        }
    }

    void Shutdown()
    {
        SCOPED_TIMER("Shutdown");

        sCoreData.SurgeApplication->OnShutdown();
        delete sCoreData.SurgeApplication;
        sCoreData.SurgeApplication = nullptr;

        sCoreData.SurgeRenderer->Shutdown();
        sCoreData.SurgeRenderContext->Shutdown();
    }

    void Close()
    {
        sCoreData.mRunning = false;
    }

    Scope<RenderContext>& CoreGetRenderContext()
    {
        return sCoreData.SurgeRenderContext;
    }

    Scope<Window>& CoreGetWindow()
    {
        return sCoreData.SurgeWindow;
    }

    Scope<Renderer>& CoreGetRenderer()
    {
        return sCoreData.SurgeRenderer;
    }
}
