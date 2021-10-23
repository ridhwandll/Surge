// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Panels/PerformancePanel.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Graphics/RenderContext.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include <imgui.h>

namespace Surge
{
    void PerformancePanel::Init(void* panelInitArgs)
    {
        mCode = GetStaticCode();
    }

    void PerformancePanel::Render(bool* show)
    {
        if (!*show)
            return;

        if (ImGui::Begin(PanelCodeToString(mCode), show))
        {
            RenderContext* renderContext = SurgeCore::GetRenderContext();

            ImGui::Text("Device: %s", renderContext->GetGPUInfo().Name.c_str());
            ImGui::Text("Frame Time: % .2f ms ", Clock::GetMilliseconds());
            ImGui::Text("FPS: % .2f", ImGui::GetIO().Framerate);

            if (ImGui::CollapsingHeader("GPU Memory Status"))
            {
                Surge::GPUMemoryStats memoryStatus = renderContext->GetMemoryStatus();
                float used = memoryStatus.Used / 1000000.0f;
                float free = memoryStatus.Free / 1000000.0f;
                ImGui::Text("Used: %f Mb", used);
                ImGui::Text("Local-Free: %f Mb", free);
                ImGui::Text("Total Allocated: %f Mb", used + free);
            }

            if (ImGui::CollapsingHeader("Shaders"))
            {
                Vector<Ref<Shader>>& allAhaders = SurgeCore::GetRenderer()->GetData()->ShaderSet.GetAllShaders();
                if (ImGui::BeginTable("ShaderTable", 2, ImGuiTableFlags_BordersOuter | ImGuiTableFlags_Resizable))
                {
                    for (Ref<Shader>& shader : allAhaders)
                    {
                        ImGui::PushID(shader->GetPath().c_str());
                        ImGui::TableNextColumn();
                        ImGui::TextUnformatted(Filesystem::GetNameWithExtension(shader->GetPath()).c_str());
                        ImGui::TableNextColumn();
                        if (ImGui::Button("Reload"))
                            shader->Reload();
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
            }
        }
        ImGui::End();
    }

    void PerformancePanel::Shutdown()
    {
    }

} // namespace Surge
