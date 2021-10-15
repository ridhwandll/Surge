// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Image.hpp"
#include <glm/glm.hpp>

namespace Surge::ImGuiAux
{
    void DrawRectAroundWidget(const glm::vec4& color, float thickness, float rounding);
    void DockSpace();

    void TextCentered(const char* text);
    void Image(const Ref<Image2D>& image, const glm::vec2& size);

} // namespace Surge::ImGuiAux