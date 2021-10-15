// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Panels/InspectorPanel.hpp"
#include "Surge/ECS/Components.hpp"
#include <imgui.h>
#include <imgui_stdlib.h>
#include "glm/gtc/type_ptr.hpp"
#include "Surge/Utility/FileDialogs.hpp"

namespace Surge
{
    template <typename Func>
    static void DrawComponent(const String& name, Func& function)
    {
        ImGui::PushID(name.c_str());
        if (ImGui::CollapsingHeader(name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::BeginTable("##ComponentTable", 2, ImGuiTableFlags_Resizable))
            {
                ImGui::TableNextColumn();
                function();
                ImGui::EndTable();
            }
        }
        ImGui::PopID();
    }

    void InspectorPanel::Init(void* panelInitArgs)
    {
        mCode = GetStaticCode();
        mHierarchy = nullptr;
    }

    void InspectorPanel::Render(bool* show)
    {
        if (!*show)
            return;

        if (ImGui::Begin(PanelCodeToString(mCode), show))
        {
            Entity& entity = mHierarchy->GetSelectedEntity();
            if (entity)
                DrawComponents(entity);
        }
        ImGui::End();
    }

    void InspectorPanel::Shutdown()
    {
    }

    void InspectorPanel::DrawComponents(Entity& entity)
    {
        if (entity.HasComponent<NameComponent>())
        {
            NameComponent& component = entity.GetComponent<NameComponent>();
            ImGui::PushItemWidth(-1);
            ImGui::InputText("##n@Me", &component.Name);
            ImGui::PopItemWidth();
        }

        if (entity.HasComponent<TransformComponent>())
        {
            TransformComponent& component = entity.GetComponent<TransformComponent>();
            DrawComponent("Transform", [&component]() {
                ImGui::TextUnformatted("Position");
                ImGui::TableNextColumn();
                ImGui::DragFloat3("##pos", glm::value_ptr(component.Position));
                ImGui::TableNextColumn();

                ImGui::TextUnformatted("Rotation");
                ImGui::TableNextColumn();
                ImGui::DragFloat3("##rot", glm::value_ptr(component.Rotation));
                ImGui::TableNextColumn();

                ImGui::TextUnformatted("Scale");
                ImGui::TableNextColumn();
                ImGui::DragFloat3("##scale", glm::value_ptr(component.Scale));
            });
        }

        if (entity.HasComponent<MeshComponent>())
        {
            MeshComponent& component = entity.GetComponent<MeshComponent>();
            DrawComponent("Mesh", [&component]() {
                ImGui::TextUnformatted("Path");
                ImGui::TableNextColumn();
                const String& path = component.Mesh ? component.Mesh->GetPath() : "";
                if (ImGui::Button(path.empty() ? "Open..." : path.c_str()))
                {
                    String path = FileDialog::OpenFile("");
                    if (!path.empty())
                        component.Mesh = Ref<Mesh>::Create(path);
                }
            });
        }
    }

} // namespace Surge
