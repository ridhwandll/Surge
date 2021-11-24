// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Panels/InspectorPanel.hpp"
#include "Surge/ECS/Components.hpp"
#include "Utility/ImGuiAux.hpp"
#include "Surge/Utility/FileDialogs.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <IconsFontAwesome.hpp>

namespace Surge
{
    template <typename XComponent, typename Func>
    static void DrawComponent(Entity& entity, const String& name, Func& function, bool isRemoveable = true)
    {
        const int64_t& hash = SurgeReflect::GetReflection<XComponent>()->GetHash();
        ImGui::PushID(static_cast<int>(hash));

        // Push the bold font
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        bool open = ImGuiAux::PropertyGridHeader(name.c_str());
        ImGui::PopFont();

        if (open)
        {
            const ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
            const float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y;

            if (isRemoveable)
            {
                ImGui::SameLine();
                ImGui::SetCursorPosX(contentRegionAvailable.x + 13.0f);
                if (ImGui::Button(ICON_SURGE_TRASH_O))
                    entity.RemoveComponent<XComponent>();
            }
            if (ImGui::BeginTable("##ComponentTable", 2, ImGuiTableFlags_Resizable))
            {
                function();
                ImGui::EndTable();
            }
            ImGui::TreePop();
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
                    if (ImGui::MenuItem("Point Light"))
                        entity.AddComponent<PointLightComponent>();
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
            DrawComponent<TransformComponent>(
                entity, "Transform", [&component]() {
                    ImGuiAux::Property<glm::vec3>("Position", component.Position);
                    ImGuiAux::Property<glm::vec3>("Rotation", component.Rotation);
                    ImGuiAux::Property<glm::vec3>("Scale", component.Scale);
                },
                false);
        }

        if (entity.HasComponent<MeshComponent>())
        {
            MeshComponent& component = entity.GetComponent<MeshComponent>();
            DrawComponent<MeshComponent>(entity, "Mesh", [&component]() {
                const String meshPath = component.Mesh ? component.Mesh->GetPath() : "";
                if (ImGuiAux::Button("Path", meshPath.empty() ? "Open..." : meshPath.c_str()))
                {
                    String path = FileDialog::OpenFile("");
                    if (!path.empty())
                        component.Mesh = Ref<Mesh>::Create(path);
                }

                if (component.Mesh)
                {
                    Ref<Material>& material = component.Material;
                    const ShaderBuffer& shaderBuffer = material->GetShaderBuffer();
                    for (const ShaderBufferMember& member : shaderBuffer.Members)
                    {
                        if (member.DataType == ShaderDataType::Int)
                            ImGui::DragInt(member.Name.c_str(), &material->Get<int>(member.Name));
                        if (member.DataType == ShaderDataType::Float)
                            ImGui::SliderFloat(member.Name.c_str(), &material->Get<float>(member.Name), 0.0f, 1.0f);
                        if (member.DataType == ShaderDataType::Float2)
                            ImGui::DragFloat2(member.Name.c_str(), glm::value_ptr(material->Get<glm::vec2>(member.Name)));
                        if (member.DataType == ShaderDataType::Float3)
                            ImGui::ColorEdit3(member.Name.c_str(), glm::value_ptr(material->Get<glm::vec3>(member.Name)));
                        if (member.DataType == ShaderDataType::Float4)
                            ImGui::DragFloat4(member.Name.c_str(), glm::value_ptr(material->Get<glm::vec4>(member.Name)));
                    }
                }
            });
        }

        if (entity.HasComponent<CameraComponent>())
        {
            CameraComponent& component = entity.GetComponent<CameraComponent>();
            DrawComponent<CameraComponent>(entity, "Camera", [&component]() {
                RuntimeCamera& camera = component.Camera;
                ImGuiAux::Property<bool>("Primary", component.Primary);

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
                    if (ImGuiAux::Property<float>("Vertical FOV", verticalFOV)) // In degree
                        camera.SetPerspectiveVerticalFOV(verticalFOV);

                    float nearClip = camera.GetPerspectiveNearClip();
                    if (ImGuiAux::Property<float>("Near Clip", nearClip))
                        camera.SetPerspectiveNearClip(nearClip);

                    float farClip = camera.GetPerspectiveFarClip();
                    if (ImGuiAux::Property<float>("Far Clip", farClip))
                        camera.SetPerspectiveFarClip(farClip);
                }

                if (camera.GetProjectionType() == RuntimeCamera::ProjectionType::Orthographic)
                {
                    float orthoSize = camera.GetOrthographicSize();
                    if (ImGuiAux::Property<float>("Size", orthoSize))
                        camera.SetOrthographicSize(orthoSize);

                    float nearClip = camera.GetOrthographicNearClip();
                    if (ImGuiAux::Property<float>("Near Clip", nearClip))
                        camera.SetOrthographicNearClip(nearClip);

                    float farClip = camera.GetOrthographicFarClip();
                    if (ImGuiAux::Property<float>("Far Clip", farClip))
                        camera.SetOrthographicFarClip(farClip);

                    ImGuiAux::Property<bool>("Fixed Aspect Ratio", component.FixedAspectRatio);
                }
            });
        }

        if (entity.HasComponent<PointLightComponent>())
        {
            PointLightComponent& component = entity.GetComponent<PointLightComponent>();
            DrawComponent<PointLightComponent>(entity, "PointLight", [&component]() {
                ImGuiAux::Property<glm::vec3, ImGuiAux::CustomProprtyFlag::Color3>("Color", component.Color);
                ImGuiAux::Property<float>("Intensity", component.Intensity);
                ImGuiAux::Property<float>("Radius", component.Radius);
            });
        }

        // Debug Only
        if (entity.HasComponent<ParentChildComponent>())
        {
            ParentChildComponent& component = entity.GetComponent<ParentChildComponent>();
            DrawComponent<ParentChildComponent>(
                entity, "ParentChildComponent", [this, &component]() {
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Parent:");
                    ImGui::TableNextColumn();
                    Entity parent = mHierarchy->GetSceneContext()->FindEntityByUUID(component.ParentID);
                    parent ? ImGui::TextUnformatted(parent.GetComponent<NameComponent>().Name.c_str()) : ImGui::TextUnformatted("None");

                    ImGui::TableNextColumn();
                    ImGui::Text("ChildCount: %i", component.ChildIDs.size());
                },
                false);
        }
    }

} // namespace Surge
