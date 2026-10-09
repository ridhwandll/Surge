// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/ECS/Scene.hpp"
#include "Gizmos/GizmoIcons.hpp"
#include <glm/glm.hpp>
#include <imgui.h>

namespace Surge
{
    class RuntimeCamera;

    struct GizmoSettings
    {
        bool ShowIcons = true;
        float IconSize = 30.0f;      // Pixels
        bool ShowWireForAll = false; // Frustums and light/audio ranges for every entity, otherwise only for the selection
        bool ShowCameraFrustums = true;
        bool ShowLightGizmos = true;
        bool ShowAudioGizmos = true;
        bool ShowSelectionOutline = true;
        bool ShowColliders = true;
    };

    // Editor-only scene visualization. Component icons and wire gizmos (camera frustums, light and audio ranges) are an ImGui overlay (ImGuizmo)
    class SceneGizmos
    {
    public:
        SceneGizmos() = default;
        ~SceneGizmos() = default;
        SURGE_DISABLE_COPY_AND_MOVE(SceneGizmos);

        void Shutdown();

        // (Rid) Called through Scene::Update(EditorCamera&, editorDraw), editorDraw callback, in Editor.cpp
        void SubmitSceneDebug(Scene& scene, Entity selected);

        // (Rid) Called before ImGuizmo so the manipulator stays on top, in ViewportPanel.cpp
        void DrawOverlay(Scene& scene, Entity selected, const glm::mat4& view, const glm::mat4& projection, float nearClip, const ImVec2& viewportMin, const ImVec2& viewportSize, bool allowHover);

        // Entity whose icon is under the mouse (nearest one wins), invalid when none. Valid after DrawOverlay() for the current frame
        Entity GetHoveredEntity() const { return mHoveredEntity; }

        void OnSettingsGUI();
        GizmoSettings& GetSettings() { return mSettings; }

    private:
        struct IconCmd
        {
            entt::entity Handle;
            ImVec2 ScreenPos;
            float Depth;
            GizmoIcon Icon;
            ImU32 Tint;
            bool Inactive;
        };

        // 3D -> viewport helpers, lines are clipped against the near plane so nothing behind the camera gets drawn
        bool ProjectPoint(const glm::vec3& worldPos, ImVec2& outScreen, float* outDepth = nullptr) const;
        float GetWorldPerPixel(const glm::vec3& worldPos) const;
        void DrawLine3D(const glm::vec3& a, const glm::vec3& b, ImU32 color, float thickness);
        void DrawPolyline3D(const glm::vec3* points, int count, bool closed, ImU32 color, float thickness);
        void DrawCircle3D(const glm::vec3& center, const glm::vec3& axisU, const glm::vec3& axisV, float radius, ImU32 color, float thickness);
        void DrawWireSphere(const glm::vec3& center, float radius, ImU32 color, float thickness);

        void DrawCameraFrustum(const glm::mat4& cameraWorld, const RuntimeCamera& camera, ImU32 color, float thickness);
        void DrawDirectionalLight(const glm::mat4& lightWorld, ImU32 color, float thickness);
        void DrawIcons(Scene& scene, entt::entity selectedHandle, bool allowHover);

    private:
        GizmoSettings mSettings;
        Vector<IconCmd> mIcons;
        Entity mHoveredEntity;

        // Per-overlay state
        ImDrawList* mDrawList = nullptr;
        glm::mat4 mViewProjection = glm::mat4(1.0f);
        glm::vec3 mCameraPosition = glm::vec3(0.0f);
        float mNearClip = 0.1f;
        float mProjectionScaleY = 1.0f;
        ImVec2 mViewportMin = { 0.0f, 0.0f };
        ImVec2 mViewportSize = { 0.0f, 0.0f };
    };
} // namespace Surge
