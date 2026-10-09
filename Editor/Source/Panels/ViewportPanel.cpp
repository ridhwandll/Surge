// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Panels/ViewportPanel.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/Core/Input/Input.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Asset/AssetManager.hpp"
#include "Surge/ECS/Components/Components.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include "SurgeMath/Math.hpp"
#include "Editor.hpp"
#include "ContentBrowserPanel.hpp"
#include "UIEditorPanel.hpp"

#include <imgui.h>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/constants.hpp>
#include "Surge/Graphics/UISystem/UIManager.hpp"
#include "Utility/ImGuiAux.hpp"
#include "Gizmos/GizmoIcons.hpp"

namespace Surge
{
    void ViewportPanel::Init(void* panelInitArgs)
    {
        mCode = GetStaticCode();
        mSceneHierarchy = static_cast<Editor*>(Core::GetClient())->GetPanelManager().GetPanel<SceneHierarchyPanel>();
        mEditorCam = static_cast<EditorCamera*>(panelInitArgs);

        ImGuizmo::Style& style = ImGuizmo::GetStyle();

        style.CenterCircleSize = 5.0f;
        style.TranslationLineThickness = 3.5f;
        style.TranslationLineArrowSize = 8.0f;
        style.RotationLineThickness = 3.0f;
        style.RotationOuterLineThickness = 3.0f;
        style.ScaleLineThickness = 3.5f;
        style.ScaleLineCircleSize = 8.0f;
        style.HatchedAxisLineThickness = 0.0f;
        style.OutlineThickness = 1.25f;
        style.InfoRounding = 4.0f;

        style.Colors[ImGuizmo::COLOR::DIRECTION_X] = ImVec4(0.984f, 0.200f, 0.200f, 0.95f);
        style.Colors[ImGuizmo::COLOR::PLANE_X] = ImVec4(0.984f, 0.200f, 0.200f, 0.25f);

        style.Colors[ImGuizmo::COLOR::DIRECTION_Y] = ImVec4(0.553f, 0.831f, 0.000f, 0.95f);
        style.Colors[ImGuizmo::COLOR::PLANE_Y] = ImVec4(0.553f, 0.831f, 0.000f, 0.25f);

        style.Colors[ImGuizmo::COLOR::DIRECTION_Z] = ImVec4(0.137f, 0.565f, 1.000f, 0.95f);
        style.Colors[ImGuizmo::COLOR::PLANE_Z] = ImVec4(0.137f, 0.565f, 1.000f, 0.25f);

        style.Colors[ImGuizmo::COLOR::SELECTION] = ImVec4(1.000f, 0.659f, 0.000f, 1.00f);
        style.Colors[ImGuizmo::COLOR::TRANSLATION_LINE] = ImVec4(1.000f, 0.659f, 0.000f, 1.00f);
        style.Colors[ImGuizmo::COLOR::ROTATION_USING_BORDER] = ImVec4(1.000f, 0.659f, 0.000f, 1.00f);
        style.Colors[ImGuizmo::COLOR::ROTATION_USING_FILL] = ImVec4(1.000f, 0.659f, 0.000f, 0.25f);
        style.Colors[ImGuizmo::COLOR::SCALE_LINE] = ImVec4(0.450f, 0.450f, 0.450f, 0.70f);
        style.Colors[ImGuizmo::COLOR::INACTIVE] = ImVec4(0.600f, 0.600f, 0.600f, 0.60f);
        style.Colors[ImGuizmo::COLOR::OUTLINE] = ImVec4(0.000f, 0.000f, 0.000f, 0.55f);
        style.Colors[ImGuizmo::COLOR::INFO_BACKGROUND] = ImVec4(0.060f, 0.060f, 0.060f, 0.85f);
        style.Colors[ImGuizmo::COLOR::TEXT] = ImVec4(0.920f, 0.920f, 0.920f, 1.00f);
    }

