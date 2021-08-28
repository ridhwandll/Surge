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
        Application* mApplication = nullptr;
        Scope<Window> mWindow = nullptr;
        bool mRunning = false;
    };

    static CoreData sCoreData;

    void Initialize(Application* application)
    {
        Clock::Start();

        sCoreData.mApplication = application;
        sCoreData.mWindow = Window::Create({ 1280, 720, "Surge", WindowFlags::CreateDefault });
        sCoreData.mWindow->RegisterApplication(application);
        sCoreData.mApplication->OnInitialize();
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

    SURGE_API Scope<Surge::Window>& GetWindow()
    {
        return sCoreData.mWindow;
    }
}
