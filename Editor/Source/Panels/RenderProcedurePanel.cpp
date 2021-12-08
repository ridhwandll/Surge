// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Core/Core.hpp"
#include "Panels/RenderProcedurePanel.hpp"
#include "Surge/Graphics/RenderProcedure/GeometryProcedure.hpp"
#include "Surge/Graphics/RenderProcedure/ShadowMapProcedure.hpp"
#include "Utility/ImGuiAux.hpp"

namespace Surge
{
    template <typename T, typename F>
    static void PropertyRenderProcedure(const char* name, F uiFunction)
    {
        if (ImGuiAux::PropertyGridHeader(name, false))
        {
            if (ImGui::BeginTable("ShaderTable", 2, ImGuiTableFlags_Resizable))
            {
                RenderProcedureManager* renderProcManager = Core::GetRenderer()->GetRenderProcManager();
                T* proc = renderProcManager->GetProcedure<T>();

                bool isProcActive = renderProcManager->IsProcecureActive<T>();
                if (ImGuiAux::TProperty<bool>("Active", &isProcActive))
                    renderProcManager->SetProcecureActive<T>(isProcActive);

                if (isProcActive)
                {
                    if (ImGuiAux::TButton("Restart Procedure", "Restart"))
                        renderProcManager->RestartProcedure<ShadowMapProcedure>();
                    uiFunction(renderProcManager, proc);
                }

                ImGui::EndTable();
            }

            ImGui::TreePop();
        }
    }

    void RenderProcedurePanel::Init(void* panelInitArgs)
    {
        mCode = GetStaticCode();
    }

    void RenderProcedurePanel::Render(bool* show)
    {
        if (!*show)
            return;

        if (ImGui::Begin(PanelCodeToString(mCode), show))
        {
            PropertyRenderProcedure<GeometryProcedure>("Geometry Procedure", [](RenderProcedureManager* renderProcManager, GeometryProcedure* proc) {
            });

            PropertyRenderProcedure<ShadowMapProcedure>("Shadow Map Procedure", [](RenderProcedureManager* renderProcManager, ShadowMapProcedure* proc) {
                static Uint selectedCascadeIndex = 0;
                ShadowMapProcedure::InternalData* shadowProcInternalData = renderProcManager->GetRenderProcData<ShadowMapProcedure>();

                const char* cascadeCountStrings[] = {"2", "3", "4"};
                ImGuiAux::TComboBox("Cascade Count", cascadeCountStrings, 3, static_cast<int>(proc->GetCascadeCount()) - 2, [&](int i) {
                    proc->SetCascadeCount(static_cast<CascadeCount>(i + 2));
                });

                const char* shadowQualityStrings[] = {"Low", "Medium", "Ultra"};
                ImGuiAux::TComboBox("Shadow Quality", shadowQualityStrings, 3, static_cast<int>(proc->GetShadowQuality()), [&](int i) {
                    proc->SetShadowQuality(static_cast<ShadowQuality>(i));
                });

                ImGuiAux::TProperty<bool>("Visualize Cascades", &shadowProcInternalData->VisualizeCascades);
                ImGuiAux::TProperty<float>("Cascade Split Lambda", &shadowProcInternalData->CascadeSplitLambda);
                int shadowMapResolution = static_cast<int>(proc->GetShadowMapsResolution());
                if (ImGuiAux::TProperty<int>("Shadow Map Resolution", &shadowMapResolution, 1024, 8192))
                    proc->SetShadowMapsResolution(shadowMapResolution);

                ImGui::TreePop();
            });
        }
    }

    void RenderProcedurePanel::Shutdown()
    {
    }

} // namespace Surge
