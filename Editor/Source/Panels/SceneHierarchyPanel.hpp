// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Panels/IPanel.hpp"
#include "Surge/ECS/Scene.hpp"

namespace Surge
{
    class SceneHierarchyPanel : public IPanel
    {
    public:
        SceneHierarchyPanel() = default;
        ~SceneHierarchyPanel() = default;

        virtual void Init(void* panelInitArgs) override;
        virtual void OnEvent(Event& e) override;
        virtual void Render(bool* show) override;
        virtual void Shutdown() override;

        static PanelCode GetStaticCode() { return PanelCode::SceneHierarchy; }

        void SetSceneContext(Scene* scene)
        {
            SG_ASSERT_NOMSG(scene);
            mSelectedEntity = {};
            mSceneContext = scene;
        }
        Scene* GetSceneContext() { return mSceneContext; }

        void SetSelectedEntity(Entity& e)
        {
            mSelectedEntity = e;
            mRevealSelection = true; // Selected from elsewhere (Viewport, undo, paste...), open its parents and scroll to it
        }
        Entity& GetSelectedEntity() { return mSelectedEntity; }
        bool IsFocusedOrHovered() const { return mHierarchyFocused || mHierarchyHovered; }

        void StartRename(Entity entity);

    private:
        void DrawEntityNode(Entity& e);
        void DrawRenameField(Entity& e);

    private:
        bool mHierarchyHovered = false;
        bool mHierarchyFocused = false;
        bool mRevealSelection = false;

        // Inline rename
        Entity mRenamingEntity;
        String mRenameBuffer;
        bool mFocusRename = false;
        int mRenameGraceFrames = 0; // Frames the field may take to become active before the rename is dropped
        PanelCode mCode;
        Scene* mSceneContext;
        Entity mSelectedEntity; // TODO: Make It a vector when we allow multiple selection
    };

} // namespace Surge