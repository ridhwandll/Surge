// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Viewport/EditorViewport.hpp"
#include "Surge/Graphics/Camera/EditorCamera.hpp"
#include "Picking/ScenePicker.hpp"

namespace Surge
{
    struct EditorSettings;
    struct ViewportSettings;
    class Entity;

    // Free editor camera with gizmos, the transform manipulator and click selection
    class SceneViewport final : public EditorViewport
    {
    public:
        SceneViewport(Uint index, const EditorCamera& camera);

        static void SetupManipulatorStyle(); // ImGuizmo look (global), once at startup

        virtual void OnEvent(Event& e) override;
        virtual void UpdateViewCamera(Scene& scene) override;

        // Before the scene update: only the viewport that owns the mouse interaction moves its camera
        void UpdateCamera(bool ownsInput);

        EditorCamera& GetCamera() { return mCamera; }
        bool IsManipulating() const { return mGizmoInUse; }

        void ApplySettings(const ViewportSettings& settings);
        void CaptureSettings(ViewportSettings& settings) const;

    protected:
        virtual void OnViewportGUI(const ImVec2& imageMin, const ImVec2& imageSize) override;

    private:
        void DrawToolbar(EditorSettings& settings);
        void HandleSelectionClick(Entity& selectedEntity, bool manipulatorActive, const ImVec2& imageMin, const ImVec2& imageSize);

    private:
        EditorCamera mCamera;
        bool mGizmoInUse = false;

        // Click selection, a press only selects when released without dragging
        ScenePicker mPicker;
        ImVec2 mPickPressPos = { 0.0f, 0.0f };
        bool mPickPending = false;
    };

} // namespace Surge
