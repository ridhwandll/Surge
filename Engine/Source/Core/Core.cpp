// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Core.hpp"
#include "Clock.hpp"
#include "Window.hpp"
#include "Input.hpp"

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
