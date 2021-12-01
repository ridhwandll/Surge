// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Client.hpp"
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/Window/Window.hpp"
#include "Surge/Graphics/RenderContext.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"

namespace Surge::Core
{
    struct CoreData
    {
        Client* SurgeClient = nullptr; // Provided by the User

        Window* SurgeWindow = nullptr;
        RenderContext* SurgeRenderContext = nullptr;
        Renderer* SurgeRenderer = nullptr;
        bool Running = false;

        Vector<std::function<void()>> FrameEndCallbacks;
    };

    void Initialize(Client* application);
    void Run();
    void Shutdown();

    void AddFrameEndCallback(const std::function<void()>& func); // FrameEndCallbacks are a way to accomplish some task at the very end of a frame

    Window* GetWindow();
    RenderContext* GetRenderContext();
    Renderer* GetRenderer();
    CoreData* GetData();
    Client* GetClient();

} // namespace Surge::Core
