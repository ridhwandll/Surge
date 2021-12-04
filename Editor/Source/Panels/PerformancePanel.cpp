// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Panels/PerformancePanel.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Graphics/RenderContext.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include <imgui.h>
#include "Editor.hpp"
#include "SceneHierarchyPanel.hpp"
#include "Surge/ECS/Components.hpp"
#include "Utility/ImGuiAux.hpp"
#include "Surge/Graphics/RenderProcedure/ShadowMapProcedure.hpp"

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
            RenderContext* renderContext = Core::GetRenderContext();
            const float headerSpacingOffset = -(ImGui::GetStyle().ItemSpacing.y + 1.0f);

            ImGui::Text("Device: %s", renderContext->GetGPUInfo().Name.c_str());
            ImGui::Text("Frame Time: % .2f ms ", Clock::GetMilliseconds());
            ImGui::Text("FPS: % .2f", ImGui::GetIO().Framerate);

            if (ImGuiAux::PropertyGridHeader("GPU Memory Status", false))
            {
                Surge::GPUMemoryStats memoryStatus = renderContext->GetMemoryStatus();
                float used = memoryStatus.Used / 1000000.0f;
                float free = memoryStatus.Free / 1000000.0f;
                ImGui::Text("Used: %f Mb", used);
                ImGui::Text("Local-Free: %f Mb", free);
                ImGui::Text("Total Allocated: %f Mb", used + free);
                ImGui::TreePop();
            }

            if (ImGuiAux::PropertyGridHeader("Shaders", false))
            {
                Vector<Ref<Shader>>& allAhaders = Core::GetRenderer()->GetData()->ShaderSet.GetAllShaders();
                if (ImGui::BeginTable("ShaderTable", 2, ImGuiTableFlags_Resizable))
                {
                    for (Ref<Shader>& shader : allAhaders)
                    {
                        ImGui::PushID(shader->GetPath().c_str());
                        if (ImGuiAux::TButton(Filesystem::GetNameWithExtension(shader->GetPath()).c_str(), "Reload"))
                            shader->Reload();
                        ImGui::PopID();
                    }
                    ImGui::EndTable();
                }
                ImGui::TreePop();
            }

            ShadowMapProcedure::InternalData* shadowProcInternalData = Core::GetRenderer()->GetRenderProcManager()->GetRenderProcData<ShadowMapProcedure>();
            static Uint selectedCascadeIndex = 0;
            if (ImGuiAux::PropertyGridHeader("Shadows", false))
            {
                if (ImGui::BeginTable("ShaderTable", 2, ImGuiTableFlags_Resizable))
                {
                    ImGuiAux::TProperty<bool>("Visualize Cascades", shadowProcInternalData->VisualizeCascades);
                    ImGuiAux::TProperty<float>("Cascade Split Lambda", shadowProcInternalData->CascadeSplitLambda);
                    int shadowMapResolution = (int)shadowProcInternalData->ShadowMapResolution;
                    if (ImGuiAux::TProperty<int>("Shadow Map Resolution", shadowMapResolution, 1024, 8192))
                    {
                        ShadowMapProcedure* shadowProc = Core::GetRenderer()->GetRenderProcManager()->GetProcedure<ShadowMapProcedure>();
                        shadowProc->ResizeShadowMaps(shadowMapResolution);
                    }
                    ImGui::EndTable();
                }

                if (ImGuiAux::PropertyGridHeader("Shadows", false, glm::vec2(1.0f, 1.0f), true))
                {
                    ImGui::SliderInt("##CascadeIndex", reinterpret_cast<int*>(&selectedCascadeIndex), 0, shadowProcInternalData->CascadeCount - 1);
                    ImGuiAux::Image(shadowProcInternalData->ShadowMapFramebuffers[selectedCascadeIndex]->GetDepthAttachment(), {200, 200});
                    ImGui::TreePop();
                }
                ImGui::TreePop();
            }

#ifdef SURGE_DEBUG
            if (ImGuiAux::PropertyGridHeader("All Entities (Debug Only)", false))
            {
                SceneHierarchyPanel* hierarchy = static_cast<Editor*>(Core::GetClient())->GetPanelManager().GetPanel<SceneHierarchyPanel>();
                Scene* scene = hierarchy->GetSceneContext();
                scene->GetRegistry().each([&scene](entt::entity e) {
                    Entity ent = Entity(e, scene);
                    ImGui::Text("%i -", e);
                    ImGui::SameLine();
                    ImGui::Text(ent.GetComponent<NameComponent>().Name.c_str());
                });
                ImGui::TreePop();
            }
#endif
        }
        ImGui::End();
    }

    void PerformancePanel::Shutdown()
    {
    }
} // namespace Surge