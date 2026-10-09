// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Panels/IPanel.hpp"
#include "Panels/SceneHierarchyPanel.hpp"
#include "Picking/ScenePicker.hpp"
#include <glm/glm.hpp>
#include <Imgui/imgui.h>

namespace Surge
{
    struct EditorSettings;
    class ViewportPanel : public IPanel
    {
    public:
        ViewportPanel() = default;
        virtual ~ViewportPanel() override = default;

        virtual void Init(void* panelInitArgs) override;
        virtual void OnEvent(Event& e) override;
        virtual void Render(bool* show) override;
        virtual void Shutdown() override;

        void OnSceneContextChanged() { SetSceneName(); }
        const glm::vec2& GetViewportSize() const { return mViewportSize; }
        bool IsViewportHovered() const { return mIsViewportHovered; }
        bool IsFocusedOrHovered() const { return mIsViewportFocused || mIsViewportHovered; }
    public:
        static PanelCode GetStaticCode() { return PanelCode::Viewport; }

        void SetSceneName();

    private:
        void HandleSelectionClick(Entity& selectedEntity, bool manipulatorActive, const ImVec2& viewportMin, const ImVec2& viewportSize);
        void DrawToolbar(EditorSettings& settings); // Gizmo mode/space/snapping + camera settings, next to the aspect combo

    private:
        PanelCode mCode;
        glm::vec2 mViewportSize = glm::vec2(0.0f);
        bool mGizmoInUse = false;
        bool mIsViewportHovered = false;
        bool mIsViewportFocused = false;

        // Click selection, a press only selects when released without dragging
        ScenePicker mPicker;
        ImVec2 mPickPressPos = { 0.0f, 0.0f };
        bool mPickPending = false;

        bool mIsFullscreen = false;
        bool mRestoreScreenPosBeforeFullscreen = false;
        ImGuiID mPreviousDockID = 0;

        SceneHierarchyPanel* mSceneHierarchy;
        String mSceneName;
        EditorCamera* mEditorCam;
    };
} // namespace Surge