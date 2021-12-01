// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Panels/SceneHierarchyPanel.hpp"
#include "Surge/ECS/Components.hpp"
#include "Surge/Core/Input/Input.hpp"
#include "Utility/ImGUIAux.hpp"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>

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
                ImGui::Separator();
                if (ImGui::MenuItem("Point Light"))
                {
                    mSceneContext->CreateEntity(mSelectedEntity, "Point Light");
                    mSelectedEntity.AddComponent<PointLightComponent>();
                }
                if (ImGui::MenuItem("Directional Light"))
                {
                    mSceneContext->CreateEntity(mSelectedEntity, "Directional Light");
                    mSelectedEntity.AddComponent<DirectionalLightComponent>();
                }
                ImGui::EndPopup();
            }

            constexpr ImGuiTableFlags flags = ImGuiTableFlags_Resizable;
            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, {0.0f, 2.8f});
            if (ImGui::BeginTable("HierarchyTable", 2, flags))
            {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, ImGui::CalcTextSize("A").x * 12.0f);
                ImGui::TableHeadersRow();

                Uint idCounter = 0;
                mSceneContext->GetRegistry().each([&](entt::entity e) {
                    ImGui::PushID(idCounter);
                    Entity ent = Entity(e, mSceneContext);

                    // Only draw the entities which are not parented, i.e. at top level
                    if (ent.GetParent() == 0)
                    {
                        ImGui::TableNextColumn();

                        // NOTE(Rid): DrawEntityNode is a recursive function, that is DrawEntityNode(child) is called inside DrawEntityNode, to draw the children
                        DrawEntityNode(ent);
                    }

                    ImGui::PopID();
                    idCounter++;
                });
                ImGui::EndTable();
            }
            ImGui::PopStyleVar();
        }
        ImGui::PopStyleColor(2);
        ImGui::End();
    }

    void SceneHierarchyPanel::DrawEntityNode(Entity& e)
    {
        String& name = e.GetComponent<NameComponent>().Name;
        ImGuiTreeNodeFlags flags = ((mSelectedEntity == e) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
        flags |= ImGuiTreeNodeFlags_SpanFullWidth;

        bool isSelectedEntity = false;
        if (mSelectedEntity == e)
        {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.123f, 0.123f, 0.123f, 1.0f));
            isSelectedEntity = true;
        }

        bool opened = false;
        {
            ImGuiAux::ScopedColor style({ImGuiCol_Header, ImGuiCol_HeaderHovered, ImGuiCol_HeaderActive}, {0.0, 0.0, 0.0, 0.0});
            opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uint64_t>(static_cast<Uint>(e.Raw()))), flags, name.c_str());
        }

        // Drag and drop
        if (ImGui::BeginDragDropSource())
        {
            ImGui::Text(e.GetComponent<NameComponent>().Name.c_str());
            ImGui::SetDragDropPayload(HIERARCHY_ENTITY_DND, &e, sizeof(Entity));
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget())
        {
            const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(HIERARCHY_ENTITY_DND);
            if (payload)
            {
                Entity& droppedEntity = *(Entity*)payload->Data;
                mSceneContext->ParentEntity(droppedEntity, e);
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::IsItemClicked() && !mRenaming)
            mSelectedEntity = e;

        if (isSelectedEntity)
        {
            if (!mRenaming && mSelectedEntity)
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32({0.1f, 0.1f, 0.1f, 1.0f}));

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

                // Only execute when the frame ends, else it will give crash on "Entity not found"
                Surge::Core::AddFrameEndCallback([=]() { mSceneContext->DestroyEntity(e); });
            }
            if (e.GetParent() != 0)
            {
                if (ImGui::MenuItem("UnParent"))
                {
                    mSceneContext->UnparentEntity(e);
                }
            }
            ImGui::EndPopup();
        }

        if (opened)
        {
            // Draw children, via recursive function call
            auto& k = e.GetComponent<ParentChildComponent>().ChildIDs;
            for (auto& child : k)
            {
                Entity e = mSceneContext->FindEntityByUUID(child);
                if (e)
                {
                    DrawEntityNode(e);
                }
            }
            ImGui::TreePop();
        }
        if (!e.GetComponent<ParentChildComponent>().ParentID)
        {
            ImGui::TableNextColumn();
            ImGui::TextUnformatted("Entity");
        }
    }

    void SceneHierarchyPanel::Shutdown()
    {
    }

} // namespace Surge
