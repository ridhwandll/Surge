// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Image.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

namespace Surge::ImGuiAux
{
    enum class CustomProprtyFlag
    {
        None,
        Color3,
        Color4
    };

    void DrawRectAroundWidget(const glm::vec4& color, float thickness, float rounding);
    void DockSpace();

    void TextCentered(const char* text);
    void Image(const Ref<Image2D>& image, const glm::vec2& size);

    template <typename T, CustomProprtyFlag F = CustomProprtyFlag::None>
    FORCEINLINE bool Property(const char* title, T& value)
    {
        ImGui::PushID(title);
        bool result = false;
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(title);
        ImGui::TableNextColumn();

        if constexpr (F == CustomProprtyFlag::None)
        {
            if constexpr (std::is_same_v<T, float>)
                result = ImGui::DragFloat("##v", &value);
            else if constexpr (std::is_same_v<T, glm::vec2>)
                result = ImGui::DragFloat2("##v", glm::value_ptr(value), 0.1f, 0.0f, 0.0f, "%.2f");
            else if constexpr (std::is_same_v<T, glm::vec3>)
                result = ImGui::DragFloat3("##v", glm::value_ptr(value), 0.1f, 0.0f, 0.0f, "%.2f");
            else if constexpr (std::is_same_v<T, glm::vec4>)
                result = ImGui::DragFloat4("##v", glm::value_ptr(value), 0.1f, 0.0f, 0.0f, "%.2f");
            else if constexpr (std::is_same_v<T, bool>)
                result = ImGui::Checkbox("##v", &value);
        }
        else if constexpr (F == CustomProprtyFlag::Color3 && std::is_same_v<T, glm::vec3>)
            result = ImGui::ColorEdit3("##v", glm::value_ptr(value));
        else if constexpr (F == CustomProprtyFlag::Color4 && std::is_same_v<T, glm::vec4>)
            result = ImGui::ColorEdit4("##v", glm::value_ptr(value));
        else
            static_assert(false, "Invalid case! Maybe you used wrong CustomProprtyFlag with wrong type? For example: Using glm::vec3 with CustomProprtyFlag::Color4");

        if (ImGui::IsItemHovered() || ImGui::IsItemActive())
            DrawRectAroundWidget({1.0f, 0.5f, 0.1f, 1.0f}, 1.5f, 1.0f);
        ImGui::PopID();

        return result;
    }

    FORCEINLINE bool Button(const char* title, const char* buttonText)
    {
        ImGui::PushID(title);
        bool result = false;
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(title);
        ImGui::TableNextColumn();

        result = ImGui::Button(buttonText);

        if (ImGui::IsItemHovered() || ImGui::IsItemActive())
            DrawRectAroundWidget({1.0f, 0.5f, 0.1f, 1.0f}, 1.5f, 1.0f);
        ImGui::PopID();

        return result;
    }
} // namespace Surge::ImGuiAux