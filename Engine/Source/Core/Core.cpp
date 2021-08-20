// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Core.hpp"
#include "Clock.hpp"
#include "Window.hpp"

namespace Surge
{
    struct CoreData
    {
        Application* mApplication = nullptr;
        Scope<Window> mWindow = nullptr;
        bool mRunning = false;
    };

    static CoreData sCoreData;

    void Initialize(Application* application)
    {
        Clock::Start();

        sCoreData.mApplication = application;
        sCoreData.mWindow = Window::Create(1280, 720, "Surge Window");
        sCoreData.mApplication->OnInitialize();

        const Window& window = *sCoreData.mWindow;
        Log<LogSeverity::INFO>("Create {0} ({1}, {2})", window.GetTitle(), window.GetWidth(), window.GetHeight());

        sCoreData.mRunning = true;
    }

    void Run()
    {
        while (sCoreData.mRunning)
        {
            Clock::Update();
            sCoreData.mApplication->OnUpdate();
            sCoreData.mWindow->Update();
        }
    }

    void Shutdown()
    {
        sCoreData.mApplication->OnShutdown();
    }

    void Close()
    {
        sCoreData.mRunning = false;
    }
}
