// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Utility/ImGuiUtils.hpp"
#include <imgui.h>

namespace Surge
{
    void ImGuiUtils::Image(const Ref<Image2D>& image, const glm::vec2& size)
    {
        void* imguiTextureId = SurgeCore::GetRenderContext()->GetImGuiTextureID(image);
        ImGui::Image(imguiTextureId, {size.x, size.y});
    }

} // namespace Surge
