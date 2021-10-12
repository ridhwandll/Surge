// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include <Surge/Surge.hpp>
#include "Panels/Titlebar.hpp"
#include "Panels/PaneManager.hpp"

namespace Surge
{
    class Editor : public Application
    {
    public:
        virtual void OnInitialize() override;
        virtual void OnUpdate() override;
        virtual void OnImGuiRender() override;
        virtual void OnEvent(Event& e) override;
        virtual void OnShutdown() override;

    private:
        EditorCamera mCamera;
        Renderer* mRenderer;

        Ref<Scene> mScene;
        Entity mEntity;
        Entity mOtherEntity;

        PanelManager mPanelManager;
        // Panels //TODO: Have a panel manager, and automate all these stuff
        Titlebar mTitleBar;
    };

} // namespace Surge
