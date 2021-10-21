// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Panels/InspectorPanel.hpp"
#include "Surge/ECS/Components.hpp"
#include <imgui.h>
#include <imgui_stdlib.h>
#include "glm/gtc/type_ptr.hpp"
#include "Surge/Utility/FileDialogs.hpp"

namespace Surge
{
    template <typename XComponent, typename Func>
    static void DrawComponent(const String& name, Func& function)
    {
        int64_t hash = SurgeReflect::GetReflection<XComponent>()->GetHash();
        ImGui::PushID(static_cast<int>(hash));
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
            if (mHierarchy->GetSceneContext()->IsValid(entity.Raw()))
            {
                DrawComponents(entity);

                if (ImGui::Button("Add Component", {ImGui::GetWindowWidth() - 15, 0.0f}))
                    ImGui::OpenPopup("AddComponentPopup");

                if (ImGui::BeginPopup("AddComponentPopup"))
                {
                    if (ImGui::MenuItem("Camera"))
                        entity.AddComponent<CameraComponent>();
                    if (ImGui::MenuItem("Mesh"))
                        entity.AddComponent<MeshComponent>();
                    ImGui::EndPopup();
                }
            }
        }
        ImGui::End();
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
            DrawComponent<TransformComponent>("Transform", [&component]() {
                ImGui::TextUnformatted("Position");
                ImGui::TableNextColumn();
                ImGui::DragFloat3("##pos", glm::value_ptr(component.Position), 0.1f, 0.0f, 0.0f, "%.2f");
                ImGui::TableNextColumn();

                ImGui::TextUnformatted("Rotation");
                ImGui::TableNextColumn();
                ImGui::DragFloat3("##rot", glm::value_ptr(component.Rotation), 0.1f, 0.0f, 0.0f, "%.2f");
                ImGui::TableNextColumn();

                ImGui::TextUnformatted("Scale");
                ImGui::TableNextColumn();
                ImGui::DragFloat3("##scale", glm::value_ptr(component.Scale), 0.1f, 0.0f, 0.0f, "%.2f");
            });
        }

        if (entity.HasComponent<MeshComponent>())
        {
            MeshComponent& component = entity.GetComponent<MeshComponent>();
            DrawComponent<MeshComponent>("Mesh", [&component]() {
                ImGui::TextUnformatted("Path");
                ImGui::TableNextColumn();
                const String& path = component.Mesh ? component.Mesh->GetPath() : "";
                if (ImGui::Button(path.empty() ? "Open..." : path.c_str()))
                {
                    String path = FileDialog::OpenFile("");
                    if (!path.empty())
                        component.Mesh = Ref<Mesh>::Create(path);
                }
                if (component.Mesh)
                {
                    Ref<Material>& material = component.Mesh->GetMaterial();
                    const ShaderBuffer& shaderBuffer = material->GetShaderBuffer();
                    for (const ShaderBufferMember& member : shaderBuffer.Members)
                    {
                        if (member.DataType == ShaderDataType::Int)
                            ImGui::DragInt(member.Name.c_str(), &material->Get<int>(member.Name));
                        if (member.DataType == ShaderDataType::Float)
                            ImGui::DragFloat(member.Name.c_str(), &material->Get<float>(member.Name));
                        if (member.DataType == ShaderDataType::Float2)
                            ImGui::DragFloat2(member.Name.c_str(), glm::value_ptr(material->Get<glm::vec2>(member.Name)));
                        if (member.DataType == ShaderDataType::Float3)
                            ImGui::DragFloat3(member.Name.c_str(), glm::value_ptr(material->Get<glm::vec3>(member.Name)));
                        if (member.DataType == ShaderDataType::Float4)
                            ImGui::DragFloat4(member.Name.c_str(), glm::value_ptr(material->Get<glm::vec4>(member.Name)));
                    }
                }
            });
        }

        if (entity.HasComponent<CameraComponent>())
        {
            CameraComponent& component = entity.GetComponent<CameraComponent>();
            DrawComponent<CameraComponent>("Camera", [&component]() {
                RuntimeCamera& camera = component.Camera;
                ImGui::TextUnformatted("Primary");
                ImGui::TableNextColumn();
                ImGui::Checkbox("##prIm@ry", &component.Primary);

                const char* projectionTypeStrings[] = {"Perspective", "Orthographic"};
                const char* currentProjectionTypeString = projectionTypeStrings[static_cast<int>(camera.GetProjectionType())];

                ImGui::TableNextColumn();
                ImGui::TextUnformatted("Projection");
                ImGui::TableNextColumn();
                ImGui::PushItemWidth(-1);
                if (ImGui::BeginCombo("##Projection", currentProjectionTypeString))
                {
                    for (int i = 0; i < 2; i++)
                    {
                        const bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
                        if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
                        {
                            currentProjectionTypeString = projectionTypeStrings[i];
                            camera.SetProjectionType(static_cast<RuntimeCamera::ProjectionType>(i));
                        }
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();

                if (camera.GetProjectionType() == RuntimeCamera::ProjectionType::Perspective)
                {
                    float verticalFOV = camera.GetPerspectiveVerticalFOV();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Vertical FOV");
                    ImGui::TableNextColumn();
                    if (ImGui::DragFloat("##verticalFov", &verticalFOV)) // In degree
                        camera.SetPerspectiveVerticalFOV(verticalFOV);

                    float nearClip = camera.GetPerspectiveNearClip();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Near Clip");
                    ImGui::TableNextColumn();
                    if (ImGui::DragFloat("##nclIp", &nearClip))
                        camera.SetPerspectiveNearClip(nearClip);

                    float farClip = camera.GetPerspectiveFarClip();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Far Clip");
                    ImGui::TableNextColumn();
                    if (ImGui::DragFloat("##FaRcLiP", &farClip))
                        camera.SetPerspectiveFarClip(farClip);
                }

                if (camera.GetProjectionType() == RuntimeCamera::ProjectionType::Orthographic)
                {
                    float orthoSize = camera.GetOrthographicSize();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Size");
                    ImGui::TableNextColumn();
                    if (ImGui::DragFloat("##sizee", &orthoSize))
                        camera.SetOrthographicSize(orthoSize);

                    float nearClip = camera.GetOrthographicNearClip();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Near Clip");
                    ImGui::TableNextColumn();
                    if (ImGui::DragFloat("##neArCLipOrtho", &nearClip))
                        camera.SetOrthographicNearClip(nearClip);

                    float farClip = camera.GetOrthographicFarClip();
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Far Clip");
                    ImGui::TableNextColumn();
                    if (ImGui::DragFloat("##faRCLiPOrtho", &farClip))
                        camera.SetOrthographicFarClip(farClip);

                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Fixed Aspect Ratio");
                    ImGui::TableNextColumn();
                    ImGui::Checkbox("##FiXeDasPectRatio", &component.FixedAspectRatio);
                }
            });
        }
    }

    void InspectorPanel::Shutdown()
    {
    }

} // namespace Surge
