// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Panels/EditorTitleBar.hpp"
#include "Editor.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Time/Clock.hpp"
#include "Surge/Asset/AssetManager.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Utility/Platform.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include "Utility/ImGuiAux.hpp"
#include "Gizmos/GizmoIcons.hpp"
#include "Console/EditorConsole.hpp"
#include "Panels/ConsolePanel.hpp"
#include "Panels/ContentBrowserPanel.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <mutex>

namespace Surge
{
    namespace
    {
        constexpr float SIDE_PADDING = 12.0f;
        constexpr float LOGO_SIZE = 22.0f;
        constexpr float ITEM_HEIGHT = 28.0f; // Menu buttons, console counters
        constexpr float ITEM_ROUNDING = 2.0f;
        constexpr float MENU_PADDING_X = 10.0f;
        constexpr float GROUP_GAP = 16.0f; // Between the left, center and right groups
        constexpr float WINDOW_BUTTON_WIDTH = 46.0f;

        constexpr float FONT_SIZE = 15.0f;
        constexpr float SMALL_FONT_SIZE = 14.0f;

        // Play controls: three buttons in one pill
        constexpr ImVec2 PLAY_BUTTON_SIZE = ImVec2(32.0f, 26.0f);
        constexpr float PLAY_GROUP_PADDING = 3.0f;
        constexpr float PLAY_BUTTON_SPACING = 2.0f;
        constexpr float PLAY_GROUP_WIDTH = PLAY_BUTTON_SIZE.x * 3.0f + PLAY_BUTTON_SPACING * 2.0f + PLAY_GROUP_PADDING * 2.0f;

        constexpr ImVec4 TEXT_COLOR = ImVec4(0.92f, 0.92f, 0.92f, 1.0f);
        constexpr ImVec4 DIM_TEXT_COLOR = ImVec4(0.62f, 0.62f, 0.62f, 1.0f);
        constexpr ImVec4 FAINT_TEXT_COLOR = ImVec4(0.38f, 0.38f, 0.38f, 1.0f);
        constexpr ImVec4 HOVER_COLOR = ImVec4(1.0f, 1.0f, 1.0f, 0.07f);
        constexpr ImVec4 ACTIVE_COLOR = ImVec4(1.0f, 1.0f, 1.0f, 0.11f);
        constexpr ImVec4 DIVIDER_COLOR = ImVec4(1.0f, 1.0f, 1.0f, 0.10f);
        constexpr ImVec4 BOTTOM_LINE_COLOR = ImVec4(0.16f, 0.16f, 0.16f, 1.0f); // Same as the status bar's top line
        constexpr ImVec4 WARN_COLOR = ImVec4(1.0f, 0.8f, 0.25f, 1.0f);
        constexpr ImVec4 ERROR_COLOR = ImVec4(1.0f, 0.4f, 0.45f, 1.0f);
        constexpr ImVec4 CLOSE_HOVER_COLOR = ImVec4(0.77f, 0.17f, 0.11f, 1.0f); // Windows 11 caption close button

        struct Menu
        {
            const char* Label;
            const char* PopupID;
        };
        constexpr Menu MENUS[] = { { "File", "##FileMenu" }, { "Edit", "##EditMenu" }, { "View", "##ViewMenu" }, { "Help", "##HelpMenu" } };

