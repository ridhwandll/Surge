// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Core/Core.hpp"
#include "Utility/ImGuiAux.hpp"
#include <imgui.h>

namespace Surge
{
    void ImGuiAux::Image(const Ref<Image2D>& image, const glm::vec2& size)
    {
        void* imguiTextureId = SurgeCore::GetRenderContext()->GetImGuiTextureID(image);
        ImGui::Image(imguiTextureId, {size.x, size.y});
    }

    void ImGuiAux::TextCentered(const char* text)
    {
        float windowWidth = ImGui::GetWindowSize().x;
        float textWidth = ImGui::CalcTextSize(text).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::Text(text);
    }

} // namespace Surge
