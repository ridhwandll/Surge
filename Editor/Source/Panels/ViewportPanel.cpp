// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Panels/ViewportPanel.hpp"
#include "Surge/Graphics/Image.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Hash.hpp"
#include "Utility/ImGUIAux.hpp"
#include "SurgeReflect/TypeTraits.hpp"
#include <imgui.h>

namespace Surge
{
    void ViewportPanel::Init(void* panelInitArgs)
    {
    }

    void ViewportPanel::Render(bool* show)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.0f, 0.0f});
        if (ImGui::Begin("Viewport", show))
        {
            const Ref<Image2D>& outputImage = SurgeCore::GetRenderer()->GetData()->OutputFrambuffer->GetColorAttachment(0);
            mViewportSize = {ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y};
            ImGuiAux::Image(outputImage, mViewportSize);
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ViewportPanel::Shutdown()
    {
    }

} // namespace Surge
