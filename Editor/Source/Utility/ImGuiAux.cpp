// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Core/Core.hpp"
#include "Utility/ImGuiAux.hpp"
#include "Editor.hpp"
#include <imgui_internal.h>

namespace Surge
{
    void ImGuiAux::DrawRectAroundWidget(const glm::vec4& color, float thickness, float rounding)
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        const ImRect& rect = (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HasDisplayRect) ? g.LastItemData.DisplayRect : g.LastItemData.Rect;
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRect(rect.Min, rect.Max, ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, color.w)), rounding, ImDrawCornerFlags_All, thickness);
    }

    void ImGuiAux::Image(const Ref<Image2D>& image, const glm::vec2& size)
    {
        void* imguiTextureId = Core::GetRenderContext()->GetImGuiTextureID(image);
        ImGui::Image(imguiTextureId, {size.x, size.y});
    }

    void ImGuiAux::TextCentered(const char* text)
    {
        float windowWidth = ImGui::GetWindowSize().x;
        float textWidth = ImGui::CalcTextSize(text).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::Text(text);
    }

    void ImGuiAux::DockSpace()
    {
        constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos({viewport->Pos.x, viewport->Pos.y + ((static_cast<Editor*>(Core::GetClient())->GetTitlebar().GetHeight()) - 20.0f)});
        ImGui::SetNextWindowSize({viewport->Size.x, viewport->Size.y});
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace", nullptr, windowFlags);
        ImGui::PopStyleVar(3);

        // DockSpace
        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 270.0f;
        ImGui::DockSpace(ImGui::GetID("MyDockSpace"), ImVec2(0.0f, ImGui::GetWindowHeight() - 60.0f));
        style.WindowMinSize.x = minWinSizeX;
        ImGui::End();
    }
} // namespace Surge