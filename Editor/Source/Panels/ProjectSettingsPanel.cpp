// Copyright (c) - SurgeTechnologies - All rights reserved
#include "ProjectSettingsPanel.hpp"
#include "Surge/Core/Core.hpp"
#include "Editor.hpp"
#include <imgui.h>
#include <imgui_stdlib.h>
#include "IconsFontAwesome.hpp"

namespace Surge
{
    void ProjectSettingsPanel::Init(void* panelInitArgs)
    {
        mCode = GetStaticCode();
        mActiveProject = &reinterpret_cast<Editor*>(Surge::Core::GetClient())->GetActiveProject();
    }

    void ProjectSettingsPanel::Render(bool* show)
    {
        if (!*show)
            return;

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

                for (Uint i = 0; i < mActiveProject->GetAllScenes().size(); i++)
                {
                    ImGui::PushID(i);
                    ImGui::TableNextColumn();

                    Ref<Scene>& scene = mActiveProject->GetAllScenes()[i];
                    UUID currentSceneUUID = scene->GetUUID();

                    bool opened = false;
                    {
                        ImGuiAux::ScopedColor style({ImGuiCol_Header, ImGuiCol_HeaderHovered, ImGuiCol_HeaderActive}, {0.0, 0.0, 0.0, 0.0});
                        opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(scene.Raw()), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Bullet, scene->GetName().c_str());
                    }
                    if (ImGui::IsItemClicked() && !mRenamingMech)
                        mSelectedSceneUUID = currentSceneUUID;

                    if (ImGui::IsWindowHovered() && Input::IsKeyPressed(Key::F2))
                        mRenamingMech.SetRenamingState(true);

                    if (mSelectedSceneUUID == currentSceneUUID)
                        mRenamingMech.Update(scene->GetName());

                    if (mSelectedSceneUUID == currentSceneUUID)
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiAux::Colors::ExtraDark));

                    // Second column
                    ImGui::TableNextColumn();
                    if (mActiveProject->GetActiveScene()->GetUUID() != currentSceneUUID)
                    {
                        if (ImGui::SmallButton("Remove"))
                            mActiveProject->RemoveScene(i);
                        ImGui::SameLine();
                        if (ImGui::SmallButton("Edit"))
                            mActiveProject->SetActiveScene(i);
                    }

                    if (opened)
                        ImGui::TreePop();

                    ImGui::PopID();
                }
                ImGui::EndTable();

                if (ImGuiAux::ButtonCentered("Add Scene"))
                {
                    Ref<Scene> newScene = mActiveProject->AddScene("NewScene");
                    mSelectedSceneUUID = newScene->GetUUID();
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
