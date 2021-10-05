// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Events/Event.hpp"

namespace Surge
{
    struct ApplicationOptions
    {
        bool EnableImGui = true;
    };

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

        void SetAppOptions(const ApplicationOptions& appCreateInfo) { mAppOptions = appCreateInfo; }
        const ApplicationOptions& GetAppOptions() const { return mAppOptions; }

    private:
        ApplicationOptions mAppOptions;
    };

} // namespace Surge