        void PushRegularFont(float size)
        {
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0], size);
        }

        void MenuHeader(const char* title)
        {
            {
                ImGuiAux::ScopedBoldFont font;
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", title);
            }
            ImGuiAux::StyledSeparator();
        }

        void VerticalDivider(ImDrawList* drawList, float x, float centerY)
        {
            drawList->AddLine(ImVec2(x, centerY - 8.0f), ImVec2(x, centerY + 8.0f), ImGui::GetColorU32(DIVIDER_COLOR));
        }
    } // namespace

    void EditorTitleBar::Init(Editor* editor)
    {
        mEditor = editor;
        mEngineLogo = editor->LoadEditorIcon("Editor/Assets/Textures/EngineLogo.png"); // Mipmapped, drawn well below its 200px source
    }

    void EditorTitleBar::Shutdown()
    {
        Core::GetRenderer()->GetRHI()->DestroyImage(mEngineLogo);
    }

    void EditorTitleBar::Render()
    {
        const bool projectOpen = mEditor->GetCurrentProject().IsValid();
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, HEIGHT));
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGuiAux::Colors::ExtraDark);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                                           ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        if(ImGui::Begin("##SurgeTitleBar", nullptr, flags))
        {
            mWindowPos = ImGui::GetWindowPos();
            mWidth = ImGui::GetWindowWidth();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const float centerY = mWindowPos.y + HEIGHT * 0.5f;

            DrawPlayModeAccent();

            const ImVec2 logoMin(mWindowPos.x + SIDE_PADDING, std::floor(centerY - LOGO_SIZE * 0.5f));
            drawList->AddImage(Core::GetRenderer()->GetRHI()->GetImGuiImage(mEngineLogo), logoMin, ImVec2(logoMin.x + LOGO_SIZE, logoMin.y + LOGO_SIZE));
            float leftX = SIDE_PADDING + LOGO_SIZE + 8.0f;
            float rightX = DrawWindowButtons() - 8.0f;

            if(!projectOpen)
            {
                ImGuiAux::ScopedBoldFont font(FONT_SIZE);
                constexpr const char* title = "Surge Editor";
                const ImVec2 titleSize = ImGui::CalcTextSize(title);
                drawList->AddText(ImVec2(mWindowPos.x + leftX + 2.0f, std::floor(centerY - titleSize.y * 0.5f)), ImGui::GetColorU32(TEXT_COLOR), title);
                leftX += titleSize.x + 2.0f + 14.0f;
                VerticalDivider(drawList, mWindowPos.x + leftX, centerY);

                PushRegularFont(FONT_SIZE);
                constexpr const char* subtitle = "Project Browser";
                drawList->AddText(ImVec2(mWindowPos.x + leftX + 14.0f, std::floor(centerY - ImGui::GetFontSize() * 0.5f)), ImGui::GetColorU32(DIM_TEXT_COLOR), subtitle);
                ImGui::PopFont();
            }
            else
            {
                leftX = DrawMenus(leftX);

                // Centered on the window, pushed right when the menus need the room, dropped when even that doesn't fit
                const float playX = std::max(std::floor((mWidth - PLAY_GROUP_WIDTH) * 0.5f), leftX + GROUP_GAP);
                float breadcrumbMaxX = rightX - GROUP_GAP;
                float rightMinX = leftX + GROUP_GAP;
                if(playX + PLAY_GROUP_WIDTH + GROUP_GAP <= rightX)
                {
                    DrawPlaybackControls(playX);
                    breadcrumbMaxX = playX - GROUP_GAP;
                    rightMinX = playX + PLAY_GROUP_WIDTH + GROUP_GAP;
                }

                // Right side, dropped one by one as the window narrows: the console counters first in line, then the frame stats
                rightX = DrawConsoleCounters(rightX, rightMinX);
                DrawFrameStats(rightX - 10.0f, rightMinX);

                DrawBreadcrumb(leftX + 6.0f, breadcrumbMaxX);
            }
        }
        ImGui::End();

        ImGui::PopStyleVar(4);
        ImGui::PopStyleColor();
    }

    void EditorTitleBar::DrawPlayModeAccent()
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 max(mWindowPos.x + mWidth, mWindowPos.y + HEIGHT);
        if(!mEditor->IsPlaying())
        {
            drawList->AddLine(ImVec2(mWindowPos.x, max.y - 1.0f), ImVec2(max.x, max.y - 1.0f), ImGui::GetColorU32(BOTTOM_LINE_COLOR));
            return;
        }

        // Play mode: a soft amber glow rising from an accent line along the bottom edge, breathing while running, steady while paused
        const bool paused = mEditor->IsPaused();
        ImVec4 accent = ImGuiAux::Colors::ThemeColor1;
        const float intensity = paused ? 1.0f : 0.75f + 0.25f * std::sin(static_cast<float>(ImGui::GetTime()) * 2.5f);

        const ImU32 glowTop = ImGui::GetColorU32(ImVec4(accent.x, accent.y, accent.z, 0.0f));
        const ImU32 glowBottom = ImGui::GetColorU32(ImVec4(accent.x, accent.y, accent.z, 0.16f * intensity));
        drawList->AddRectFilledMultiColor(mWindowPos, max, glowTop, glowTop, glowBottom, glowBottom);

        accent.w = intensity;
        drawList->AddRectFilled(ImVec2(mWindowPos.x, max.y - 2.0f), max, ImGui::GetColorU32(accent));
    }

    float EditorTitleBar::DrawMenus(float x)
    {
        bool anyOpen = false;
        for(const Menu& menu : MENUS)
            anyOpen |= ImGui::IsPopupOpen(menu.PopupID);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const float y = std::floor((HEIGHT - ITEM_HEIGHT) * 0.5f);
        float popupX[std::size(MENUS)];

        PushRegularFont(FONT_SIZE);
        for(size_t i = 0; i < std::size(MENUS); i++)
        {
            const Menu& menu = MENUS[i];
            const ImVec2 textSize = ImGui::CalcTextSize(menu.Label);

            ImGui::SetCursorPos(ImVec2(x, y));
            const bool pressed = ImGui::InvisibleButton(menu.Label, ImVec2(textSize.x + MENU_PADDING_X * 2.0f, ITEM_HEIGHT));
            const bool hovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);
            bool open = ImGui::IsPopupOpen(menu.PopupID);

            // Like an OS menu bar: once a menu is open, hovering the others switches to them. Clicking the open one closes it (click outside the popup)
            if(!open && (pressed || (hovered && anyOpen)))
            {
                ImGui::OpenPopup(menu.PopupID);
                open = true;
            }

            const ImVec2 min = ImGui::GetItemRectMin();
            const ImVec2 max = ImGui::GetItemRectMax();
            if(open || hovered)
                drawList->AddRectFilled(min, max, ImGui::GetColorU32(open ? ACTIVE_COLOR : HOVER_COLOR), ITEM_ROUNDING);
            drawList->AddText(ImVec2(min.x + MENU_PADDING_X, std::floor(min.y + (ITEM_HEIGHT - textSize.y) * 0.5f)), ImGui::GetColorU32(open || hovered ? TEXT_COLOR : DIM_TEXT_COLOR), menu.Label);

            popupX[i] = min.x;
            x = max.x - mWindowPos.x + 2.0f;
        }
        ImGui::PopFont();

        // Each popup opens right under its button
        const float popupY = mWindowPos.y + HEIGHT + 2.0f;
        ImGuiAux::StyledPopupVars::Push();
        ImGui::SetNextWindowPos(ImVec2(popupX[0], popupY));
        DrawFileMenu();
        ImGui::SetNextWindowPos(ImVec2(popupX[1], popupY));
        DrawEditMenu();
        ImGui::SetNextWindowPos(ImVec2(popupX[2], popupY));
        DrawViewMenu();
        ImGui::SetNextWindowPos(ImVec2(popupX[3], popupY));
        DrawHelpMenu();
        ImGuiAux::StyledPopupVars::Pop();

        return x;
    }

    void EditorTitleBar::DrawBreadcrumb(float x, float maxX)
    {
        if(maxX - x < 60.0f)
            return;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const float centerY = mWindowPos.y + HEIGHT * 0.5f;
        drawList->PushClipRect(ImVec2(mWindowPos.x + x, mWindowPos.y), ImVec2(mWindowPos.x + maxX, mWindowPos.y + HEIGHT), true);

        VerticalDivider(drawList, mWindowPos.x + x, centerY);
        float cursor = mWindowPos.x + x + 14.0f;

#ifdef SURGE_DEBUG
        {
            ImGuiAux::ScopedBoldFont font(SMALL_FONT_SIZE);
            const ImVec2 textSize = ImGui::CalcTextSize("DEBUG");
            const ImVec2 min(cursor, std::floor(centerY - textSize.y * 0.5f) - 3.0f);
            const ImVec2 max(min.x + textSize.x + 12.0f, min.y + textSize.y + 6.0f);
            const ImVec4 amber = ImGuiAux::Colors::ThemeColor1;
            drawList->AddText(ImVec2(min.x + 6.0f, min.y + 3.0f), ImGui::GetColorU32(amber), "DEBUG");
            cursor = max.x + 10.0f;
        }
#endif

        {
            ImGuiAux::ScopedBoldFont font(FONT_SIZE);
            const char* projectName = mEditor->GetCurrentProject().Name.c_str();
            const ImVec2 textSize = ImGui::CalcTextSize(projectName);
            drawList->AddText(ImVec2(cursor, std::floor(centerY - textSize.y * 0.5f)), ImGui::GetColorU32(TEXT_COLOR), projectName);
            cursor += textSize.x + 9.0f;
        }

        // Chevron between the project and the scene
        const ImVec2 chevron[] = { ImVec2(cursor, centerY - 4.0f), ImVec2(cursor + 4.0f, centerY), ImVec2(cursor, centerY + 4.0f) };
        drawList->AddPolyline(chevron, static_cast<int>(std::size(chevron)), ImGui::GetColorU32(FAINT_TEXT_COLOR), ImDrawFlags_None, 1.5f);
        cursor += 4.0f + 9.0f;

        const AssetID sceneID = mEditor->GetCurrentScene()->GetID();
        AssetManager* assetManager = Core::GetAssetManager();
        const String sceneName = sceneID && assetManager->IsRegistered(sceneID) ? Filesystem::GetFilenameWithoutExt(assetManager->GetMetadata(sceneID).RelativePath) : String("Untitled");
        PushRegularFont(FONT_SIZE);
        const ImVec2 sceneSize = ImGui::CalcTextSize(sceneName.c_str());
        drawList->AddText(ImVec2(cursor, std::floor(centerY - sceneSize.y * 0.5f)), ImGui::GetColorU32(DIM_TEXT_COLOR), sceneName.c_str());
        ImGui::PopFont();
        cursor += sceneSize.x;

        // Unsaved changes
        if(mEditor->GetHistory().IsDirty())
            drawList->AddCircleFilled(ImVec2(cursor + 9.0f, centerY + 1.0f), 3.5f, ImGui::GetColorU32(ImGuiAux::Colors::ThemeColor1));

        drawList->PopClipRect();
    }

    void EditorTitleBar::DrawPlaybackControls(float x)
    {
        const bool playing = mEditor->IsPlaying();
        const bool paused = mEditor->IsPaused();
        const ImVec4 amber = ImGuiAux::Colors::ThemeColor1;
        const ImVec4 selection = ImGuiAux::Colors::Selection;

        // The pill behind the buttons
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const float groupHeight = PLAY_BUTTON_SIZE.y + PLAY_GROUP_PADDING * 2.0f;
        const float groupY = std::floor((HEIGHT - groupHeight) * 0.5f);
        const ImVec2 groupMin(mWindowPos.x + x, mWindowPos.y + groupY);
        const ImVec2 groupMax(groupMin.x + PLAY_GROUP_WIDTH, groupMin.y + groupHeight);
        drawList->AddRectFilled(groupMin, groupMax, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.04f)), ITEM_ROUNDING + PLAY_GROUP_PADDING);
        drawList->AddRect(groupMin, groupMax, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.07f)), ITEM_ROUNDING + PLAY_GROUP_PADDING);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, HOVER_COLOR);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ACTIVE_COLOR);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, ITEM_ROUNDING);
        ImGui::SetCursorPos(ImVec2(x + PLAY_GROUP_PADDING, groupY + PLAY_GROUP_PADDING));

        // Play/Stop: shown pressed (selection gray) while running
        if(playing)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, selection);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(selection.x * 1.25f, selection.y * 1.25f, selection.z * 1.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(selection.x * 0.8f, selection.y * 0.8f, selection.z * 0.8f, 1.0f));
        }
        if(GizmoIcons::IconButton("##PlayStop", playing ? GizmoIcon::STOP : GizmoIcon::PLAY, PLAY_BUTTON_SIZE, playing ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : TEXT_COLOR, 0.55f))
        {
            if(playing)
                mEditor->OnRuntimeEnd();
            else
                mEditor->OnRuntimeStart();
        }
        if(playing)
            ImGui::PopStyleColor(3);
        ImGuiAux::ItemTooltip(playing ? "Stop (F5)" : "Play (F5)");

        if(!playing)
            ImGui::BeginDisabled();

        ImGui::SameLine(0.0f, PLAY_BUTTON_SPACING);
        if(paused)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(amber.x, amber.y, amber.z, 0.16f));
        if(GizmoIcons::IconButton("##PauseResume", paused ? GizmoIcon::PLAY : GizmoIcon::PAUSE, PLAY_BUTTON_SIZE, paused ? amber : TEXT_COLOR, 0.55f))
            mEditor->SetPaused(!paused);
        if(paused)
            ImGui::PopStyleColor();
        ImGuiAux::ItemTooltip(paused ? "Resume (F6)" : "Pause (F6)");

        ImGui::SameLine(0.0f, PLAY_BUTTON_SPACING);
        if(GizmoIcons::IconButton("##Step", GizmoIcon::STEP, PLAY_BUTTON_SIZE, TEXT_COLOR, 0.55f))
            mEditor->StepFrame();
        ImGuiAux::ItemTooltip("Step one frame, pauses first (F7)");

        if(!playing)
            ImGui::EndDisabled();

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
    }

    float EditorTitleBar::DrawFrameStats(float rightX, float minX)
    {
        mStatsTimer += ImGui::GetIO().DeltaTime;
        if(mStatsTimer >= 0.25f || mFpsText.empty())
        {
            mFrameTimeText = std::format("{:.2f} ms", Core::GetClock().GetMilliseconds());
            mFpsText = std::format("{:.0f} FPS", ImGui::GetIO().Framerate);
            mStatsTimer = 0.0f;
        }

        PushRegularFont(SMALL_FONT_SIZE);
        constexpr float gap = 8.0f;
        const ImVec2 frameTimeSize = ImGui::CalcTextSize(mFrameTimeText.c_str());
        const ImVec2 fpsSize = ImGui::CalcTextSize(mFpsText.c_str());
        const float width = frameTimeSize.x + gap + fpsSize.x;
        if(rightX - width < minX)
        {
            ImGui::PopFont();
            return rightX;
        }

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const float x = mWindowPos.x + rightX - width;
        const float y = std::floor(mWindowPos.y + (HEIGHT - frameTimeSize.y) * 0.5f);
        drawList->AddText(ImVec2(x, y), ImGui::GetColorU32(DIM_TEXT_COLOR), mFrameTimeText.c_str());
        drawList->AddText(ImVec2(x + frameTimeSize.x + gap, y), ImGui::GetColorU32(TEXT_COLOR), mFpsText.c_str());
        ImGui::PopFont();
        return rightX - width;
    }

    float EditorTitleBar::DrawConsoleCounters(float rightX, float minX)
    {
        EditorConsole& console = EditorConsole::Get();
        Uint warnings = 0, errors = 0;
        {
            std::lock_guard<std::mutex> lock(console.GetMutex());
            warnings = console.GetWarningCount();
            errors = console.GetErrorCount();
        }

        char warningText[16], errorText[16];
        std::snprintf(warningText, sizeof(warningText), "%u", warnings);
        std::snprintf(errorText, sizeof(errorText), "%u", errors);

        PushRegularFont(SMALL_FONT_SIZE);
        constexpr float paddingX = 8.0f;
        constexpr float iconSize = 12.0f;
        constexpr float iconGap = 5.0f;
        constexpr float counterGap = 12.0f;
        const ImVec2 warningSize = ImGui::CalcTextSize(warningText);
        const ImVec2 errorSize = ImGui::CalcTextSize(errorText);
        const float width = paddingX * 2.0f + (iconSize + iconGap) * 2.0f + warningSize.x + counterGap + errorSize.x;
        if(rightX - width < minX)
        {
            ImGui::PopFont();
            return rightX;
        }

        ImGui::SetCursorPos(ImVec2(rightX - width, std::floor((HEIGHT - ITEM_HEIGHT) * 0.5f)));
        if(ImGui::InvisibleButton("##ConsoleCounters", ImVec2(width, ITEM_HEIGHT)))
            mEditor->GetPanelManager().GetPanel<ConsolePanel>()->Focus();
        const bool hovered = ImGui::IsItemHovered();
        if(hovered)
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const ImVec2 min = ImGui::GetItemRectMin();
        const ImVec2 max = ImGui::GetItemRectMax();
        if(hovered)
            drawList->AddRectFilled(min, max, ImGui::GetColorU32(ImGui::IsItemActive() ? ACTIVE_COLOR : HOVER_COLOR), ITEM_ROUNDING);

        const float centerY = std::floor((min.y + max.y) * 0.5f);
        const ImU32 cutout = ImGui::GetColorU32(ImGuiAux::Colors::ExtraDark);
        float x = min.x + paddingX;

        // Warning: triangle with an exclamation mark carved out
        const ImU32 warningColor = ImGui::GetColorU32(warnings > 0 ? WARN_COLOR : FAINT_TEXT_COLOR);
        const float half = iconSize * 0.5f;
        drawList->AddTriangleFilled(ImVec2(x + half, centerY - half), ImVec2(x + iconSize, centerY + half - 0.5f), ImVec2(x, centerY + half - 0.5f), warningColor);
        drawList->AddLine(ImVec2(x + half, centerY - 2.0f), ImVec2(x + half, centerY + 1.5f), cutout, 1.5f);
        drawList->AddCircleFilled(ImVec2(x + half, centerY + 3.5f), 0.9f, cutout);
        x += iconSize + iconGap;
        drawList->AddText(ImVec2(x, std::floor(centerY - warningSize.y * 0.5f)), ImGui::GetColorU32(warnings > 0 ? TEXT_COLOR : FAINT_TEXT_COLOR), warningText);
        x += warningSize.x + counterGap;

        // Error: disc with a cross carved out
        const ImU32 errorColor = ImGui::GetColorU32(errors > 0 ? ERROR_COLOR : FAINT_TEXT_COLOR);
        const ImVec2 errorCenter(x + half, centerY);
        drawList->AddCircleFilled(errorCenter, half, errorColor);
        drawList->AddLine(ImVec2(errorCenter.x - 2.5f, errorCenter.y - 2.5f), ImVec2(errorCenter.x + 2.5f, errorCenter.y + 2.5f), cutout, 1.5f);
        drawList->AddLine(ImVec2(errorCenter.x + 2.5f, errorCenter.y - 2.5f), ImVec2(errorCenter.x - 2.5f, errorCenter.y + 2.5f), cutout, 1.5f);
        x += iconSize + iconGap;
        drawList->AddText(ImVec2(x, std::floor(centerY - errorSize.y * 0.5f)), ImGui::GetColorU32(errors > 0 ? TEXT_COLOR : FAINT_TEXT_COLOR), errorText);
        ImGui::PopFont();

        if(hovered)
            ImGuiAux::ItemTooltip(std::format("{} warnings, {} errors. Click to open the Console", warnings, errors).c_str());
        return rightX - width;
    }

    float EditorTitleBar::DrawWindowButtons()
    {
        enum class WindowButton
        {
            MINIMIZE,
            MAXIMIZE,
            CLOSE
        };
        constexpr WindowButton buttons[] = { WindowButton::MINIMIZE, WindowButton::MAXIMIZE, WindowButton::CLOSE };
        constexpr const char* ids[] = { "##Minimize", "##Maximize", "##Close" };

        Window* window = Core::GetWindow();
        const bool maximized = window->IsWindowMaximized();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const float startX = mWidth - WINDOW_BUTTON_WIDTH * static_cast<float>(std::size(buttons));

        for(size_t i = 0; i < std::size(buttons); i++)
        {
            const WindowButton button = buttons[i];
            const bool isClose = button == WindowButton::CLOSE;

            ImGui::SetCursorPos(ImVec2(startX + WINDOW_BUTTON_WIDTH * static_cast<float>(i), 0.0f));
            const bool pressed = ImGui::InvisibleButton(ids[i], ImVec2(WINDOW_BUTTON_WIDTH, HEIGHT));
            const bool hovered = ImGui::IsItemHovered();
            const bool held = ImGui::IsItemActive();
            const ImVec2 min = ImGui::GetItemRectMin();
            const ImVec2 max = ImGui::GetItemRectMax();

            if(hovered || held)
            {
                ImVec4 background = isClose ? CLOSE_HOVER_COLOR : (held ? ACTIVE_COLOR : HOVER_COLOR);
                if(isClose && held)
                    background.w = 0.8f;
                drawList->AddRectFilled(min, max, ImGui::GetColorU32(background));
            }

            // Thin Windows 11 style glyphs on whole pixels, ImDrawList adds the half pixel offset itself
            const ImU32 color = ImGui::GetColorU32((isClose && hovered) ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : TEXT_COLOR);
            const ImVec2 c(std::floor((min.x + max.x) * 0.5f), std::floor((min.y + max.y) * 0.5f));
            switch(button)
            {
                case WindowButton::MINIMIZE:
                    drawList->AddLine(ImVec2(c.x - 5.0f, c.y), ImVec2(c.x + 5.0f, c.y), color);
                    break;
                case WindowButton::MAXIMIZE:
                    if(maximized) // Restore: two overlapping squares
                    {
                        drawList->AddRect(ImVec2(c.x - 5.0f, c.y - 3.0f), ImVec2(c.x + 4.0f, c.y + 6.0f), color, 1.0f);
                        const ImVec2 back[] = { ImVec2(c.x - 3.0f, c.y - 3.0f), ImVec2(c.x - 3.0f, c.y - 5.0f), ImVec2(c.x + 5.0f, c.y - 5.0f), ImVec2(c.x + 5.0f, c.y + 3.0f), ImVec2(c.x + 3.0f, c.y + 3.0f) };
                        drawList->AddPolyline(back, static_cast<int>(std::size(back)), color, ImDrawFlags_None, 1.0f);
                    }
                    else
                        drawList->AddRect(ImVec2(c.x - 5.0f, c.y - 5.0f), ImVec2(c.x + 6.0f, c.y + 6.0f), color, 1.0f);
                    break;
                case WindowButton::CLOSE:
                    drawList->AddLine(ImVec2(c.x - 5.0f, c.y - 5.0f), ImVec2(c.x + 5.0f, c.y + 5.0f), color);
                    drawList->AddLine(ImVec2(c.x + 5.0f, c.y - 5.0f), ImVec2(c.x - 5.0f, c.y + 5.0f), color);
                    break;
            }

            if(!pressed)
                continue;

            switch(button)
            {
                case WindowButton::MINIMIZE:
                    window->Minimize();
                    break;
                case WindowButton::MAXIMIZE:
                    if(maximized)
                        window->RestoreFromMaximize();
                    else
                        window->Maximize();
                    break;
                case WindowButton::CLOSE:
                    mEditor->ConfirmDiscardChanges("Exit", []() { Platform::RequestExit(); });
                    break;
            }
        }
        return startX;
    }

    void EditorTitleBar::DrawFileMenu()
    {
        if(!ImGui::BeginPopup("##FileMenu"))
            return;

        MenuHeader("FILE");

        const bool playing = mEditor->IsPlaying();
        AssetManager* assetManager = Core::GetAssetManager();
        const Project& project = mEditor->GetCurrentProject();
        const AssetID currentSceneID = mEditor->GetCurrentScene()->GetID();
        const bool sceneIsAsset = currentSceneID && assetManager->IsRegistered(currentSceneID);

        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
        if(ImGuiAux::StyledMenuItem("New Scene", "Ctrl+N", nullptr, !playing))
            mEditor->NewScene();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
        if(ImGui::BeginMenu("Open Scene", !playing))
        {
            // Sorted by path, the registry map has no stable order
            Vector<Pair<String, AssetID>> scenes;
            for(const auto& [id, metadata] : assetManager->GetRegistryMap())
            {
                if(metadata.Type == AssetType::SCENE && !HasFlag(metadata.Flags, AssetFlags::MISSING))
                    scenes.push_back({ metadata.RelativePath, id });
            }
            std::sort(scenes.begin(), scenes.end(), [](const auto& a, const auto& b) { return a.Data1 < b.Data1; });

            if(scenes.empty())
                ImGui::TextDisabled("No scenes in the project");
            for(const auto& [path, id] : scenes)
            {
                const bool isCurrent = id == currentSceneID;
                const char* tag = isCurrent ? "OPEN" : (id == project.StartScene ? "START" : nullptr);
                if(ImGuiAux::StyledMenuItem(path.c_str(), tag, nullptr, !isCurrent))
                    mEditor->OpenScene(id);
            }
            ImGui::EndMenu();
        }
        ImGui::PopStyleVar();

        ImGuiAux::StyledSeparator();

        if(ImGuiAux::StyledMenuItem("Save Scene", "Ctrl+S", nullptr, sceneIsAsset))
            mEditor->SaveScene();
        if(ImGuiAux::StyledMenuItem("Save Scene As...", "Ctrl+Shift+S", nullptr, !playing))
            mEditor->OpenSaveSceneAsPopup();
        if(ImGuiAux::StyledMenuItem("Set as Start Scene", nullptr, nullptr, sceneIsAsset && currentSceneID != project.StartScene && !mEditor->GetProjectFilePath().empty()))
            mEditor->SetStartScene(currentSceneID);

        ImGuiAux::StyledSeparator();

        if(ImGuiAux::StyledMenuItem("Show Project in Explorer"))
            Platform::OpenFolderInExplorer(Filesystem::GetParentPath(mEditor->GetProjectFilePath()).string());
        if(ImGuiAux::StyledMenuItem("Close Project"))
            mEditor->CloseProject();
        if(ImGuiAux::StyledMenuItem("Exit"))
            mEditor->ConfirmDiscardChanges("Exit", []() { Platform::RequestExit(); });
        ImGui::PopStyleColor();

        ImGui::EndPopup();
    }

    void EditorTitleBar::DrawEditMenu()
    {
        if(!ImGui::BeginPopup("##EditMenu"))
            return;

        MenuHeader("EDIT");

        const bool playing = mEditor->IsPlaying();
        EditorHistory& history = mEditor->GetHistory();
        const String undoLabel = history.CanUndo() ? std::format("Undo {}", history.GetUndoName()) : String("Undo");
        const String redoLabel = history.CanRedo() ? std::format("Redo {}", history.GetRedoName()) : String("Redo");

        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
        if(ImGuiAux::StyledMenuItem(undoLabel.c_str(), "Ctrl+Z", nullptr, !playing && history.CanUndo()))
            mEditor->RequestUndo();
        if(ImGuiAux::StyledMenuItem(redoLabel.c_str(), "Ctrl+Y", nullptr, !playing && history.CanRedo()))
            mEditor->RequestRedo();

        ImGuiAux::StyledSeparator();

        Entity selected = mEditor->GetSelectedEntity();
        const bool hasSelection = (bool)selected;
        if(ImGuiAux::StyledMenuItem("Copy", "Ctrl+C", nullptr, hasSelection))
            mEditor->CopyEntity(selected);
        if(ImGuiAux::StyledMenuItem("Paste", "Ctrl+V", nullptr, !playing && mEditor->HasEntityInClipboard()))
            mEditor->PasteEntity();
        if(ImGuiAux::StyledMenuItem("Duplicate", "Ctrl+D", nullptr, hasSelection))
            mEditor->DuplicateEntity(selected);
        if(ImGuiAux::StyledMenuItem("Delete", "Del", nullptr, hasSelection))
            mEditor->DeleteEntity(selected);

        ImGuiAux::StyledSeparator();

        if(ImGuiAux::StyledMenuItem("Focus Selection", "F", nullptr, hasSelection))
            mEditor->FocusEntity(selected);
        ImGui::PopStyleColor();

        ImGui::EndPopup();
    }

    void EditorTitleBar::DrawViewMenu()
    {
        if(!ImGui::BeginPopup("##ViewMenu"))
            return;

        MenuHeader("EDITOR PANELS");

        PanelManager& panels = mEditor->GetPanelManager();
        ViewportManager& viewports = mEditor->GetViewports();
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
        for(auto& [panelCode, panel] : panels.GetAllPanels())
            ImGui::MenuItem(PanelCodeToString(panelCode), nullptr, &panel.Show);
        if(ImGuiAux::StyledMenuItem("Content Drawer", "Ctrl+Space"))
            panels.GetPanel<ContentBrowserPanel>()->ToggleDrawer();

        ImGuiAux::StyledSeparator();
        {
            ImGuiAux::ScopedBoldFont font;
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "VIEWPORTS");
        }
        viewports.DrawMenuItems();
        ImGuiAux::StyledSeparator();

        if(ImGui::MenuItem("Reset Layout"))
        {
            for(auto& [panelCode, panel] : panels.GetAllPanels())
                panel.Show = true;
            if(!viewports.GetActiveSceneView())
                viewports.AddSceneView();
            if(!viewports.GetGameView())
                viewports.OpenGameView();
            mEditor->RequestResetLayout(); // Rebuilt right before the dockspace next frame
        }
        ImGui::PopStyleColor();

        ImGui::EndPopup();
    }

    void EditorTitleBar::DrawHelpMenu()
    {
        if(!ImGui::BeginPopup("##HelpMenu"))
            return;

        MenuHeader("HELP");

        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
        if(ImGuiAux::StyledMenuItem("Keyboard Shortcuts"))
            mEditor->OpenShortcutsWindow();
        ImGui::PopStyleColor();

        ImGui::EndPopup();
    }

} // namespace Surge
