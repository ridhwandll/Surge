// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Client.hpp"
#include "Surge/Graphics/Camera/EditorCamera.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Asset/AssetManager.hpp"

#define INITIAL_WIDTH 1280
#define INITIAL_HEIGHT 720

namespace Surge
{
    class Player : public Surge::Client
    {
    public:
        Player() = default;
        virtual ~Player() = default;

        virtual void OnInitialize() override;
        virtual void OnUpdate() override;
        virtual void OnEvent(Event& e) override;
        virtual void OnShutdown() override;
        virtual void ChangeRuntimeScene(AssetID sceneID) override;
    private:
        Ref<Scene> CreateRuntimeScene(AssetID sceneID); // Fresh copy of the scene asset, the asset itself is unloaded again
        void Resize(Uint width, Uint height);
        void OnImGuiRender();
        void OpenProject();
    private:
        Renderer* mRenderer;
        AssetManager* mAssetManager;
    };
} // namespace Surge