// Copyright (c) - SurgeTechnologies - All rights reserved
#define IMGUI_DEFINE_MATH_OPERATORS
#include "Panels/Titlebar.hpp"
#include "Surge/Core/Core.hpp"
#include "Utility/ImGUIAux.hpp"
#include "Editor.hpp"
#include <imgui.h>
#include <imgui_internal.h>
#include <IconsFontAwesome.hpp>

namespace Surge
{
    Titlebar::Titlebar()
    {
        mEditor = SurgeCore::GetApplication<Editor>();
    }

    void Titlebar::Render()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.0f, 8.0f));

        // Start drawing window
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowBgAlpha(1.0f);
        ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y));
        ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, GetHeight()));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, {0.12f, 0.12f, 0.12f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_Button, {0.12f, 0.12f, 0.12f, 1.0f});
        if (ImGui::Begin("##dummy", NULL, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDecoration))
        {
            if (ImGui::SmallButton("View"))
                ImGui::OpenPopup("ViewPopup");

            float windowWidth = ImGui::GetWindowSize().x;
            float textWidth = ImGui::CalcTextSize(ICON_SURGE_PLAY).x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            SceneState sceneState = mEditor->GetSceneState();
            if (sceneState == SceneState::Edit)
            {
                if (ImGui::Button(ICON_SURGE_PLAY))
                {
                    mEditor->OnRuntimeStart();
                }
            }
            else if (sceneState == SceneState::Play)
            {
                if (ImGui::Button(ICON_SURGE_STOP))
                {
                    mEditor->OnRuntimeEnd();
                }
            }

            // System buttons
            {
                Window* window = SurgeCore::GetWindow();
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                const float buttonSize = ImGui::GetFrameHeight();
                const float iconMargin = buttonSize * 0.33;
                const float buttonOffsetFromWindowBorder = 10;
                const ImVec2 windowPos = ImGui::GetWindowPos();
                const ImVec2 windowSize = ImGui::GetWindowSize();
                ImRect buttonRect {};
                buttonRect.Min = windowPos + ImVec2(ImGui::GetWindowWidth() - buttonSize, buttonOffsetFromWindowBorder);
                buttonRect.Max = windowPos + windowSize - ImVec2(0, 20);
                buttonRect.TranslateX(-buttonOffsetFromWindowBorder);

                if (ImGui::BeginPopup("ViewPopup"))
                {
                    Editor* editor = SurgeCore::GetApplication<Editor>();
                    PanelManager& panelManager = editor->GetPanelManager();

                    for (auto& [code, element] : panelManager.GetAllPanels())
                    {
                        if (ImGui::MenuItem(PanelCodeToString(code)))
                            element.Show = !element.Show;
                    }

                    ImGui::EndMenu();
                }

                // Exit button
                {
                    bool hovered = false;
                    bool held = false;
                    bool pressed = ImGui::ButtonBehavior(buttonRect, ImGui::GetID("EXIT"), &hovered, &held, 0);

                    if (hovered)
                    {
                        const ImU32 color = ImGui::GetColorU32(ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                        drawList->AddRectFilled(buttonRect.Min, buttonRect.Max, color, 1.0f);
                    }

                    // Render the cross
                    {
                        const ImU32 color = ImGui::GetColorU32({1.0f, 1.0f, 1.0f, 1.0f});
                        drawList->AddLine(buttonRect.Min + ImVec2(iconMargin, iconMargin), buttonRect.Max - ImVec2(iconMargin, iconMargin), color, 1.5f);
                        drawList->AddLine(buttonRect.Min + ImVec2(buttonRect.GetWidth() - iconMargin, iconMargin), buttonRect.Max - ImVec2(buttonRect.GetWidth() - iconMargin, iconMargin), color, 1.5f);
                    }

                    if (pressed)
                        SurgeCore::Close();

                    buttonRect.Min.x -= buttonSize;
                    buttonRect.Max.x -= buttonSize;
                }

                // Maximize button
                {
                    bool hovered = false;
                    bool held = false;
                    bool pressed = ImGui::ButtonBehavior(buttonRect, ImGui::GetID("MAXIMIZE"), &hovered, &held, 0);

                    if (hovered)
                    {
                        const ImU32 color = ImGui::GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
                        drawList->AddRectFilled(buttonRect.Min, buttonRect.Max, color, 1.0f);
                    }

                    // Render the box
                    {
                        const ImU32 color = ImGui::GetColorU32({1.0f, 1.0f, 1.0f, 1.0f});
                        drawList->AddRect(buttonRect.Min + ImVec2(iconMargin, iconMargin), buttonRect.Max - ImVec2(iconMargin, iconMargin), color, 0.0f, 0, 1.2f);
                    }

                    if (pressed)
                    {
                        if (!window->IsWindowMaximized())
                            window->Maximize();
                        else
                            window->RestoreFromMaximize();
                    }

                    buttonRect.Min.x -= buttonSize;
                    buttonRect.Max.x -= buttonSize;
                }

                // Minimize button
                {
                    bool hovered = false;
                    bool held = false;
                    bool pressed = ImGui::ButtonBehavior(buttonRect, ImGui::GetID("MINIMIZE"), &hovered, &held, 0);

                    if (hovered)
                    {
                        const ImU32 color = ImGui::GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
                        drawList->AddRectFilled(buttonRect.Min, buttonRect.Max, color, 1.0f);
                    }

                    // Render the Line
                    {
                        const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
                        drawList->AddLine(buttonRect.Min + ImVec2(iconMargin, buttonRect.GetHeight() / 2), buttonRect.Max - ImVec2(iconMargin, buttonRect.GetHeight() / 2), color, 1.2f);
                    }

                    if (pressed)
                        window->Minimize();

                    buttonRect.Min.x -= buttonSize;
                    buttonRect.Max.x -= buttonSize;
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
    }

} // namespace Surge
