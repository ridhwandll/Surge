// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Input/Input.hpp"
#include "Utility/ImGuiAux.hpp"
#include "Editor.hpp"
#include <imgui_stdlib.h>

namespace Surge
{
    void ImGuiAux::DrawRectAroundWidget(const glm::vec4& color, float thickness, float rounding)
    {
        ImGuiContext& g = *GImGui;
        const ImRect& rect = (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_HasDisplayRect) ? g.LastItemData.DisplayRect : g.LastItemData.Rect;
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRect(rect.Min, rect.Max, ImGui::ColorConvertFloat4ToU32(ImVec4(color.x, color.y, color.z, color.w)), rounding, ImDrawCornerFlags_All, thickness);
    }

    void ImGuiAux::Image(const Ref<Image2D>& image, const glm::vec2& size)
    {
        ImGui::Image(Core::GetRenderContext()->GetImGuiTextureID(image), {size.x, size.y});
    }

    void ImGuiAux::TextCentered(const char* text)
    {
        float windowWidth = ImGui::GetWindowSize().x;
        float textWidth = ImGui::CalcTextSize(text).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::TextUnformatted(text);
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

    bool ImGuiAux::PropertyGridHeader(const String& name, bool openByDefault, const glm::vec2& size, bool spacing)
    {
        ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

        if (openByDefault)
            treeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;

        bool open = false;
        float framePaddingX = size.x;
        float framePaddingY = size.y;

        ImGuiAux::ScopedStyle headerRounding({ImGuiStyleVar_FrameRounding}, 0.0f);
        ImGuiAux::ScopedStyle headerPaddingAndHeight({ImGuiStyleVar_FramePadding}, ImVec2 {framePaddingX, framePaddingY});

        ImGui::PushID(name.c_str());
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        String uppercaseName = name;
        for (char& n : uppercaseName)
            n = toupper(n);
        open = ImGui::TreeNodeEx("##dummyId", treeNodeFlags, uppercaseName.c_str());
        ImGui::PopFont();
        ImGui::PopID();

        DrawRectAroundWidget({0.3f, 0.3f, 0.3f, 1.0f}, 0.2f, 0.1f);
        const float headerSpacingOffset = -(ImGui::GetStyle().ItemSpacing.y + 1.0f);
        if (!spacing)
        {
            if (!open)
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + headerSpacingOffset);
        }

        return open;
    }

    bool ImGuiAux::ButtonCentered(const char* title)
    {
        float windowWidth = ImGui::GetWindowSize().x;
        float textWidth = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        bool res = ImGui::Button(title);

        if (ImGui::IsItemHovered() || ImGui::IsItemActive())
            DrawRectAroundWidget(Colors::ThemeColor, 1.5f, 1.0f);

        return res;
    }

    void ImGuiAux::RenamingMechanism::Update(String& name, const std::function<void(const String& newName)>& onRenameEnd)
    {
        if (ImGui::IsWindowHovered() && Input::IsKeyPressed(Key::F2))
            mRenaming = true;

        if (mRenaming)
        {
            if (!name.empty())
            {
                mTempBuffer = name;
                mOldName = mTempBuffer;
                name.clear();
            }
            ImGui::SameLine();

            // Copy the name from mTempBuffer
            if (ImGui::InputText("##Txt", &mTempBuffer, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
            {
                mRenaming = false;
                name = mTempBuffer;
                onRenameEnd(mTempBuffer);
                mTempBuffer.clear();
                mOldName.clear();
            }
            ImGui::SetKeyboardFocusHere();
            ImGuiAux::DrawRectAroundWidget({0.1f, 0.3f, 1.0f, 1.0f}, 1.5f, 1.0f);

            // Revert to old name if user hits Escape
            if (Input::IsKeyPressed(Key::Escape))
            {
                mRenaming = false;
                name = mOldName;
                mOldName.clear();
                mTempBuffer.clear();
            }
        }
    }

} // namespace Surge