// Copyright (c) - SurgeTechnologies - All rights reserved
#include "InspectorPanel.hpp"
#include "Surge/Core/Core.hpp"

#include "Surge/ECS/Components/Components.hpp"
#include "Surge/ECS/Components/ScriptComponent.hpp"

#include "Surge/Graphics/HighLevel/Mesh.hpp"
#include "Surge/Graphics/HighLevel/Font.hpp"

#include "Surge/Asset/AssetManager.hpp"

#include "Surge/ScriptEngine/ScriptAsset.hpp"
#include "Surge/Audio/Audio.hpp"
#include "Surge/Serializer/Serializer.hpp"

#include "Editor.hpp"
#include "MaterialEditorPanel.hpp"
#include "ContentBrowserPanel.hpp"
#include "UIEditorPanel.hpp"
#include "Surge/Graphics/UISystem/UILayout.hpp"
#include "Utility/ImGuiAux.hpp"
#include "Gizmos/GizmoIcons.hpp"
#include "StatusBar/EditorStatusBar.hpp"
#include "Scripting/ScriptFields.hpp"
#include "Surge/ScriptEngine/ScriptEngine.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <imgui_internal.h>
#include <json/json.hpp>
#include "Surge/Utility/Filesystem.hpp"
#include "Surge/Audio/AudioEngine.hpp"
#include <algorithm>

namespace Surge
{
    namespace
    {
        constexpr ImU32 DISABLED_GLYPH = IM_COL32(110, 110, 110, 255);
        constexpr float HEADER_GRADIENT_WIDTH = 0.4f;  // Fraction of the header the component color fades over
        constexpr float HEADER_GRADIENT_ALPHA = 0.44f; // Opacity at the right edge
        constexpr float OUTLINE_THICKNESS = 1.5f;
        const ImVec4 SLOT_BG(0.2f, 0.2f, 0.2f, 1.0f);
        const ImVec4 SLOT_BG_HOVERED(0.07f, 0.07f, 0.07f, 1.0f);
        const ImVec4 SLOT_BG_ACTIVE(0.11f, 0.11f, 0.11f, 1.0f);
        const ImVec4 SLOT_BORDER(0.30f, 0.30f, 0.30f, 1.0f);
        const ImVec4 MUTED_TEXT(0.55f, 0.55f, 0.55f, 1.0f);

        ImU32 WithAlpha(ImU32 color, float alpha)
        {
            return (color & ~IM_COL32_A_MASK) | (static_cast<ImU32>(alpha * 255.0f + 0.5f) << IM_COL32_A_SHIFT);
        }

        void MenuSection(const char* title)
        {
            ImGuiAux::StyledSeparator();
            ImGuiAux::ScopedBoldFont font;
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", title);
            ImGuiAux::StyledSeparator();
        }

        // ------------------------------------------------------------------ Component identity (header glyph + accent color)

        struct ComponentStyle
        {
            GizmoIcon Icon;
            ImU32 Color;
        };

        constexpr ImU32 PHYSICS_COLOR = IM_COL32(255, 125, 120, 255);
        constexpr ImU32 COLLIDER_COLOR = IM_COL32(120, 215, 130, 255);
        constexpr ImU32 AUDIO_COLOR = IM_COL32(110, 215, 160, 255);

        template <typename T>
        ComponentStyle StyleOf()
        {
            if constexpr(std::is_same_v<T, TransformComponent>)              return { GizmoIcon::TRANSFORM, IM_COL32(205, 205, 205, 255) };
            else if constexpr(std::is_same_v<T, CameraComponent>)            return { GizmoIcon::CAMERA, IM_COL32(130, 190, 255, 255) };
            else if constexpr(std::is_same_v<T, SpriteRendererComponent>)    return { GizmoIcon::SPRITE, IM_COL32(255, 145, 95, 255) };
            else if constexpr(std::is_same_v<T, MeshComponent>)              return { GizmoIcon::MESH, IM_COL32(150, 160, 255, 255) };
            else if constexpr(std::is_same_v<T, LightComponent>)             return { GizmoIcon::POINT_LIGHT, IM_COL32(255, 205, 90, 255) };
            else if constexpr(std::is_same_v<T, EnvironmentComponent>)       return { GizmoIcon::ENVIRONMENT, IM_COL32(255, 170, 100, 255) };
            else if constexpr(std::is_same_v<T, TextComponent>)              return { GizmoIcon::TEXT, IM_COL32(225, 225, 225, 255) };
            else if constexpr(std::is_same_v<T, RigidbodyComponent>)         return { GizmoIcon::RIGIDBODY, PHYSICS_COLOR };
            else if constexpr(std::is_same_v<T, ScriptComponent>)            return { GizmoIcon::SCRIPT, IM_COL32(185, 150, 255, 255) };
            else if constexpr(std::is_same_v<T, UICanvasComponent>)          return { GizmoIcon::UI_CANVAS, IM_COL32(240, 130, 190, 255) };
            else if constexpr(std::is_same_v<T, AudioSourceComponent>)       return { GizmoIcon::AUDIO_SOURCE, AUDIO_COLOR };
            else if constexpr(std::is_same_v<T, AudioListenerComponent>)     return { GizmoIcon::AUDIO_LISTENER, AUDIO_COLOR };
            else                                                             return { GizmoIcon::COLLIDER, COLLIDER_COLOR }; // Colliders
        }

        template <typename T>
        bool HasComponentOf(Entity e) { return e.HasComponent<T>(); }

        // Components a user sees in the Inspector (ID/Name/Relationship are always there and not shown as components)
        template <typename... Ts>
        int CountComponents(Entity e) { return (static_cast<int>(e.HasComponent<Ts>()) + ...); }

        int CountVisibleComponents(Entity e)
        {
            return CountComponents<TransformComponent, CameraComponent, SpriteRendererComponent, MeshComponent, LightComponent, EnvironmentComponent,
                                   RigidbodyComponent, BoxColliderComponent, SphereColliderComponent, CapsuleColliderComponent, CylinderColliderComponent,
                                   ConvexColliderComponent, MeshColliderComponent, ScriptComponent, TextComponent, UICanvasComponent,
                                   AudioListenerComponent, AudioSourceComponent>(e);
        }

        // ------------------------------------------------------------------ Component clipboard
        // Held as serialized JSON (the format undo uses): no asset Refs kept alive in a static, runtime handles never get copied

        struct ComponentClipboard
        {
            String Key; // Reflection class name, the component's key in Serializer::SerializeEntity()
            String DisplayName;
            String Json;
            bool (*Has)(Entity) = nullptr;

            bool IsValid() const { return !Json.empty(); }
        };
        ComponentClipboard sComponentClipboard;

        template <typename T>
        const char* ComponentKey() { return SurgeReflect::GetReflection<T>()->GetName().c_str(); }

        template <typename T>
        void CopyComponent(Entity entity, const char* displayName)
        {
            const nlohmann::json state = nlohmann::json::parse(Serializer::SerializeEntity(entity), nullptr, false);
            const char* key = ComponentKey<T>();
            if(state.is_discarded() || !state.contains(key))
                return;
            sComponentClipboard = { key, displayName, state[key].dump(), &HasComponentOf<T> };
        }

        // Applies at frame end: deserializing may add the component, nothing may hold component references then.
        // The selection tracker sees the change like any other edit (undoable)
        void PasteComponent(Entity entity)
        {
            Core::AddFrameEndCallback([entity, clipboard = sComponentClipboard]() {
                if(!entity)
                    return;
                nlohmann::json state = nlohmann::json::parse(Serializer::SerializeEntity(entity), nullptr, false);
                nlohmann::json value = nlohmann::json::parse(clipboard.Json, nullptr, false);
                if(state.is_discarded() || value.is_discarded())
                    return;
                state[clipboard.Key] = std::move(value);
                Serializer::DeserializeEntity(state.dump(), entity);
            });
        }

