// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

namespace Surge
{
    class Application
    {
    public:
        Application() = default;
        virtual ~Application() = default;

        // Called once, when Engine initializes
        virtual void OnInitialize() {};

        // Called per frame
        virtual void OnUpdate() {};

        // Called at the engine shutdown
        virtual void OnShutdown() {};
    };
}
