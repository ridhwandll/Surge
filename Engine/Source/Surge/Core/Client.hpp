// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/ECS/Scene.hpp"
#include "Surge/Core/Events/Event.hpp"
#include "Surge/Core/Window/Window.hpp"
#include "Surge/Core/Project.hpp"

namespace Surge
{
    struct ClientOptions
    {
        WindowDesc WindowDescription;
        bool EnableImGui = true;
        bool RenderFinalImageToSwapchian = true;
    };

    class Client
    {
    public:
        Client() = default;
        virtual ~Client() = default;

        virtual void OnInitialize() {};
        virtual void OnUpdate() {};
        virtual void OnEvent(Event& e) { (void)e; };
        virtual void OnShutdown() {};

        virtual Ref<Scene> GetRuntimeScene() const { return mActiveScene; }

        // Replaces the running scene with a fresh copy of the scene asset (Lua: Scene.Load). Called at the end of a frame,
        // never while a scene is updating. The default does nothing: clients without a runtime scene ignore it
        virtual void ChangeRuntimeScene(AssetID sceneID) { (void)sceneID; }

        const Project& GetProject() const { return mCurrentProject; }

        void SetOptions(const ClientOptions& appCreateInfo) { mClientOptions = appCreateInfo; }
        const ClientOptions& GetClientOptions() const { return mClientOptions; }

    protected:
        Project mCurrentProject;
        Ref<Scene> mActiveScene;

    private:
        ClientOptions mClientOptions;
    };

    template <typename T>
    T* MakeClient()
    {
        static_assert(std::is_base_of_v<Client, T>, "Class MUST derive from Surge::Client");
        T* client = new T();
        return client;
    }

} // namespace Surge
