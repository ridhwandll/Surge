// Copyright (c) - SurgeTechnologies - All rights reserved
#define IMGUI_DEFINE_MATH_OPERATORS
#include "Panels/Titlebar.hpp"
#include "Surge/Core/Core.hpp"
#include "Utility/ImGUIAux.hpp"
#include "Editor.hpp"
#include <imgui.h>
#include <imgui_internal.h>

namespace Surge
{
    void Titlebar::Render()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.0f, 8.0f));

        // Start drawing window
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowBgAlpha(1.0f);
        if (ImGui::BeginMainMenuBar())
        {
            // System buttons
            {
                Window* window = SurgeCore::GetWindow();
                const float buttonSize = ImGui::GetFrameHeight();
                float iconMargin = buttonSize * 0.35f;
                ImRect buttonRect = ImRect(ImGui::GetWindowPos() + ImVec2(ImGui::GetWindowWidth() - buttonSize, 0.0f), ImGui::GetWindowPos() + ImGui::GetWindowSize());
                ImDrawList* drawList = ImGui::GetWindowDrawList();

                ImGui::Text("Surge Editor");
                ImGui::SameLine();
                if (ImGui::BeginMenu("View"))
                {
                    Editor* editor = static_cast<Editor*>(SurgeCore::GetApplication());
                    PanelManager& panelManager = editor->GetPanelManager();

                    for (auto& [code, element] : panelManager.GetAllPanels())
                    {
                        if (ImGui::MenuItem(PanelCodeToString(code)))
                            element.Show = !element.Show;
                    }

                    ImGui::EndMenu();
                }
                ImGui::SameLine();

                // Exit button
                {
                    bool hovered = false;
                    bool held = false;
                    bool pressed = ImGui::ButtonBehavior(buttonRect, ImGui::GetID("EXIT"), &hovered, &held, 0);

                    if (hovered)
                    {
                        const ImU32 color = ImGui::GetColorU32(ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
                        drawList->AddRectFilled(buttonRect.Min, buttonRect.Max, color);
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
                        drawList->AddRectFilled(buttonRect.Min, buttonRect.Max, color);
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
                        drawList->AddRectFilled(buttonRect.Min, buttonRect.Max, color);
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
            ImGui::EndMainMenuBar();
        }
        ImGui::PopStyleVar();
    }

} // namespace Surge
