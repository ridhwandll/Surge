// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Panels/SceneHierarchyPanel.hpp"
#include "Surge/ECS/Components.hpp"
#include "Surge/Core/Input/Input.hpp"
#include "Utility/ImGUIAux.hpp"
#include <imgui.h>
#include <imgui_stdlib.h>

namespace Surge
{
    void SceneHierarchyPanel::Init(void* panelInitArgs)
    {
        mCode = GetStaticCode();
        mSceneContext = nullptr;
        mSelectedEntity = {};
        mRenaming = false;
    }

    void SceneHierarchyPanel::Render(bool* show)
    {
        if (!*show)
            return;

        // Draw the entities
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        if (ImGui::Begin(PanelCodeToString(mCode), show))
        {
            if (ImGui::Button("Add Entity", {ImGui::GetWindowWidth() - 15, 0.0f}))
                ImGui::OpenPopup("Add Entity");

            if (ImGui::BeginPopup("Add Entity") || (ImGui::BeginPopupContextWindow(nullptr, 1, false)))
            {
                if (ImGui::MenuItem("Empty Entity"))
                {
                    mSceneContext->CreateEntity(mSelectedEntity, "Entity");
                    mRenaming = true;
                }
                if (ImGui::MenuItem("Mesh"))
                {
                    mSceneContext->CreateEntity(mSelectedEntity, "Mesh");
                    mSelectedEntity.AddComponent<MeshComponent>();
                    mRenaming = true;
                }
                if (ImGui::MenuItem("Camera"))
                {
                    mSceneContext->CreateEntity(mSelectedEntity, "Camera");
                    mSelectedEntity.AddComponent<CameraComponent>();
                }
                if (ImGui::MenuItem("Point Light"))
                {
                    mSceneContext->CreateEntity(mSelectedEntity, "Point Light");
                    mSelectedEntity.AddComponent<PointLightComponent>();
                }
                ImGui::EndPopup();
            }

            Uint idCounter = 0;
            mSceneContext->GetRegistry().each([&](entt::entity e) {
                ImGui::PushID(idCounter);
                Entity ent = Entity(e, mSceneContext);
                DrawEntityNode(ent);
                ImGui::PopID();
                idCounter++;
            });
        }
        ImGui::PopStyleColor(2);
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
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.123f, 0.123f, 0.123f, 1.0f));
            isSelectedEntity = true;
        }

        bool opened = false;
        opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uint64_t>(static_cast<Uint>(e.Raw()))), flags, name.c_str());

        if (ImGui::IsItemClicked() && !mRenaming)
            mSelectedEntity = e;

        if (isSelectedEntity)
        {
            if (!mRenaming && mSelectedEntity)
                ImGuiAux::DrawRectAroundWidget({1.0f, 0.5f, 0.1f, 1.0f}, 1.5f, 1.0f);

            // Start renaming
            if (ImGui::IsWindowHovered() && Input::IsKeyPressed(Key::F2))
                mRenaming = true;

            // Renaming ongoing
            if (mRenaming)
            {
                if (!name.empty())
                {
                    mTempBuffer = name;
                    mOldName = mTempBuffer;
                    name.clear();
                }
                ImGui::SameLine();

                // Copy the name from mTempBuffer
                if (ImGui::InputText("##Txt", &mTempBuffer, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    mRenaming = false;
                    name = mTempBuffer;
                    mTempBuffer.clear();
                    mOldName.clear();
                }
                ImGui::SetKeyboardFocusHere();
                ImGuiAux::DrawRectAroundWidget({0.1f, 0.3f, 1.0f, 1.0f}, 1.5f, 1.0f);

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

        if (ImGui::BeginPopupContextItem() && !mRenaming)
        {
            if (ImGui::MenuItem("Delete"))
            {
                if (mSelectedEntity == e)
                    mSelectedEntity = {};

                mSceneContext->DestroyEntity(e);
            }

            ImGui::EndPopup();
        }

        if (opened)
            ImGui::TreePop();

        ImGui::SetCursorPos({ImGui::GetCursorPos().x, ImGui::GetCursorPos().y - 3});
    }

    void SceneHierarchyPanel::Shutdown()
    {
    }
} // namespace Surge