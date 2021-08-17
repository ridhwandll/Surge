// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Core.hpp"
#include "Clock.hpp"

namespace Surge
{
    struct CoreData
    {
        Application* mApplication = nullptr;
        bool mRunning = false;
    };

    static CoreData sCoreData;

    void Initialize(Application* application)
    {
        Clock::Start();

        sCoreData.mRunning = true;
        sCoreData.mApplication = application;
        sCoreData.mApplication->OnInitialize();
    }

    void Run()
    {
        while (sCoreData.mRunning)
        {
            Clock::Update();
            sCoreData.mApplication->OnUpdate();
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