        // Script-driven components own live Lua instances, everything else can be reset to its defaults in edit mode
        template <typename T>
        constexpr bool IsResettable_v = !std::is_same_v<T, ScriptComponent> && !std::is_same_v<T, UICanvasComponent>;

        template <typename T>
        void ResetComponent(T& component)
        {
            if constexpr(std::is_same_v<T, SpriteRendererComponent>)
                component = SpriteRendererComponent(glm::vec4(1.0f));
            else
                component = T {};

            if constexpr(std::is_same_v<T, TransformComponent>)
                component.MarkDirty();
        }

        template <typename, typename = void>
        constexpr bool HasActiveProperty_v = false;
        template <typename T>
        constexpr bool HasActiveProperty_v<T, std::void_t<decltype(std::declval<T>().Active)>> = true;

        // ------------------------------------------------------------------ Property widgets

        // Label column, X/Y/Z fields with a colored axis tag (click: reset that axis), optional uniform lock (scale)
        bool DrawVec3Control(const char* label, glm::vec3& values, float resetValue, float speed, bool* uniformLock = nullptr)
        {
            bool modified = false;
            ImGui::PushID(label);

            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);
            ImGui::TableNextColumn();

            const float frameHeight = ImGui::GetFrameHeight();
            const float spacing = 4.0f;
            const float lockWidth = uniformLock ? frameHeight + spacing : 0.0f;
            const float fieldWidth = glm::max((ImGui::GetContentRegionAvail().x - lockWidth - spacing * 2.0f) / 3.0f, 30.0f);
            const float tagWidth = glm::min(frameHeight * 0.85f, fieldWidth * 0.4f);
            const float rounding = ImGui::GetStyle().FrameRounding;
            const bool locked = uniformLock && *uniformLock;
            const glm::vec3 before = values;

            constexpr const char* axisNames[] = { "X", "Y", "Z" };
            constexpr ImU32 axisColors[] = { IM_COL32(200, 70, 70, 255), IM_COL32(80, 170, 80, 255), IM_COL32(70, 115, 210, 255) };

            int changedAxis = -1;
            bool wasReset = false;
            for(int axis = 0; axis < 3; axis++)
            {
                ImGui::PushID(axis);

                // Axis tag
                if(ImGui::InvisibleButton("##Tag", ImVec2(tagWidth, frameHeight)))
                {
                    values[axis] = resetValue;
                    changedAxis = axis;
                    wasReset = true;
                }
                ImGuiAux::DelayedToolTip("Click to reset");
                const ImVec2 tagMin = ImGui::GetItemRectMin();
                const ImVec2 tagMax = ImGui::GetItemRectMax();
                const ImU32 tagColor = ImGui::IsItemHovered() ? WithAlpha(axisColors[axis], 0.85f) : axisColors[axis];
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                drawList->AddRectFilled(tagMin, tagMax, tagColor, rounding, ImDrawFlags_RoundCornersLeft);
                {
                    ImGuiAux::ScopedBoldFont font;
                    const ImVec2 textSize = ImGui::CalcTextSize(axisNames[axis]);
                    drawList->AddText(ImVec2((tagMin.x + tagMax.x - textSize.x) * 0.5f, (tagMin.y + tagMax.y - textSize.y) * 0.5f), IM_COL32(245, 245, 245, 255), axisNames[axis]);
                }

                // Value
                ImGui::SameLine(0.0f, 0.0f);
                ImGui::SetNextItemWidth(fieldWidth - tagWidth);
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
                if(ImGui::DragFloat("##Value", &values[axis], speed, 0.0f, 0.0f, "%.2f"))
                    changedAxis = axis;
                ImGui::PopStyleVar();

                if(ImGui::IsItemHovered() || ImGui::IsItemActive())
                    ImGuiAux::DrawRectAroundWidget(ImGuiAux::Colors::ThemeColor2, OUTLINE_THICKNESS, 1.0f);

                ImGui::PopID();
                if(axis < 2)
                    ImGui::SameLine(0.0f, spacing);
            }

            if(changedAxis >= 0)
            {
                modified = true;
                if(locked && wasReset)
                    values = glm::vec3(resetValue);
                else if(locked)
                {
                    // Keep the proportions: every axis scales by the edited axis' factor
                    const float previous = before[changedAxis];
                    if(glm::abs(previous) > 1e-6f)
                        values = before * (values[changedAxis] / previous);
                    else
                        values = glm::vec3(values[changedAxis]);
                }
            }

            if(uniformLock)
            {
                ImGui::SameLine(0.0f, spacing);
                const ImVec4 glyphColor = locked ? ImVec4(ImGuiAux::Colors::ThemeColor1) : MUTED_TEXT;
                if(GizmoIcons::IconButton("##UniformLock", GizmoIcon::LINK, ImVec2(frameHeight, frameHeight), glyphColor, 0.66f))
                    *uniformLock = !*uniformLock;
                ImGuiAux::DelayedToolTip(locked ? "Uniform scale: ON" : "Uniform scale: OFF");
            }

            ImGui::PopID();
            return modified;
        }

