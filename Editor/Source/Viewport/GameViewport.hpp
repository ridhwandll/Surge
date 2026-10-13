// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Viewport/EditorViewport.hpp"

namespace Surge
{
    struct ViewportSettings;

    // The view through the scene's primary camera (the Player's screen): the in-game UI is laid out at its size, takes input in it and the UI Editor puts its handles on it
    class GameViewport final : public EditorViewport
    {
    public:
        GameViewport();

        virtual void UpdateViewCamera(Scene& scene) override;

        void ApplySettings(const ViewportSettings& settings);
        void CaptureSettings(ViewportSettings& settings) const;

    protected:
        virtual void ComputeImageRect(const ImVec2& available, ImVec2& outOffset, ImVec2& outSize) override;
        virtual bool WantsRender() const override { return mHasCamera; }
        virtual void OnViewportGUI(const ImVec2& imageMin, const ImVec2& imageSize) override;

    private:
        int mAspectMode = 0; // Index into ASPECT_PRESETS
        int mSafeAreaMode = 0; // Index into SAFE_AREA_PRESETS, previews UI::SafeArea without a device
        bool mHasCamera = false;
    };

} // namespace Surge
