// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Core/String.hpp"
#include "Surge/Core/Events/Event.hpp"
#include "Surge/Graphics/Renderer/RenderView.hpp"
#include <glm/glm.hpp>
#include <imgui.h>

namespace Surge
{
    class Scene;

    enum class ViewportType : uint8_t
    {
        SCENE,
        GAME
    };

    // Dockable window showing one Renderer view
    class EditorViewport
    {
    public:
        EditorViewport(ViewportType type, Uint index);
        virtual ~EditorViewport();
        SURGE_DISABLE_COPY_AND_MOVE(EditorViewport);

        void Render();
        virtual void OnEvent(Event& e);

        virtual void UpdateViewCamera(Scene& scene) = 0;

        ViewportType GetType() const { return mType; }
        RenderViewID GetViewID() const { return mView; }
        Uint GetIndex() const { return mIndex; }
        const String& GetWindowName() const { return mWindowName; }
        bool IsOpen() const { return mOpen; }
        bool IsVisible() const { return mVisible; }
        bool IsHovered() const { return mHovered; }
        bool IsFocused() const { return mFocused; }
        const glm::vec2& GetSize() const { return mImageSize; } // Size of the image on screen, 0 while hidden
        ImGuiID GetDockID() const { return mDockID; }

        void SetInitialDock(ImGuiID dockID) { mInitialDockID = dockID; } // Only used if the window has no saved layout
        void RequestFocus() { mFocusRequested = true; }

        static String MakeWindowName(ViewportType type, Uint index); // "Scene###SceneView1", "Scene (2)###SceneView2"

    protected:
        // Part of the content region the image covers
        virtual void ComputeImageRect(const ImVec2& available, ImVec2& outOffset, ImVec2& outSize);
        virtual bool WantsRender() const { return true; }
        virtual void OnViewportGUI(const ImVec2& imageMin, const ImVec2& imageSize) = 0; // Overlays and interaction on top of the image

        void DrawCornerLabel(const char* text, const ImVec2& imageMin, const ImVec2& imageSize) const;

    protected:
        RenderViewID mView = INVALID_RENDER_VIEW;
        glm::vec2 mImageSize = { 0.0f, 0.0f };
        ImVec2 mContentMin = { 0.0f, 0.0f };  // Window content region, screen space
        ImVec2 mContentSize = { 0.0f, 0.0f };

    private:
        void HandleAssetDrop();

    private:
        ViewportType mType;
        Uint mIndex;
        String mWindowName;

        bool mOpen = true;
        bool mVisible = false;
        bool mHovered = false;
        bool mFocused = false;
        bool mFocusRequested = false;

        ImGuiID mDockID = 0;
        ImGuiID mInitialDockID = 0;

        bool mIsFullscreen = false;
        bool mRestoreDockAfterFullscreen = false;
        ImGuiID mDockBeforeFullscreen = 0;
    };

} // namespace Surge