        template <typename T>
        bool DrawAssetDropSlot(const char* label, Ref<Asset>& assetPtr, AssetType expectedType, const char* dropHint)
        {
            bool modified = false;
            AssetManager* am = Core::GetAssetManager();
            const bool hasAsset = static_cast<bool>(assetPtr);

            ImGui::PushID(label); // Several slots per component (UI Canvas: Layout + Script), each needs its own button IDs
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(label);
            ImGui::TableNextColumn();

            String buttonText = dropHint;
            if(hasAsset)
                buttonText = Filesystem::GetFilenameWithExt(am->GetMetadata(assetPtr->GetID()).RelativePath);

            // Highlight while a matching asset is dragged over the Inspector
            bool acceptsDrag = false;
            if(const ImGuiPayload* drag = ImGui::GetDragDropPayload(); drag && drag->IsDataType(CONTENT_BROWSER_PAYLOAD))
                acceptsDrag = am->GetMetadata(*static_cast<const AssetID*>(drag->Data)).Type == expectedType;

            ImGuiAux::ScopedBoldFont font;
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, OUTLINE_THICKNESS);
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));

            const float clearWidth = hasAsset ? ImGui::GetFrameHeight() : 0.0f;
            const float spacing = hasAsset ? 4.0f : 0.0f;
            const float slotWidth = ImGui::GetContentRegionAvail().x - clearWidth - spacing;

            const ImVec4 border = acceptsDrag ? ImVec4(ImGuiAux::Colors::ThemeColor1) : (hasAsset ? SLOT_BORDER : ImVec4(0.65f, 0.25f, 0.25f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Border, border);
            ImGui::PushStyleColor(ImGuiCol_Button, SLOT_BG);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, SLOT_BG_HOVERED);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, SLOT_BG_ACTIVE);
            ImGui::PushStyleColor(ImGuiCol_Text, hasAsset ? ImGui::GetStyleColorVec4(ImGuiCol_Text) : ImVec4(0.85f, 0.45f, 0.45f, 1.0f));

            if(ImGui::Button(buttonText.c_str(), ImVec2(slotWidth, 0.0f)) && hasAsset)
            {
                Editor* editor = static_cast<Editor*>(Core::GetClient());
                editor->GetPanelManager().GetPanel<ContentBrowserPanel>()->SetSelectedAsset(assetPtr->GetID());
            }
            ImGui::PopStyleColor(5);
            ImGuiAux::DelayedToolTip(hasAsset ? "Click: show in the Content Browser\nDrop another asset here to replace it" : "Drop an asset from the Content Browser");

            if(ImGui::BeginDragDropTarget())
            {
                if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(CONTENT_BROWSER_PAYLOAD))
                {
                    SG_ASSERT(payload->DataSize == sizeof(AssetID), "Payload size mismatch!");
                    const AssetID droppedAssetID = *static_cast<const AssetID*>(payload->Data);
                    if(am->GetMetadata(droppedAssetID).Type == expectedType)
                    {
                        assetPtr = am->Load<T>(droppedAssetID);
                        modified = true;
                    }
                    else
                        EditorStatusBar::Get().Notify(Severity::Warn, std::format("{} expects a {} asset", label, SurgeReflect::EnumToString(expectedType)));
                }
                ImGui::EndDragDropTarget();
            }

            if(hasAsset)
            {
                ImGui::SameLine(0.0f, spacing);
                ImGui::PushStyleColor(ImGuiCol_Button, SLOT_BG);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.18f, 0.18f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Border, SLOT_BORDER);
                ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.5f, 0.5f));
                if(ImGui::Button("x", ImVec2(clearWidth, 0.0f)))
                {
                    assetPtr = nullptr;
                    modified = true;
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor(3);
                ImGuiAux::DelayedToolTip("Clear");
            }

            ImGui::PopStyleVar(3);
            ImGui::PopID();
            return modified;
        }

        // An explanatory line in the value column of a component table
        void HintRow(const char* text, const ImVec4& color = MUTED_TEXT)
        {
            ImGui::TableNextColumn();
            ImGui::TableNextColumn();
            ImGui::PushTextWrapPos(0.0f);
            ImGui::TextColored(color, "%s", text);
            ImGui::PopTextWrapPos();
        }

        // ------------------------------------------------------------------ Component header + options

        template <typename XComponent>
        bool DrawComponentMenu(Entity entity, XComponent& component, const char* name, bool removable)
        {
            const bool playing = static_cast<Editor*>(Core::GetClient())->IsPlaying();
            bool remove = false;

            {
                ImGuiAux::ScopedBoldFont font;
                ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", name);
            }
            ImGuiAux::StyledSeparator();

            if constexpr(std::is_same_v<XComponent, TransformComponent>)
            {
                if(ImGuiAux::StyledMenuItem("Reset"))
                    ResetComponent(component);
                if(ImGuiAux::StyledMenuItem("Reset Position"))
                {
                    component.Position = glm::vec3(0.0f);
                    component.MarkDirty();
                }
                if(ImGuiAux::StyledMenuItem("Reset Rotation"))
                {
                    component.Rotation = glm::vec3(0.0f);
                    component.MarkDirty();
                }
                if(ImGuiAux::StyledMenuItem("Reset Scale"))
                {
                    component.Scale = glm::vec3(1.0f);
                    component.MarkDirty();
                }
                ImGuiAux::StyledSeparator();
            }
            else if constexpr(IsResettable_v<XComponent>)
            {
                if(ImGuiAux::StyledMenuItem("Reset", nullptr, nullptr, !playing))
                    ResetComponent(component);
                ImGuiAux::StyledSeparator();
            }

            if(ImGuiAux::StyledMenuItem("Copy"))
                CopyComponent<XComponent>(entity, name);
            const bool canPaste = !playing && sComponentClipboard.IsValid() && sComponentClipboard.Key == ComponentKey<XComponent>();
            if(ImGuiAux::StyledMenuItem("Paste Values", nullptr, nullptr, canPaste))
                PasteComponent(entity);

            if(removable)
            {
                ImGuiAux::StyledSeparator();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.45f, 0.45f, 1.0f));
                if(ImGuiAux::StyledMenuItem("Remove Component"))
                    remove = true;
                ImGui::PopStyleColor();
            }
            return remove;
        }

        // Kebab "more" button, three dots drawn in place
        bool OptionsButton(const char* strId, float size)
        {
            const bool pressed = ImGui::InvisibleButton(strId, ImVec2(size, size));
            const ImVec2 min = ImGui::GetItemRectMin();
            const ImVec2 max = ImGui::GetItemRectMax();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            if(ImGui::IsItemHovered())
                drawList->AddRectFilled(min, max, IM_COL32(255, 255, 255, 22), ImGui::GetStyle().FrameRounding);

            const ImVec2 center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
            const float gap = size * 0.18f;
            const float radius = glm::max(1.5f, size * 0.06f);
            const ImU32 color = ImGui::GetColorU32(ImGui::IsItemHovered() ? ImGuiCol_Text : ImGuiCol_TextDisabled);
            for(int i = -1; i <= 1; i++)
                drawList->AddCircleFilled(ImVec2(center.x, center.y + gap * static_cast<float>(i)), radius, color, 8);
            return pressed;
        }

        template <typename XComponent, bool IsRemoveable = true, typename Func>
        void DrawComponent(Entity& entity, const char* name, Func&& function)
        {
            const int64_t hash = SurgeReflect::GetReflection<XComponent>()->GetHash();
            ImGui::PushID(static_cast<int>(hash));

            XComponent& component = entity.GetComponent<XComponent>();
            ComponentStyle style = StyleOf<XComponent>();
            if constexpr(std::is_same_v<XComponent, LightComponent>)
                style.Icon = component.Type == LightType::DIRECTIONAL ? GizmoIcon::DIRECTIONAL_LIGHT : GizmoIcon::POINT_LIGHT;

            bool active = true;
            if constexpr(HasActiveProperty_v<XComponent>)
                active = component.Active;

            // Header: framed tree node (keeps the open state per component type), everything else is drawn on top of it
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 5.0f));

            const float labelX = ImGui::GetCursorScreenPos().x + ImGui::GetTreeNodeToLabelSpacing();
            const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth |
                                             ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;
            const bool open = ImGui::TreeNodeEx(reinterpret_cast<void*>(hash), flags, "%s", ""); // Label drawn below, with the icon
            const bool openMenuFromHeader = ImGui::IsItemClicked(ImGuiMouseButton_Right);
            const ImVec2 headerMin = ImGui::GetItemRectMin();
            const ImVec2 headerMax = ImGui::GetItemRectMax();

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const float fontSize = ImGui::GetFontSize();
            const float centerY = (headerMin.y + headerMax.y) * 0.5f;
            const float headerHeight = headerMax.y - headerMin.y;

            // Component color fading out from the right edge, glyph and name
            {
                const ImU32 gradientColor = active ? style.Color : DISABLED_GLYPH;
                const ImU32 left = WithAlpha(gradientColor, 0.0f);
                const ImU32 right = WithAlpha(gradientColor, HEADER_GRADIENT_ALPHA);
                drawList->AddRectFilledMultiColor(ImVec2(headerMax.x - (headerMax.x - headerMin.x) * HEADER_GRADIENT_WIDTH, headerMin.y), headerMax, left, right, right, left);
            }
            GizmoIcons::DrawGlyph(drawList, ImVec2(labelX + fontSize * 0.5f, centerY), fontSize * 0.9f, style.Icon, active ? style.Color : DISABLED_GLYPH, ImGui::GetColorU32(ImGuiCol_Header));
            {
                ImGuiAux::ScopedBoldFont font;
                const ImU32 textColor = ImGui::GetColorU32(active ? ImGuiCol_Text : ImGuiCol_TextDisabled);
                drawList->AddText(ImVec2(labelX + fontSize + 7.0f, centerY - fontSize * 0.5f), textColor, name);
            }

            // Right side: active toggle, options
            bool openMenu = openMenuFromHeader;
            float rightX = headerMax.x - 2.0f;
            {
                const float buttonSize = headerHeight - 4.0f;
                rightX -= buttonSize;
                ImGui::SetCursorScreenPos(ImVec2(rightX, headerMin.y + 2.0f));
                if(OptionsButton("##Options", buttonSize))
                    openMenu = true;
            }
            if constexpr(HasActiveProperty_v<XComponent>)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f, 3.0f));
                const float checkboxSize = ImGui::GetFrameHeight();
                rightX -= checkboxSize + 4.0f;
                ImGui::SetCursorScreenPos(ImVec2(rightX, centerY - checkboxSize * 0.5f));
                ImGui::Checkbox("##Active", &component.Active);
                ImGuiAux::DelayedToolTip(component.Active ? "Enabled" : "Disabled");
                ImGui::PopStyleVar();
            }
            ImGui::SetCursorScreenPos(ImVec2(headerMin.x, headerMax.y + ImGui::GetStyle().ItemSpacing.y));
            ImGui::PopStyleVar();

            if(openMenu)
                ImGui::OpenPopup("##ComponentMenu");

            bool removeComponent = false;
            ImGuiAux::StyledPopupVars::Push();
            if(ImGui::BeginPopup("##ComponentMenu"))
            {
                removeComponent = DrawComponentMenu(entity, component, name, IsRemoveable);
                ImGuiAux::EndStyledPopup();
            }
            else
                ImGuiAux::StyledPopupVars::Pop();

            if(open)
            {
                ImGui::Dummy(ImVec2(0.0f, 1.0f));
                ImGui::Indent(6.0f);
                const float labelWidth = glm::clamp(ImGui::GetContentRegionAvail().x * 0.38f, 90.0f, 170.0f);
                if(ImGui::BeginTable("##ComponentTable", 2, ImGuiTableFlags_SizingFixedFit))
                {
                    ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, labelWidth);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    if(!active)
                        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.6f); // Still editable, just reads as off
                    function();
                    if(!active)
                        ImGui::PopStyleVar();

                    ImGui::EndTable();
                }
                ImGui::Unindent(6.0f);
                ImGui::Dummy(ImVec2(0.0f, 6.0f));
                ImGui::TreePop();
            }
            else
                ImGui::Dummy(ImVec2(0.0f, 2.0f));

            if constexpr(IsRemoveable)
            {
                if(removeComponent)
                    Core::AddFrameEndCallback([entity]() mutable {
                        if(entity && entity.HasComponent<XComponent>())
                            entity.RemoveComponent<XComponent>();
                    });
            }

            ImGui::PopID();
        }

        // ------------------------------------------------------------------ Add Component catalog

        struct AddableComponent
        {
            const char* Name;
            const char* Category;
            ComponentStyle Style;
            bool (*Has)(Entity);
            void (*Add)(Entity);
            const char* (*MissingRequirement)(Entity); // nullptr = always addable
        };

        template <typename T>
        void AddDefault(Entity e) { e.AddComponent<T>(); }

        const char* RequiresMesh(Entity e) { return e.HasComponent<MeshComponent>() ? nullptr : "Requires a Mesh component"; }

        const AddableComponent sAddableComponents[] = {
            { "Camera", "RENDERING", StyleOf<CameraComponent>(), &HasComponentOf<CameraComponent>, &AddDefault<CameraComponent>, nullptr },
            { "Sprite Renderer", "RENDERING", StyleOf<SpriteRendererComponent>(), &HasComponentOf<SpriteRendererComponent>,
              [](Entity e) { e.AddComponent<SpriteRendererComponent>(glm::vec4(1.0f)); }, nullptr },
            { "Mesh", "RENDERING", StyleOf<MeshComponent>(), &HasComponentOf<MeshComponent>, &AddDefault<MeshComponent>, nullptr },
            { "Light", "RENDERING", StyleOf<LightComponent>(), &HasComponentOf<LightComponent>, &AddDefault<LightComponent>, nullptr },
            { "Environment", "RENDERING", StyleOf<EnvironmentComponent>(), &HasComponentOf<EnvironmentComponent>, &AddDefault<EnvironmentComponent>, nullptr },
            { "Text", "RENDERING", StyleOf<TextComponent>(), &HasComponentOf<TextComponent>, &AddDefault<TextComponent>, nullptr },

            { "Rigidbody", "PHYSICS", StyleOf<RigidbodyComponent>(), &HasComponentOf<RigidbodyComponent>, &AddDefault<RigidbodyComponent>, nullptr },
            { "Box Collider", "PHYSICS", StyleOf<BoxColliderComponent>(), &HasComponentOf<BoxColliderComponent>, &AddDefault<BoxColliderComponent>, nullptr },
            { "Sphere Collider", "PHYSICS", StyleOf<SphereColliderComponent>(), &HasComponentOf<SphereColliderComponent>, &AddDefault<SphereColliderComponent>, nullptr },
            { "Capsule Collider", "PHYSICS", StyleOf<CapsuleColliderComponent>(), &HasComponentOf<CapsuleColliderComponent>, &AddDefault<CapsuleColliderComponent>, nullptr },
            { "Cylinder Collider", "PHYSICS", StyleOf<CylinderColliderComponent>(), &HasComponentOf<CylinderColliderComponent>, &AddDefault<CylinderColliderComponent>, nullptr },
            { "Convex Collider", "PHYSICS", StyleOf<ConvexColliderComponent>(), &HasComponentOf<ConvexColliderComponent>, &AddDefault<ConvexColliderComponent>, &RequiresMesh },
            { "Mesh Collider", "PHYSICS", StyleOf<MeshColliderComponent>(), &HasComponentOf<MeshColliderComponent>, &AddDefault<MeshColliderComponent>, &RequiresMesh },

            { "Script", "SCRIPTING & UI", StyleOf<ScriptComponent>(), &HasComponentOf<ScriptComponent>,
              [](Entity e) { e.AddComponent<ScriptComponent>().Scripts.emplace_back(); }, nullptr }, // One empty slot to drop a script on
            { "UI Canvas", "SCRIPTING & UI", StyleOf<UICanvasComponent>(), &HasComponentOf<UICanvasComponent>, &AddDefault<UICanvasComponent>, nullptr },

            { "Audio Source", "AUDIO", StyleOf<AudioSourceComponent>(), &HasComponentOf<AudioSourceComponent>, &AddDefault<AudioSourceComponent>, nullptr },
            { "Audio Listener", "AUDIO", StyleOf<AudioListenerComponent>(), &HasComponentOf<AudioListenerComponent>, &AddDefault<AudioListenerComponent>, nullptr },
        };

        // One row of the Add Component list: glyph, name, and why it is unavailable. Returns true when clicked
        bool AddComponentRow(const char* id, const ComponentStyle& style, const char* label, const char* note, bool enabled, bool highlighted)
        {
            const float height = ImGui::GetFrameHeight();
            ImGui::BeginDisabled(!enabled);
            const bool clicked = ImGui::Selectable(id, highlighted, ImGuiSelectableFlags_None, ImVec2(0.0f, height));
            ImGui::EndDisabled();

            const ImVec2 min = ImGui::GetItemRectMin();
            const ImVec2 max = ImGui::GetItemRectMax();
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const float fontSize = ImGui::GetFontSize();
            const float centerY = (min.y + max.y) * 0.5f;

            GizmoIcons::DrawGlyph(drawList, ImVec2(min.x + 4.0f + fontSize * 0.5f, centerY), fontSize * 0.85f, style.Icon, enabled ? style.Color : DISABLED_GLYPH, ImGui::GetColorU32(ImGuiCol_PopupBg));
            drawList->AddText(ImVec2(min.x + fontSize + 12.0f, centerY - fontSize * 0.5f), ImGui::GetColorU32(enabled ? ImGuiCol_Text : ImGuiCol_TextDisabled), label);
            if(note)
            {
                const float noteWidth = ImGui::CalcTextSize(note).x;
                drawList->AddText(ImVec2(max.x - noteWidth - 4.0f, centerY - fontSize * 0.5f), ImGui::GetColorU32(ImGuiCol_TextDisabled), note);
            }
            return clicked;
        }

        bool ContainsCaseInsensitive(std::string_view text, std::string_view lowerNeedle)
        {
            if(lowerNeedle.empty())
                return true;
            auto it = std::search(text.begin(), text.end(), lowerNeedle.begin(), lowerNeedle.end(),
                                  [](char a, char b) { return ::tolower(static_cast<unsigned char>(a)) == b; });
            return it != text.end();
        }
    } // namespace

    void InspectorPanel::Init(void*)
    {
        mCode = GetStaticCode();
        mHierarchy = nullptr;
    }

    void InspectorPanel::Render(bool* show)
    {
        if(!*show)
            return;

        if(ImGui::Begin(PanelCodeToString(mCode), show))
        {
            Entity& entity = mHierarchy->GetSelectedEntity();
            if(!entity)
            {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + glm::max(ImGui::GetContentRegionAvail().y * 0.35f, 8.0f));
                {
                    ImGuiAux::ScopedBoldFont font;
                    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                    ImGuiAux::TextCentered("Nothing selected");
                    ImGui::PopStyleColor();
                }
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                ImGuiAux::TextCentered("Select an entity in the Hierarchy or a Scene view");
                ImGui::PopStyleColor();
            }
            else
            {
                DrawEntityHeader(entity);
                DrawComponents(entity);
                DrawAddComponentButton(entity);
            }
        }
        ImGui::End();
    }

    void InspectorPanel::DrawEntityHeader(Entity& entity)
    {
        const GizmoIcons::EntityIcon icon = GizmoIcons::GetEntityIcon(entity);
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Type glyph
        const float badgeSize = 44.0f;
        const ImVec2 start = ImGui::GetCursorScreenPos();
        const ImVec2 badgeMax(start.x + badgeSize, start.y + badgeSize);
        GizmoIcons::DrawGlyph(drawList, ImVec2(start.x + badgeSize * 0.5f, start.y + badgeSize * 0.5f), badgeSize * 0.7f, icon.Icon, icon.Color, ImGui::GetColorU32(ImGuiCol_WindowBg));

        // Name, ID and component 
        ImGui::SetCursorScreenPos(ImVec2(badgeMax.x + 8.0f, start.y));
        ImGui::BeginGroup();
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 4.0f));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            ImGui::SetNextItemWidth(-FLT_MIN);
            {
                ImGuiAux::ScopedBoldFont font;
                ImGui::InputTextWithHint("##EntityName", "Name", &entity.GetComponent<NameComponent>().Name);
            }
            ImGui::PopStyleVar(2);

            const uint64_t id = entity.GetComponent<IDComponent>().ID.Get();
            ImGui::TextDisabled("ID %llu", static_cast<unsigned long long>(id));
            if(ImGui::IsItemHovered())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                ImGuiAux::DelayedToolTip("Click to copy");
            }
            if(ImGui::IsItemClicked())
            {
                ImGui::SetClipboardText(std::to_string(id).c_str());
                EditorStatusBar::Get().Notify(Severity::Info, "Entity ID copied to the clipboard", 2.0f);
            }

            const int count = CountVisibleComponents(entity);
            ImGui::SameLine();
            ImGui::TextDisabled("|  %d component%s", count, count == 1 ? "" : "s");
        }
        ImGui::EndGroup();

        // Below whichever is taller
        ImGui::SetCursorScreenPos(ImVec2(start.x, glm::max(badgeMax.y, ImGui::GetItemRectMax().y) + 6.0f));

        ImGui::Separator();
        ImGui::Dummy(ImVec2(0.0f, 2.0f));
    }

    void InspectorPanel::DrawAddComponentButton(Entity& entity)
    {
        ImGui::Dummy(ImVec2(0.0f, 6.0f));

        const float width = glm::min(ImGui::GetContentRegionAvail().x, 240.0f);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - width) * 0.5f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 7.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, SLOT_BG);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, SLOT_BG_HOVERED);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, SLOT_BG_ACTIVE);
        bool openPopup = false;
        {
            ImGuiAux::ScopedBoldFont font;
            openPopup = ImGui::Button("+  Add Component", ImVec2(width, 0.0f));
        }
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        if(openPopup)
        {
            mAddComponentSearch.clear();
            ImGui::OpenPopup("##AddComponent");
        }

        // Below the button, as wide as it
        const ImVec2 buttonMin = ImGui::GetItemRectMin();
        ImGui::SetNextWindowPos(ImVec2(buttonMin.x, ImGui::GetItemRectMax().y + 2.0f), ImGuiCond_Appearing);
        ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(glm::max(width, 260.0f), 420.0f));

        ImGuiAux::StyledPopupVars::Push();
        if(!ImGui::BeginPopup("##AddComponent"))
        {
            ImGuiAux::StyledPopupVars::Pop();
            ImGui::Dummy(ImVec2(0.0f, 8.0f));
            return;
        }

        if(ImGui::IsWindowAppearing())
            ImGui::SetKeyboardFocusHere();
        ImGui::SetNextItemWidth(-FLT_MIN);
        const bool enterPressed = ImGui::InputTextWithHint("##Search", "Search components...", &mAddComponentSearch, ImGuiInputTextFlags_EnterReturnsTrue);

        String needle = mAddComponentSearch;
        std::transform(needle.begin(), needle.end(), needle.begin(), [](char c) { return static_cast<char>(::tolower(static_cast<unsigned char>(c))); });
        const bool searching = !needle.empty();

        // The first addable match is highlighted and taken by Enter
        const AddableComponent* firstMatch = nullptr;
        for(const AddableComponent& item : sAddableComponents)
        {
            if(ContainsCaseInsensitive(item.Name, needle) && !item.Has(entity) && !(item.MissingRequirement && item.MissingRequirement(entity)))
            {
                firstMatch = &item;
                break;
            }
        }

        const AddableComponent* toAdd = nullptr;

        // A copied component this entity does not have yet
        if(sComponentClipboard.IsValid() && sComponentClipboard.Has && !sComponentClipboard.Has(entity) && !static_cast<Editor*>(Core::GetClient())->IsPlaying())
        {
            const String label = std::format("Paste {}", sComponentClipboard.DisplayName);
            if(ContainsCaseInsensitive(label, needle))
            {
                ImGuiAux::StyledSeparator();
                if(AddComponentRow("##PasteComponent", ComponentStyle { GizmoIcon::ENTITY, IM_COL32(255, 165, 0, 255) }, label.c_str(), "clipboard", true, false))
                    PasteComponent(entity);
            }
        }

        const char* category = nullptr;
        bool anyShown = false;
        for(const AddableComponent& item : sAddableComponents)
        {
            if(!ContainsCaseInsensitive(item.Name, needle))
                continue;

            if(!searching && (!category || strcmp(category, item.Category) != 0))
            {
                category = item.Category;
                MenuSection(category);
            }
            else if(searching && !anyShown)
                ImGuiAux::StyledSeparator();
            anyShown = true;

            const bool has = item.Has(entity);
            const char* missing = (!has && item.MissingRequirement) ? item.MissingRequirement(entity) : nullptr;
            ImGui::PushID(item.Name);
            if(AddComponentRow("##Item", item.Style, item.Name, has ? "added" : nullptr, !has && !missing, &item == firstMatch && searching))
                toAdd = &item;
            if(missing)
                ImGuiAux::ItemTooltip(missing);
            ImGui::PopID();
        }

        if(!anyShown)
            ImGui::TextDisabled("No component matches \"%s\"", mAddComponentSearch.c_str());

        if(enterPressed && firstMatch)
            toAdd = firstMatch;
        if(toAdd)
        {
            toAdd->Add(entity);
            ImGui::CloseCurrentPopup();
        }

        ImGuiAux::EndStyledPopup();
        ImGui::Dummy(ImVec2(0.0f, 8.0f));
    }

    void InspectorPanel::DrawComponents(Entity& entity)
    {
        if(entity.HasComponent<TransformComponent>())
        {
            TransformComponent& component = entity.GetComponent<TransformComponent>();
            DrawComponent<TransformComponent, false>(entity, "Transform", [&component, this]() {
                if(DrawVec3Control("Position", component.Position, 0.0f, 0.05f))
                    component.MarkDirty();
                if(DrawVec3Control("Rotation", component.Rotation, 0.0f, 0.5f))
                    component.MarkDirty();
                if(DrawVec3Control("Scale", component.Scale, 1.0f, 0.01f, &mUniformScale))
                    component.MarkDirty();
            });
        }

        if(entity.HasComponent<CameraComponent>())
        {
            CameraComponent& component = entity.GetComponent<CameraComponent>();
            DrawComponent<CameraComponent>(entity, "Camera", [&component]() {
                RuntimeCamera& camera = component.Camera;
                ImGuiAux::TProperty<bool>("Primary", &component.Primary);

                RuntimeCamera::ProjectionType projection = camera.GetProjectionType();
                constexpr auto projectionTypeStrings = std::array { "PERSPECTIVE", "ORTHOGRAPHIC" };
                if(ImGuiAux::TComboBox("Projection", projection, projectionTypeStrings))
                    camera.SetProjectionType(projection);

                if(camera.GetProjectionType() == RuntimeCamera::ProjectionType::Perspective)
                {
                    float verticalFOV = camera.GetPerspectiveVerticalFOV();
                    if(ImGuiAux::TProperty<float>("Vertical FOV", &verticalFOV)) // In degree
                        camera.SetPerspectiveVerticalFOV(verticalFOV);

                    float nearClip = camera.GetPerspectiveNearClip();
                    if(ImGuiAux::TProperty<float>("Near Clip", &nearClip))
                        camera.SetPerspectiveNearClip(nearClip);

                    float farClip = camera.GetPerspectiveFarClip();
                    if(ImGuiAux::TProperty<float>("Far Clip", &farClip))
                        camera.SetPerspectiveFarClip(farClip);
                }
                else
                {
                    float orthoSize = camera.GetOrthographicSize();
                    if(ImGuiAux::TProperty<float>("Size", &orthoSize))
                        camera.SetOrthographicSize(orthoSize);

                    float nearClip = camera.GetOrthographicNearClip();
                    if(ImGuiAux::TProperty<float>("Near Clip", &nearClip))
                        camera.SetOrthographicNearClip(nearClip);

                    float farClip = camera.GetOrthographicFarClip();
                    if(ImGuiAux::TProperty<float>("Far Clip", &farClip))
                        camera.SetOrthographicFarClip(farClip);
                }

                ImGuiAux::TSperator("Aspect");
                ImGuiAux::TProperty<bool>("Fixed Aspect Ratio", &component.FixedAspectRatio);
                if(component.FixedAspectRatio)
                {
                    glm::vec2 aspectRatio = camera.GetAspectRatio();
                    if(ImGuiAux::TProperty<glm::vec2>("Aspect Ratio", &aspectRatio))
                        camera.SetAspectRatio(aspectRatio);
                }
            });
        }

        if(entity.HasComponent<SpriteRendererComponent>())
        {
            SpriteRendererComponent& component = entity.GetComponent<SpriteRendererComponent>();
            DrawComponent<SpriteRendererComponent>(entity, "Sprite Renderer", [&component]() {
                DrawAssetDropSlot<Texture2D>("Texture", component.TextureAsset, AssetType::TEXTURE2D, "None (Texture2D)");
                ImGuiAux::TProperty<glm::vec4, ImGuiAux::CustomProprtyFlag::Color4>("Color", &component.Color);
                ImGuiAux::TProperty<bool>("Billboard", &component.Billboard);
            });
        }

        if(entity.HasComponent<MeshComponent>())
        {
            MeshComponent& component = entity.GetComponent<MeshComponent>();
            DrawComponent<MeshComponent>(entity, "Mesh", [&component]() {
                DrawAssetDropSlot<Mesh>("Mesh", component.MeshAsset, AssetType::MESH, "None (Mesh)");
                if(!component.MeshAsset)
                    return;

                ImGuiAux::TProperty<bool>("Drop Shadow", &component.DropShadow);

                Ref<Mesh> mesh = component.MeshAsset;
                ImGuiAux::TSperator("Materials");
                for(size_t i = 0; i < mesh->GetMaterials().size(); i++)
                {
                    ImGui::PushID(static_cast<int>(i));
                    Ref<Material> material = mesh->GetMaterialAtIndex(static_cast<Uint>(i));

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Slot %zu", i);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, OUTLINE_THICKNESS);
                    ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));
                    if(material)
                    {
                        const String label = mesh->HasOverride(static_cast<Uint>(i)) ? std::format("{}  (override)", material->GetName()) : material->GetName();
                        if(ImGui::Button(label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
                        {
                            auto* editor = static_cast<Editor*>(Core::GetClient());
                            editor->GetPanelManager().GetPanel<MaterialEditorPanel>()->SetSelectedMaterial(material);
                            ImGui::SetWindowFocus("Material Editor");
                        }
                        ImGuiAux::DelayedToolTip("Click: edit in the Material Editor\nDrop a MATERIAL here to override this slot");
                    }
                    else
                    {
                        ImGuiAux::ScopedBoldFont font;
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextColored(ImGuiAux::Colors::Red, "MISSING MATERIAL");
                    }
                    ImGui::PopStyleVar(2);

                    if(ImGui::BeginDragDropTarget())
                    {
                        if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(CONTENT_BROWSER_PAYLOAD))
                        {
                            AssetManager* am = Core::GetAssetManager();
                            SG_ASSERT(payload->DataSize == sizeof(AssetID), "Payload size mismatch!");
                            const AssetID droppedAssetID = *static_cast<const AssetID*>(payload->Data);
                            const AssetMetadata& meta = am->GetMetadata(droppedAssetID);

                            if(meta.Type == AssetType::MATERIAL)
                            {
                                mesh->SetMaterialOverride(static_cast<Uint>(i), am->Load<Material>(droppedAssetID));
                                am->Save(meta.ID);
                                am->Save(component.MeshAsset->GetID());
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                    ImGui::PopID();
                }
            });
        }

        if(entity.HasComponent<LightComponent>())
        {
            LightComponent& component = entity.GetComponent<LightComponent>();
            DrawComponent<LightComponent>(entity, "Light", [&component]() {
                constexpr auto lightTypeStrings = std::array { "DIRECTIONAL", "POINT" };
                ImGuiAux::TComboBox("Type", component.Type, lightTypeStrings);
                ImGuiAux::TProperty<glm::vec3, ImGuiAux::CustomProprtyFlag::Color3>("Color", &component.Color);
                ImGuiAux::TProperty<float>("Intensity", &component.Intensity, 0.0f, 1000.0f);

                if(component.Type == LightType::POINT)
                {
                    ImGuiAux::TProperty<float>("Radius", &component.Radius, 0.0f, 10000.0f);
                    ImGuiAux::TSlider<float>("Falloff", &component.Falloff, 0.1f, 2.0f);
                }
                else
                    HintRow("Direction follows the entity's rotation");
            });
        }

        if(entity.HasComponent<EnvironmentComponent>())
        {
            EnvironmentComponent& component = entity.GetComponent<EnvironmentComponent>();
            DrawComponent<EnvironmentComponent>(entity, "Environment", [&component]() {
                ImGuiAux::TSperator("Sun Position");
                ImGuiAux::TProperty<float>("Azimuth", &component.Azimuth);
                ImGuiAux::TProperty<float>("Elevation", &component.Elevation);

                ImGuiAux::TSperator("Sky");
                ImGuiAux::TProperty<bool>("Sun Disk", &component.EnableSunDisk);
                ImGuiAux::TSlider<float>("Turbidity", &component.Turbidity, 1.5f, 10.0f, "%.2f");
                ImGuiAux::TSlider<float>("Exposure", &component.Exposure, 0.001f, 0.1f, "%.4f");
                ImGuiAux::TSlider<float>("Sun Intensity", &component.SunIntensity, 0.01f, 1.0f, "%.2f");

                ImGuiAux::TSperator("Ambient (Fast GI)");
                ImGuiAux::TProperty<glm::vec3, ImGuiAux::CustomProprtyFlag::Color3>("Sky", &component.SkyAmbient);
                ImGuiAux::TProperty<glm::vec3, ImGuiAux::CustomProprtyFlag::Color3>("Horizon", &component.HorizonAmbient);
                ImGuiAux::TProperty<glm::vec3, ImGuiAux::CustomProprtyFlag::Color3>("Ground", &component.GroundAmbient);
            });
        }

        if(entity.HasComponent<RigidbodyComponent>())
        {
            RigidbodyComponent& component = entity.GetComponent<RigidbodyComponent>();
            DrawComponent<RigidbodyComponent>(entity, "Rigidbody", [&component]() {
                constexpr auto rbTypeStrings = std::array { "STATIC", "DYNAMIC", "KINEMATIC" };
                ImGuiAux::TComboBox("Type", component.Type, rbTypeStrings);

                if(component.Type != RigidbodyType::STATIC)
                    ImGuiAux::TProperty<float>("Mass", &component.Mass, 0.0f, 100000.0f);

                ImGuiAux::TSperator("Simulation");
                ImGuiAux::TProperty<bool>("Interpolate", &component.Interpolate);
                ImGuiAux::TProperty<bool>("Use Gravity", &component.UseGravity);
                ImGuiAux::TProperty<bool>("Is Sensor", &component.IsSensor);
                ImGuiAux::TProperty<bool>("Continuous (CCD)", &component.ContinuousCollision);

                ImGuiAux::TSperator("Surface");
                ImGuiAux::TProperty<float>("Friction", &component.Friction, 0.0f, 10.0f);
                ImGuiAux::TProperty<float>("Bounciness", &component.Bounciness, 0.0f, 1.0f);

                if(component.Type != RigidbodyType::STATIC)
                {
                    ImGuiAux::TSperator("Damping");
                    ImGuiAux::TProperty<float>("Linear", &component.LinearDamping, 0.0f, 100.0f);
                    ImGuiAux::TProperty<float>("Angular", &component.AngularDamping, 0.0f, 100.0f);

                    ImGuiAux::TSperator("Freeze Rotation");
                    ImGuiAux::TProperty<bool>("X Axis", &component.FreezeRotationX);
                    ImGuiAux::TProperty<bool>("Y Axis", &component.FreezeRotationY);
                    ImGuiAux::TProperty<bool>("Z Axis", &component.FreezeRotationZ);
                }
            });
        }

        if(entity.HasComponent<BoxColliderComponent>())
        {
            BoxColliderComponent& component = entity.GetComponent<BoxColliderComponent>();
            DrawComponent<BoxColliderComponent>(entity, "Box Collider", [&component]() {
                ImGuiAux::TProperty<glm::vec3>("Half Extents", &component.HalfExtents, 0.0f, 10000.0f);
                ImGuiAux::TProperty<bool>("Show Collider", &component.ShowCollider);
            });
        }

        if(entity.HasComponent<SphereColliderComponent>())
        {
            SphereColliderComponent& component = entity.GetComponent<SphereColliderComponent>();
            DrawComponent<SphereColliderComponent>(entity, "Sphere Collider", [&component]() {
                ImGuiAux::TProperty<float>("Radius", &component.Radius, 0.0f, 10000.0f);
                ImGuiAux::TProperty<bool>("Show Collider", &component.ShowCollider);
            });
        }

        if(entity.HasComponent<CapsuleColliderComponent>())
        {
            CapsuleColliderComponent& component = entity.GetComponent<CapsuleColliderComponent>();
            DrawComponent<CapsuleColliderComponent>(entity, "Capsule Collider", [&component]() {
                ImGuiAux::TProperty<float>("Radius", &component.Radius, 0.0f, 10000.0f);
                ImGuiAux::TProperty<float>("Height", &component.Height, 0.0f, 10000.0f);
                ImGuiAux::TProperty<bool>("Show Collider", &component.ShowCollider);
            });
        }

        if(entity.HasComponent<CylinderColliderComponent>())
        {
            CylinderColliderComponent& component = entity.GetComponent<CylinderColliderComponent>();
            DrawComponent<CylinderColliderComponent>(entity, "Cylinder Collider", [&component]() {
                ImGuiAux::TProperty<float>("Radius", &component.Radius, 0.0f, 10000.0f);
                ImGuiAux::TProperty<float>("Height", &component.Height, 0.0f, 10000.0f);
                ImGuiAux::TProperty<bool>("Show Collider", &component.ShowCollider);
            });
        }

        if(entity.HasComponent<ConvexColliderComponent>())
        {
            ConvexColliderComponent& component = entity.GetComponent<ConvexColliderComponent>();
            DrawComponent<ConvexColliderComponent>(entity, "Convex Collider", [&component, &entity]() {
                if(entity.HasComponent<MeshComponent>())
                {
                    if(ImGuiAux::TProperty<glm::vec3>("Local Offset", &component.LocalOffset))
                        component.IsDirty = true;
                    if(ImGuiAux::TProperty<glm::vec3>("Local Rotation", &component.LocalRotation))
                        component.IsDirty = true;
                }
                else
                    HintRow("The hull is built from the Mesh component, add one to this entity", ImVec4(0.95f, 0.55f, 0.35f, 1.0f));
                ImGuiAux::TProperty<bool>("Show Collider", &component.ShowCollider);
            });
        }

        if(entity.HasComponent<MeshColliderComponent>())
        {
            MeshColliderComponent& component = entity.GetComponent<MeshColliderComponent>();
            DrawComponent<MeshColliderComponent>(entity, "Mesh Collider", [&component, &entity]() {
                if(entity.HasComponent<RigidbodyComponent>() && entity.GetComponent<RigidbodyComponent>().Type != RigidbodyType::DYNAMIC)
                {
                    if(entity.HasComponent<MeshComponent>())
                    {
                        ImGuiAux::TProperty<glm::vec3>("Local Offset", &component.LocalOffset);
                        ImGuiAux::TProperty<glm::vec3>("Local Rotation", &component.LocalRotation);
                    }
                    ImGuiAux::TProperty<bool>("Show Collider", &component.ShowCollider);
                    HintRow("Generated from the mesh triangles, expensive to visualize: turn Show Collider off when done");
                }
                else
                    HintRow("Invalid setup: needs a STATIC or KINEMATIC Rigidbody on this entity", ImVec4(0.95f, 0.40f, 0.40f, 1.0f));
            });
        }

        if(entity.HasComponent<ScriptComponent>())
        {
            ScriptComponent& component = entity.GetComponent<ScriptComponent>();
            DrawComponent<ScriptComponent>(entity, "Scripts", [&component, &entity]() {
                const bool playing = static_cast<Editor*>(Core::GetClient())->IsPlaying();
                int removeIndex = -1;
                for(size_t i = 0; i < component.Scripts.size(); i++)
                {
                    ScriptEntry& entry = component.Scripts[i];
                    ImGui::PushID(static_cast<int>(i));

                    const String title = entry.ScriptAsset ? ScriptEngine::GetScriptName(entry.ScriptAsset) : String("Empty slot");
                    ImGuiAux::TSperator(title.c_str());

                    // Enabled toggles OnEnable/OnDisable while playing; scripts are only added/removed in edit mode
                    ImGui::TableNextColumn();
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Enabled");
                    ImGui::TableNextColumn();
                    ImGui::Checkbox("##Active", &entry.Active);
                    if(!playing)
                    {
                        const float removeWidth = ImGui::CalcTextSize("Remove").x + ImGui::GetStyle().FramePadding.x * 2.0f;
                        ImGui::SameLine(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - removeWidth);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.18f, 0.18f, 1.0f));
                        if(ImGui::Button("Remove"))
                            removeIndex = static_cast<int>(i);
                        ImGui::PopStyleColor();
                    }

                    DrawAssetDropSlot<Script>("Script", entry.ScriptAsset, AssetType::SCRIPT, "None (Script)");
                    const bool live = playing && entry.IsInstantiated;
                    ScriptFields::DrawInspector(entry.ScriptAsset, entry.Fields, live ? &entry.Instance : nullptr, entity.GetScene());
                    ImGui::PopID();
                }
                if(removeIndex >= 0)
                    component.Scripts.erase(component.Scripts.begin() + removeIndex);

                if(!playing)
                {
                    ImGui::TableNextColumn();
                    ImGui::TableNextColumn();
                    if(ImGui::Button("+ Add Script", ImVec2(-FLT_MIN, 0.0f)))
                        component.Scripts.emplace_back();
                    ImGuiAux::ItemTooltip("Adds an empty slot, or drop a SCRIPT from the Content Browser here");
                    if(ImGui::BeginDragDropTarget())
                    {
                        if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(CONTENT_BROWSER_PAYLOAD))
                        {
                            AssetManager* am = Core::GetAssetManager();
                            const AssetID dropped = *static_cast<const AssetID*>(payload->Data);
                            if(am->GetMetadata(dropped).Type == AssetType::SCRIPT)
                                component.Scripts.emplace_back().ScriptAsset = am->Load<Script>(dropped);
                            else
                                EditorStatusBar::Get().Notify(Severity::Warn, "Only SCRIPT assets can be added to a Script component");
                        }
                        ImGui::EndDragDropTarget();
                    }
                }
            });
        }

        if(entity.HasComponent<TextComponent>())
        {
            TextComponent& component = entity.GetComponent<TextComponent>();
            DrawComponent<TextComponent>(entity, "Text", [&component]() {
                DrawAssetDropSlot<Font>("Font", component.FontAsset, AssetType::FONT, "None (Font)");
                if(!component.FontAsset)
                {
                    HintRow("Drop a FONT to edit the text");
                    return;
                }

                ImGui::TableNextColumn();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Text");
                ImGui::TableNextColumn();
                ImGui::InputTextMultiline("##Text", &component.Text, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 3.5f));
                ImGuiAux::TProperty<glm::vec4, ImGuiAux::CustomProprtyFlag::Color4>("Color", &component.Color);
                ImGuiAux::TProperty<float>("Max Width", &component.MaxWidth, 0.0f, 100000.0f);

                ImGuiAux::TSperator("Layout");
                ImGuiAux::TProperty<float>("Letter Spacing", &component.LetterSpacing);
                ImGuiAux::TProperty<float>("Line Spacing", &component.LineSpacing);
                constexpr auto alignTypeStrings = std::array { "LEFT", "CENTER", "RIGHT" };
                ImGuiAux::TComboBox("Horizontal Align", component.Alignment, alignTypeStrings);
                constexpr auto verticalAlignTypeStrings = std::array { "TOP", "CENTER", "BASELINE", "BOTTOM" };
                ImGuiAux::TComboBox("Vertical Align", component.VerticalAlignment, verticalAlignTypeStrings);

                ImGuiAux::TSperator("Style");
                ImGuiAux::TProperty<bool>("Underline", &component.Underline);
                ImGuiAux::TProperty<bool>("Italic", &component.Italic);
                ImGuiAux::TProperty<bool>("Billboard", &component.Billboard);

                ImGuiAux::TSperator("Shadow");
                ImGui::PushID("Shadow");
                ImGuiAux::TProperty<bool>("Enabled", &component.ShadowEnabled);
                if(component.ShadowEnabled)
                {
                    ImGuiAux::TProperty<glm::vec4, ImGuiAux::CustomProprtyFlag::Color4>("Color", &component.ShadowColor);
                    ImGuiAux::TProperty<glm::vec2>("Offset", &component.ShadowOffset);
                }
                ImGui::PopID();
            });
        }

        if(entity.HasComponent<UICanvasComponent>())
        {
            UICanvasComponent& component = entity.GetComponent<UICanvasComponent>();
            DrawComponent<UICanvasComponent>(entity, "UI Canvas", [&component, &entity]() {
                ImGuiAux::TProperty<bool>("Show Canvas", &component.ShowCanvas);
                ImGuiAux::TProperty<int>("Sort Order", &component.SortOrder, -1000.0f, 1000.0f);
                DrawAssetDropSlot<UILayout>("Layout", component.Layout, AssetType::UI_LAYOUT, "None (UI Layout)");

                ImGui::TableNextColumn();
                ImGui::TableNextColumn();
                UIEditorPanel* uiEditor = static_cast<Editor*>(Core::GetClient())->GetPanelManager().GetPanel<UIEditorPanel>();
                if(component.Layout)
                {
                    if(ImGui::Button("Edit Layout", ImVec2(-FLT_MIN, 0.0f)))
                        uiEditor->OpenLayout(component.Layout->GetID());
                }
                else if(ImGui::Button("Create New Layout", ImVec2(-FLT_MIN, 0.0f)))
                {
                    // Assets/UI/<EntityName>.sui, assigned and opened in the UI Editor right away
                    AssetManager* am = Core::GetAssetManager();
                    const String name = entity.HasComponent<NameComponent>() ? entity.GetComponent<NameComponent>().Name : String("UICanvas");
                    const AssetID layoutID = UIEditorPanel::CreateLayoutAsset(std::filesystem::path(am->GetAssetsDirectory()) / "UI", name);
                    if(layoutID)
                    {
                        component.Layout = am->Load<UILayout>(layoutID);
                        uiEditor->OpenLayout(layoutID);
                    }
                }

                ImGuiAux::TSperator("Script");
                DrawAssetDropSlot<Script>("Script", component.ScriptAsset, AssetType::SCRIPT, "None (Script)");
                const bool live = static_cast<Editor*>(Core::GetClient())->IsPlaying() && component.IsInstantiated;
                ScriptFields::DrawInspector(component.ScriptAsset, component.Fields, live ? &component.Instance : nullptr, entity.GetScene());
            });
        }

        if(entity.HasComponent<AudioSourceComponent>())
        {
            AudioSourceComponent& component = entity.GetComponent<AudioSourceComponent>();
            DrawComponent<AudioSourceComponent>(entity, "Audio Source", [&component]() {
                AudioEngine* audioEngine = Core::GetAudioEngine();

                DrawAssetDropSlot<Audio>("Audio Clip", component.AudioClip, AssetType::AUDIO, "None (Audio)");
                if(!component.AudioClip)
                    return;

                if(ImGuiAux::TSlider<float>("Volume", &component.Volume, 0.0f, 2.0f, "%.2f") && component.RuntimeID)
                    audioEngine->SetVolume(component.RuntimeID, component.Volume);
                if(ImGuiAux::TSlider<float>("Pitch", &component.Pitch, 0.1f, 3.0f, "%.2f") && component.RuntimeID)
                    audioEngine->SetPitch(component.RuntimeID, component.Pitch);
                if(ImGuiAux::TProperty<bool>("Loop", &component.Loop) && component.RuntimeID)
                    audioEngine->SetLooping(component.RuntimeID, component.Loop);
                ImGuiAux::TProperty<bool>("Play On Awake", &component.PlayOnAwake);
                ImGuiAux::TProperty<bool>("Streaming", &component.IsStreaming);

                ImGuiAux::TSperator("3D Sound");
                ImGuiAux::TProperty<bool>("Spatialized", &component.IsSpatialized);
                if(component.IsSpatialized)
                {
                    constexpr auto attenuationModelStrings = std::array { "NONE", "INVERSE_DISTANCE", "LINEAR_DISTANCE", "EXPONENTIAL_DISTANCE" };
                    ImGuiAux::TComboBox("Attenuation", component.Attenuation, attenuationModelStrings);

                    if(ImGuiAux::TProperty<float>("Min Distance", &component.MinDistance, 0.0f, 100000.0f) && component.RuntimeID)
                        audioEngine->SetMinDistance(component.RuntimeID, component.MinDistance);
                    if(ImGuiAux::TProperty<float>("Max Distance", &component.MaxDistance, 0.0f, 100000.0f) && component.RuntimeID)
                        audioEngine->SetMaxDistance(component.RuntimeID, component.MaxDistance);
                }
            });
        }

        if(entity.HasComponent<AudioListenerComponent>())
        {
            DrawComponent<AudioListenerComponent>(entity, "Audio Listener", []() {
                HintRow("Hears the scene's 3D sounds from this entity. Keep one active listener per scene");
            });
        }
    }

} // namespace Surge