    void ViewportPanel::OnEvent(Event& e)
    {
        if(!mIsViewportHovered)
            return;

        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<KeyPressedEvent>([&](KeyPressedEvent& keyEvent) {
            // F (focus) lives in Editor::HandleShortcuts, it also works from the Hierarchy
            EditorSettings& settings = static_cast<Editor*>(Core::GetClient())->GetSettings();
            if(!Input::IsMouseButtonPressed(Mouse::ButtonRight) && !Input::IsKeyPressed(Key::LeftControl))
            {
                switch(keyEvent.GetKeyCode())
                {
                    case Key::F11:
                    {
                        mIsFullscreen = !mIsFullscreen;
                        if(!mIsFullscreen)
                            mRestoreScreenPosBeforeFullscreen = true;

                        static_cast<Editor*>(Core::GetClient())->ShowTitlebar(!mIsFullscreen);
                        break;
                    }
                    case Key::Escape:
                    {
                        if(mIsFullscreen)
                        {
                            mIsFullscreen = false;
                            mRestoreScreenPosBeforeFullscreen = true;
                            static_cast<Editor*>(Core::GetClient())->ShowTitlebar(true);
                        }
                        break;
                    }

                    case Key::Q:
                    {
                        if(!mGizmoInUse)
                            settings.GizmoOperation = -1;
                        break;
                    }
                    case Key::W:
                    {
                        if(!mGizmoInUse)
                            settings.GizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
                        break;
                    }
                    case Key::E:
                    {
                        if(!mGizmoInUse)
                            settings.GizmoOperation = ImGuizmo::OPERATION::ROTATE;
                        break;
                    }
                    case Key::R:
                    {
                        if(!mGizmoInUse)
                            settings.GizmoOperation = ImGuizmo::OPERATION::SCALE;
                        break;
                    }
                    case Key::T:
                    {
                        if(!mGizmoInUse)
                            settings.GizmoOperation = ImGuizmo::OPERATION::UNIVERSAL;
                        break;
                    }
                    case Key::X:
                    {
                        if(!mGizmoInUse)
                            settings.GizmoWorldSpace = !settings.GizmoWorldSpace;
                        break;
                    }
                    default:
                        break;
                }
            }
        });
    }

