// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Client.hpp"
#include <functional>

namespace Surge
{
    class Clock;
    class Window;
    class Physics;
    class Renderer;
    class AssetManager;
    class ScriptEngine;
    class AudioEngine;

    namespace Core
    {
        void Initialize(Client* application);
        void Run();
        void Shutdown();
        void SetFrameCap(float fps);

        // Game time scale: scales the physics step accumulation and the delta time scripts get (0 = frozen, 1 = normal)
        void SetTimeScale(float scale);
        float GetTimeScale();
        void AddFrameEndCallback(const std::function<void()>& func); // FrameEndCallbacks are a way to accomplish some task at the very end of a frame

        Window* GetWindow();
        Physics* GetPhysics();
        Renderer* GetRenderer();
        AssetManager* GetAssetManager();
        ScriptEngine* GetScriptEngine();
        AudioEngine* GetAudioEngine();
        Client* GetClient();
        Clock& GetClock();

        String GetEngineBuildConfiguration();
    }

} // namespace Surge::Core
