// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Events/Event.hpp"

namespace Surge
{
    class Application
    {
    public:
        Application() = default;
        virtual ~Application() = default;

        virtual void OnInitialize() {};
        virtual void OnUpdate() {};
        virtual void OnEvent(Event& e) {};
        virtual void OnImGuiRender() {};
        virtual void OnShutdown() {};
    };
}
