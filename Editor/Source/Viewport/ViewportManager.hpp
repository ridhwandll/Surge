// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Viewport/EditorViewport.hpp"
#include "Surge/Core/Scope.hpp"
#include "Surge/Core/Vector.hpp"

namespace Surge
{
    class EditorCamera;
    class SceneViewport;
    class GameViewport;
    struct ViewportSettings;

    // Owns the open viewports: any number of Scene views and at most one Game view
    class ViewportManager
    {
    public:
        ViewportManager() = default;
        ~ViewportManager() = default;
        SURGE_DISABLE_COPY_AND_MOVE(ViewportManager);

        void Restore(const Vector<ViewportSettings>& saved); // Project opened, empty: one Scene and one Game view
        void Capture(Vector<ViewportSettings>& outSaved) const;
        void Clear();                                        // Releases every viewport (and its GPU targets)

        void OnUpdate(); // Before the scene update: camera input
        void Render();   // Windows, closed viewports are released afterwards
        void OnEvent(Event& e);
        void UpdateViewCameras(Scene& scene); // From the scene's render hook (transforms are final)

        SceneViewport* AddSceneView();
        GameViewport* OpenGameView(); // Creates it if closed, focuses it otherwise

        void DrawMenuItems();                // View menu entries
        void DockWindows(ImGuiID dockNode);  // Default layout: every viewport tabbed into one node
        void OnPlay();                       // Brings the Game view to front
        void OnPlayStop();                   // Brings a scene view to front, if any
        void OnSceneContextChanged();

        SceneViewport* GetActiveSceneView() const { return mActiveScene; } // Last one hovered/focused, F and new Scene views use its camera
        GameViewport* GetGameView() const { return mGameView; }            // nullptr while closed
        bool IsSceneViewFocusedOrHovered() const;
        glm::vec2 GetGameResolution() const;                               // Game view size, the active Scene view's without one
        const String& GetSceneName() const { return mSceneName; }

    private:
        Uint NextFreeSceneIndex() const;
        ImGuiID GetNewViewportDock() const;
        void AddViewport(Scope<EditorViewport>&& viewport);

    private:
        Vector<Scope<EditorViewport>> mViewports;
        SceneViewport* mActiveScene = nullptr;
        GameViewport* mGameView = nullptr;
        SceneViewport* mCameraOwner = nullptr; // Scene view whose camera takes the mouse/keyboard (hovered, kept while a button is held)
        String mSceneName;
    };

} // namespace Surge
