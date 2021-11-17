// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include <Surge/Surge.hpp>
#include "Panels/Titlebar.hpp"
#include "Panels/PaneManager.hpp"

namespace Surge
{
    enum class SceneState
    {
        Edit,
        Play
    };

    class Editor : public Surge::Client
    {
    public:
        virtual ~Editor() = default;

        virtual void OnInitialize() override;
        virtual void OnUpdate() override;
        virtual void OnImGuiRender() override;
        virtual void OnEvent(Event& e) override;
        virtual void OnShutdown() override;

        // Editor specific
        void OnRuntimeStart();
        void OnRuntimeEnd();

        Ref<Scene>& GetEditorScene() { return mEditorScene; }
        SceneState GetSceneState() { return mSceneState; }
        PanelManager& GetPanelManager() { return mPanelManager; }
        Titlebar& GetTitlebar() { return mTitleBar; }
        EditorCamera& GetCamera() { return mCamera; }

    private:
        void Resize();

    private:
        EditorCamera mCamera;
        Renderer* mRenderer;

        Ref<Scene> mEditorScene, mRuntimeScene;

        Entity mEntity;
        Entity mOtherEntity;

        PanelManager mPanelManager;
        Titlebar mTitleBar {};
        SceneState mSceneState;
    };
} // namespace Surge