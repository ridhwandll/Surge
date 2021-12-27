// Copyright (c) - SurgeTechnologies - All rights reserved
#include "ProjectSettingsPanel.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Input/Input.hpp"
#include "Editor.hpp"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <IconsFontAwesome.hpp>

namespace Surge
{
    void ProjectSettingsPanel::Init(void* panelInitArgs)
    {
        mCode = GetStaticCode();
    }

    void ProjectSettingsPanel::Render(bool* show)
    {
        if (!*show)
            return;

        Project& activeProject = reinterpret_cast<Editor*>(Surge::Core::GetClient())->GetActiveProject();

        if (ImGui::Begin(PanelCodeToString(mCode), show))
        {
            {
                ImGuiAux::ScopedBoldFont scopedBoldFont;
                ImGuiAux::TextCentered("Scenes");
            }
            if (ImGui::BeginTable("SceneTable", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable))
            {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();

                for (Uint i = 0; i < activeProject.GetAllScenes().size(); i++)
                {
                    ImGui::PushID(i);
                    ImGui::TableNextColumn();

                    Ref<Scene>& scene = activeProject.GetAllScenes()[i];
                    UUID currentSceneUUID = scene->GetMetadata().SceneUUID;

                    bool opened = false;
                    {
                        ImGuiAux::ScopedColor style({ImGuiCol_Header, ImGuiCol_HeaderHovered, ImGuiCol_HeaderActive}, {0.0, 0.0, 0.0, 0.0});
                        opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(scene.Raw()), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Bullet, scene->GetMetadata().Name.c_str());
                    }
                    if (ImGui::IsItemClicked() && !mRenamingMech)
                        mSelectedSceneUUID = currentSceneUUID;

                    if (ImGui::IsWindowHovered() && Input::IsKeyPressed(Key::F2))
                        mRenamingMech.SetRenamingState(true);

                    if (mSelectedSceneUUID == currentSceneUUID)
                    {
                        mRenamingMech.Update(scene->GetMetadata().Name, [&](const String& newName) {
                            activeProject.RenameScene(i, newName);
                        });
                    }

                    if (mSelectedSceneUUID == currentSceneUUID)
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiAux::Colors::ExtraDark));

                    // Second column
                    ImGui::TableNextColumn();
                    if (activeProject.GetActiveScene()->GetMetadata().SceneUUID != currentSceneUUID)
                    {
                        if (ImGui::SmallButton("Remove"))
                            activeProject.RemoveScene(i);
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Edit"))
                            activeProject.SetActiveScene(i);
                    }

                    if (opened)
                        ImGui::TreePop();

                    ImGui::PopID();
                }
                ImGui::EndTable();

                if (ImGuiAux::ButtonCentered("Add Scene"))
                {
                    // TODO: Don't hardcode the scene path in future
                    Ref<Scene> newScene = activeProject.AddScene("NewScene", fmt::format("{0}/NewScene.surge", activeProject.GetMetadata().ProjPath));
                    mSelectedSceneUUID = newScene->GetMetadata().SceneUUID;
                    mRenamingMech.SetRenamingState(true);
                }
            }
        }
        ImGui::End();
    }

    void ProjectSettingsPanel::Shutdown()
    {
    }

} // namespace Surge