    void ViewportPanel::Render(bool* show)
    {
        if(!*show)
            return;

        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        // Fullscreen handling
        if(mIsFullscreen)
        {
            ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->Pos);
            ImGui::SetNextWindowSize(viewport->Size);
            ImGui::SetNextWindowViewport(viewport->ID);
            windowFlags |= ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        }
        else if(mRestoreScreenPosBeforeFullscreen)
        {
            ImGui::SetNextWindowDockID(mPreviousDockID, ImGuiCond_Always);
            mRestoreScreenPosBeforeFullscreen = false;
            ImGui::SetWindowFocus(PanelCodeToString(mCode));
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0.0f, 0.0f });
        if(ImGui::Begin(PanelCodeToString(mCode), show, windowFlags))
        {
            Editor* editor = static_cast<Editor*>(Core::GetClient());
            bool isPlaying = editor->IsPlaying();

            EditorSettings& settings = editor->GetSettings();

            // Aspect Ratio
            int& currentAspectMode = settings.AspectMode;
            constexpr const char* aspectRatios[] = { "Free Aspect", "16:9 Landscape", "9:16 Portrait", "21:9 Cinematic", "4:3 Classic" };

            ImVec2 availableSpace = ImGui::GetContentRegionAvail();
            float targetAspect = 0.0f;

            if(currentAspectMode == 1) targetAspect = 16.0f / 9.0f;
            else if(currentAspectMode == 2) targetAspect = 9.0f / 16.0f;
            else if(currentAspectMode == 3) targetAspect = 21.0f / 9.0f;
            else if(currentAspectMode == 4) targetAspect = 4.0f / 3.0f;

            float renderWidth = availableSpace.x;
            float renderHeight = availableSpace.y;

            if(targetAspect > 0.0f)
            {
                renderWidth = availableSpace.x;
                renderHeight = renderWidth / targetAspect;

                // If it exceeds the available height, fit height instead
                if(renderHeight > availableSpace.y)
                {
                    renderHeight = availableSpace.y;
                    renderWidth = renderHeight * targetAspect;
                }
            }
            float offsetX = (availableSpace.x - renderWidth) * 0.5f;
            float offsetY = (availableSpace.y - renderHeight) * 0.5f;

            mViewportSize = { renderWidth, renderHeight };
            mIsViewportHovered = ImGui::IsWindowHovered();
            mIsViewportFocused = ImGui::IsWindowFocused();

            if(!mIsFullscreen)
                mPreviousDockID = ImGui::GetWindowDockID();

            // ImGui Cursor setup
            ImVec2 cursorStartPos = ImGui::GetCursorPos();
            ImGui::SetCursorPos({ cursorStartPos.x + offsetX, cursorStartPos.y + offsetY });

            ImVec2 viewportBoundsMin = ImGui::GetCursorScreenPos();
            ImVec2 mainWindowPos = ImGui::GetMainViewport()->Pos;
            float uiBoundsX = viewportBoundsMin.x - mainWindowPos.x;
            float uiBoundsY = viewportBoundsMin.y - mainWindowPos.y;
            Core::GetRenderer()->GetUIManager().SetViewportBounds(uiBoundsX, uiBoundsY, renderWidth, renderHeight);

            if(!mIsFullscreen)
                mPreviousDockID = ImGui::GetWindowDockID();

            ImTextureID mTexID = Core::GetRenderer()->GetFinalImageImGuiID();
            ImGui::Image(mTexID, { renderWidth, renderHeight });

            if(ImGui::BeginDragDropTarget())
            {
                if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(CONTENT_BROWSER_PAYLOAD))
                {
                    SG_ASSERT(payload->DataSize == sizeof(AssetID), "Payload size mismatch!");
                    AssetID droppedAssetID = *(const AssetID*)payload->Data;
                    AssetManager* assetManager = Core::GetAssetManager();
                    const AssetMetadata& metadata = assetManager->GetMetadata(droppedAssetID);

                    switch(metadata.Type)
                    {
                        case AssetType::UI_LAYOUT:
                            static_cast<Editor*>(Core::GetClient())->GetPanelManager().GetPanel<UIEditorPanel>()->OpenLayout(droppedAssetID);
                            break;
                        case AssetType::SCENE:
                            editor->OpenScene(droppedAssetID);
                            break;
                        case AssetType::MESH:
                        {
                            Ref<Mesh> droppedMesh = assetManager->Load<Mesh>(droppedAssetID);
                            if(droppedMesh)
                            {
                                auto* editor = static_cast<Editor*>(Core::GetClient());
                                Ref<Scene> currentScene = editor->GetCurrentScene();
                                Entity newEntity;
                                currentScene->CreateEntity(newEntity, "Mesh");
                                newEntity.AddComponent<MeshComponent>().MeshAsset = droppedMesh;
                                editor->OnEntityCreated(newEntity);
                            }
                            break;
                        }
                        default:
                            Log<Severity::Warn>("[ViewportPanel] Failed to load dropped asset in viewport!");
                            break;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            // UI Editor: outlines/handles + widget editing on top of the rendered layout (submitted before the overlays so they keep the hover)
            bool uiEditingActive = false;
            if(!isPlaying)
                uiEditingActive = editor->GetPanelManager().GetPanel<UIEditorPanel>()->OnViewportGUI(viewportBoundsMin, ImVec2(renderWidth, renderHeight));

            // Editor gizmos: component icons, camera frustums, light/audio ranges
            SceneGizmos& gizmos = editor->GetGizmos();
            if(!isPlaying)
            {
                const EditorCamera& camera = editor->GetCamera();
                const bool allowIconHover = mIsViewportHovered && !uiEditingActive && !ImGuizmo::IsUsing() && !ImGui::IsAnyItemHovered() && !Input::IsMouseButtonPressed(Mouse::ButtonRight);
                gizmos.DrawOverlay(*editor->GetContextScene(), mSceneHierarchy->GetSelectedEntity(), camera.GetViewMatrix(), camera.GetProjectionMatrix(), camera.GetNearAndFarPlane().x, viewportBoundsMin, ImVec2(renderWidth, renderHeight), allowIconHover);
            }

            // Overlays
            ImFont* boldFont = ImGui::GetIO().Fonts->Fonts[1];
            float padding = 3.0f;

            // Aspect Ratio Dropdown
            if(!isPlaying)
            {
                ImGui::SetCursorPos(ImVec2(cursorStartPos.x + offsetX + padding, cursorStartPos.y + offsetY + padding));
                ImGui::SetNextItemWidth(140.0f);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(15, 15, 15, 200));
                if(ImGui::BeginCombo("##AspectCombo", aspectRatios[currentAspectMode]))
                {
                    for(int i = 0; i < 5; i++)
                    {
                        bool isSelected = (currentAspectMode == i);
                        if(ImGui::Selectable(aspectRatios[i], isSelected))
                            currentAspectMode = i;
                        if(isSelected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopStyleColor();

                ImGui::SameLine(0.0f, 6.0f);
                DrawToolbar(settings);
            }

            // Scene Name (* = unsaved changes)
            const String sceneLabel = isPlaying ? String(editor->IsPaused() ? "RUNTIME (PAUSED)" : "RUNTIME") : (editor->GetHistory().IsDirty() ? mSceneName + " *" : mSceneName);
            const char* displayText = sceneLabel.c_str();
            ImGui::PushFont(boldFont);
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            float textHeight = ImGui::GetTextLineHeight();
            float textWidth = ImGui::CalcTextSize(displayText).x;

            ImVec2 bgMax = ImVec2(viewportBoundsMin.x + renderWidth - padding, viewportBoundsMin.y + padding + textHeight + (padding * 2.0f));
            ImVec2 bgMin = ImVec2(bgMax.x - textWidth - (padding * 2.0f), viewportBoundsMin.y + padding);

            drawList->AddRectFilled(bgMin, bgMax, IM_COL32(15, 15, 15, 200), 4.0f);
            drawList->AddText(ImVec2(bgMin.x + padding, bgMin.y + padding), IM_COL32(230, 230, 230, 255), displayText);
            ImGui::PopFont();

            // Gizmos
            if(!isPlaying && !uiEditingActive)
            {
                Entity& selectedEntity = mSceneHierarchy->GetSelectedEntity();
                Editor* app = static_cast<Editor*>(Core::GetClient());

                const EditorCamera& camera = app->GetCamera();
                glm::mat4 cameraView = camera.GetViewMatrix();
                glm::mat4 cameraProjection = camera.GetProjectionMatrix();
                cameraProjection[1][1] *= -1;

                ImGuizmo::SetRect(viewportBoundsMin.x, viewportBoundsMin.y, renderWidth, renderHeight);

                const bool manipulatorActive = selectedEntity && settings.GizmoOperation > 0;
                if(manipulatorActive)
                {
                    ImGuizmo::SetDrawlist();

                    TransformComponent& transformComponent = selectedEntity.GetComponent<TransformComponent>();
                    glm::mat4 transform = transformComponent.GetTransform();

                    // Snapping, holding Ctrl inverts the toolbar toggle
                    const bool snap = settings.SnapEnabled != Input::IsKeyPressed(Key::LeftControl);
                    float snapValue = settings.SnapTranslate;
                    if(settings.GizmoOperation == ImGuizmo::OPERATION::ROTATE)
                        snapValue = settings.SnapRotate;
                    else if(settings.GizmoOperation == ImGuizmo::OPERATION::SCALE)
                        snapValue = settings.SnapScale;

                    float snapValues[3] = { snapValue, snapValue, snapValue };
                    const ImGuizmo::MODE space = settings.GizmoWorldSpace ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
                    ImGuizmo::SetGizmoSizeClipSpace(0.15f);
                    ImGuizmo::Manipulate(glm::value_ptr(cameraView), glm::value_ptr(cameraProjection), static_cast<ImGuizmo::OPERATION>(settings.GizmoOperation), space, glm::value_ptr(transform), nullptr, snap ? snapValues : nullptr);

                    if(ImGuizmo::IsUsing())
                    {
                        mGizmoInUse = true;

                        RelationshipComponent& rel = selectedEntity.GetComponent<RelationshipComponent>();

                        if(rel.Parent != entt::null)
                        {
                            Entity parentEntity((entt::entity)rel.Parent, app->GetCurrentScene().Raw());
                            glm::mat4 parentWorld = parentEntity.GetComponent<TransformComponent>().GetTransform();

                            transform = glm::inverse(parentWorld) * transform;
                        }

                        glm::vec3 translation, rotation, scale;

                        Math::DecomposeTransform(transform, translation, rotation, scale);

                        glm::vec3 deltaRotation = glm::degrees(rotation) - transformComponent.Rotation;
                        transformComponent.Position = translation;
                        transformComponent.Rotation += deltaRotation;
                        transformComponent.Scale = scale;
                        transformComponent.MarkDirty();
                    }
                    else
                        mGizmoInUse = false;
                }

                HandleSelectionClick(selectedEntity, manipulatorActive, viewportBoundsMin, ImVec2(renderWidth, renderHeight));
            }
            else
                mPickPending = false;
        }
        else
        {
            mIsViewportHovered = false;
            mIsViewportFocused = false;
            mViewportSize = { 0.0f, 0.0f };
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void ViewportPanel::HandleSelectionClick(Entity& selectedEntity, bool manipulatorActive, const ImVec2& viewportMin, const ImVec2& viewportSize)
    {
        const ImVec2 mouse = ImGui::GetMousePos();
        if(ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            // The manipulator, overlay widgets and Alt (camera orbit) own the press
            const bool insideImage = mouse.x >= viewportMin.x && mouse.y >= viewportMin.y && mouse.x < viewportMin.x + viewportSize.x && mouse.y < viewportMin.y + viewportSize.y;
            const bool manipulatorHovered = manipulatorActive && (ImGuizmo::IsOver() || ImGuizmo::IsUsing());
            mPickPending = mIsViewportHovered && insideImage && !manipulatorHovered && !ImGui::IsAnyItemHovered() && !Input::IsKeyPressed(Key::LeftAlt);
            mPickPressPos = mouse;
        }

        if(!mPickPending || !ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            return;

        mPickPending = false;
        const float dragX = mouse.x - mPickPressPos.x;
        const float dragY = mouse.y - mPickPressPos.y;
        if(mGizmoInUse || dragX * dragX + dragY * dragY > 16.0f) // Drag threshold (4px) to avoid accidental clicks while moving the camera
            return;

        Editor* editor = static_cast<Editor*>(Core::GetClient());
        Entity picked = editor->GetGizmos().GetHoveredEntity(); // Icons are drawn on top of everything, so they win over the geometry behind them
        if(!picked)
        {
            const EditorCamera& camera = editor->GetCamera();
            const PickRay ray = ScenePicker::ScreenToRay({ mouse.x, mouse.y }, { viewportMin.x, viewportMin.y }, { viewportSize.x, viewportSize.y }, camera.GetViewMatrix(), camera.GetProjectionMatrix());
            picked = mPicker.Pick(*editor->GetContextScene(), ray, camera.GetViewMatrix(), { mouse.x, mouse.y }, selectedEntity);
        }

        if(picked != selectedEntity)
            editor->SelectEntity(picked); // Empty space clears the selection
    }

    void ViewportPanel::DrawToolbar(EditorSettings& settings)
    {
        struct Tool
        {
            ToolIcon Icon;
            int Operation;
            const char* Tooltip;
        };
        static constexpr Tool sTools[] = {
            { ToolIcon::SELECT, -1, "Select only (Q)" },
            { ToolIcon::MOVE, ImGuizmo::OPERATION::TRANSLATE, "Move (W)" },
            { ToolIcon::ROTATE, ImGuizmo::OPERATION::ROTATE, "Rotate (E)" },
            { ToolIcon::SCALE, ImGuizmo::OPERATION::SCALE, "Scale (R)" },
            { ToolIcon::UNIVERSAL, ImGuizmo::OPERATION::UNIVERSAL, "Move + Rotate + Scale (T)" },
        };
        Editor* editor = static_cast<Editor*>(Core::GetClient());
        Scope<GraphicsRHI>& rhi = Core::GetRenderer()->GetRHI();

        // Highlighted like the selection in the Hierarchy
        auto toggleButton = [](const char* label, bool active, const char* tooltip) -> bool {
            if(active)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGuiAux::Colors::ThemeColor2);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGuiAux::Colors::ThemeColor2);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            }
            const bool pressed = ImGui::Button(label);
            if(active)
                ImGui::PopStyleColor(3);
            ImGuiAux::ItemTooltip(tooltip);
            return pressed;
        };

        ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(15, 15, 15, 200));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(15, 15, 15, 200));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2.0f, 0.0f));

        // Tool icons, as tall as the aspect combo next to them
        const float frameHeight = ImGui::GetFrameHeight();
        const ImVec2 toolButtonSize(frameHeight * 1.15f, frameHeight);
        for(const Tool& tool : sTools)
        {
            const bool active = settings.GizmoOperation == tool.Operation;
            if(active)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGuiAux::Colors::ThemeColor2);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGuiAux::Colors::ThemeColor2);
            }

            ImGui::PushID(tool.Operation);
            const ImVec4 tint = active ? ImVec4(0.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.85f, 0.85f, 0.85f, 1.0f);
            if(GizmoIcons::IconButton("##Tool", rhi->GetImGuiImage(editor->GetToolIcon(tool.Icon)), toolButtonSize, tint, 0.9f))
                settings.GizmoOperation = tool.Operation;
            ImGui::PopID();

            if(active)
                ImGui::PopStyleColor(2);
            ImGuiAux::ItemTooltip(tool.Tooltip);
            ImGui::SameLine();
        }

        ImGui::SameLine(0.0f, 8.0f);
        if(toggleButton(settings.GizmoWorldSpace ? "World" : "Local", false, "Gizmo space: local / world (X)"))
            settings.GizmoWorldSpace = !settings.GizmoWorldSpace;

        ImGui::SameLine(0.0f, 8.0f);
        if(toggleButton("Snap", settings.SnapEnabled, "Snapping, holding Ctrl inverts it"))
            settings.SnapEnabled = !settings.SnapEnabled;
        ImGui::SameLine();
        if(ImGui::ArrowButton("##SnapSettings", ImGuiDir_Down))
            ImGui::OpenPopup("SnapSettingsPopup");

        ImGui::SameLine(0.0f, 8.0f);
        if(toggleButton("Camera", false, "Editor camera settings"))
            ImGui::OpenPopup("CameraSettingsPopup");

        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);

        ImGuiAux::StyledPopupVars::Push();
        if(ImGui::BeginPopup("SnapSettingsPopup"))
        {
            ImGui::SetNextItemWidth(120.0f);
            ImGui::DragFloat("Move", &settings.SnapTranslate, 0.05f, 0.001f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SetNextItemWidth(120.0f);
            ImGui::DragFloat("Rotate (deg)", &settings.SnapRotate, 1.0f, 0.01f, 360.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::SetNextItemWidth(120.0f);
            ImGui::DragFloat("Scale", &settings.SnapScale, 0.01f, 0.001f, 100.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::EndPopup();
        }
        if(ImGui::BeginPopup("CameraSettingsPopup"))
        {
            float speed = mEditorCam->GetCameraSpeed();
            ImGui::SetNextItemWidth(160.0f);
            if(ImGui::SliderFloat("Fly Speed", &speed, 0.0005f, 2.0f, "%.4f", ImGuiSliderFlags_Logarithmic))
                mEditorCam->SetCameraSpeed(speed);
            ImGui::TextDisabled("RMB + scroll changes it while flying");

            if(ImGui::Button("Reset View"))
                mEditorCam->SetView(glm::vec3(0.0f), glm::length(glm::vec3(-5.0f, 5.0f, 5.0f)), glm::quarter_pi<float>(), 3.0f * glm::quarter_pi<float>()); // EditorCamera's initial view
            ImGui::SameLine();
            if(ImGui::Button("Top"))
                mEditorCam->SetView(mEditorCam->GetFocalPoint(), mEditorCam->GetDistance(), glm::half_pi<float>() - 0.01f, 0.0f);
            ImGui::SameLine();
            if(ImGui::Button("Front"))
                mEditorCam->SetView(mEditorCam->GetFocalPoint(), mEditorCam->GetDistance(), 0.0f, 0.0f);
            ImGui::SameLine();
            if(ImGui::Button("Side"))
                mEditorCam->SetView(mEditorCam->GetFocalPoint(), mEditorCam->GetDistance(), 0.0f, glm::half_pi<float>());
            ImGui::EndPopup();
        }
        ImGuiAux::StyledPopupVars::Pop();
    }

    void ViewportPanel::Shutdown() {}

    void ViewportPanel::SetSceneName()
    {
        Scene* scene = mSceneHierarchy->GetSceneContext();
        AssetID assetID = scene ? scene->GetID() : AssetID(AssetID::INVALID);
        if(assetID)
            mSceneName = Filesystem::GetFilenameWithExt(Core::GetAssetManager()->GetMetadata(assetID).RelativePath);
        else
            mSceneName = "Untitled Scene";
    }

} // namespace Surge