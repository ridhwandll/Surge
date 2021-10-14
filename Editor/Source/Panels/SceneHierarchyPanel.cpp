// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Panels/SceneHierarchyPanel.hpp"
#include "Surge/ECS/Components.hpp"
#include "Surge/Core/Input/Input.hpp"
#include <imgui.h>
#include <imgui_stdlib.h>

namespace Surge
{
    void SceneHierarchyPanel::Init(void* panelInitArgs)
    {
        mCode = GetStaticCode();
        mSceneContext = nullptr;
        mSelectedEntity = {};
        mHierarchyHovered = false;
        mRenaming = false;
    }

    void SceneHierarchyPanel::Render(bool* show)
    {
        if (!*show)
            return;

        mHierarchyHovered = ImGui::IsWindowHovered();
        if (mHierarchyHovered && ImGui::IsMouseClicked(0))
            mSelectedEntity = {};

        if (ImGui::Begin(PanelCodeToString(mCode), show))
        {
            uint64_t idCounter = 0;
            mSceneContext->GetRegistry().each([&](entt::entity e) {
                ImGui::PushID(idCounter);
                Entity ent = Entity(e, mSceneContext);
                DrawEntityNode(ent);
                ImGui::PopID();
                idCounter++;
            });
        }

        ImGui::End();
    }

    void SceneHierarchyPanel::DrawEntityNode(Entity& e)
    {
        String& name = e.GetComponent<NameComponent>().Name;
        ImGuiTreeNodeFlags flags = ((mSelectedEntity == e) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowItemOverlap;

        bool isSelectedEntity = false;
        if (mSelectedEntity == e)
        {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            isSelectedEntity = true;
        }

        bool opened = false;
        opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uint64_t>(static_cast<Uint>(e.Raw()))), flags, name.c_str());

        if (isSelectedEntity)
        {
            // Start renaming
            if (ImGui::IsWindowHovered() && Input::IsKeyPressed(Key::F2))
                mRenaming = true;

            // Renaming ongoing
            if (mRenaming)
            {
                ImGui::SameLine();
                if (!name.empty())
                {
                    mTempBuffer = name;
                    mOldName = mTempBuffer;
                    name.clear();
                }
                if (ImGui::InputText("##Txt", &mTempBuffer, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    mRenaming = false;
                    name = mTempBuffer;
                    mTempBuffer.clear();
                    mOldName.clear();
                }
                ImGui::SetKeyboardFocusHere();

                // Revert to old name if user hits Escape
                if (Input::IsKeyPressed(Key::Escape))
                {
                    mRenaming = false;
                    name = mOldName;
                    mOldName.clear();
                    mTempBuffer.clear();
                }
            }

            ImGui::PopStyleColor();
        }

        if (ImGui::IsItemClicked())
            mSelectedEntity = e;

        if (opened)
            ImGui::TreePop();
    }

    void SceneHierarchyPanel::Shutdown()
    {
    }

} // namespace Surge
