// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Gizmos/SceneGizmos.hpp"
#include "Surge/Core/Core.hpp"
#include "Surge/ECS/Components/Components.hpp"
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Graphics/HighLevel/Mesh.hpp"
#include "Surge/Physics/Physics.hpp"

#include <Jolt/Renderer/DebugRenderer.h>
#include <glm/gtc/constants.hpp>
#include <algorithm>

namespace Surge
{
    static constexpr int CIRCLE_SEGMENTS = 64;
    static constexpr ImU32 LINE_RIM_COLOR = IM_COL32(0, 0, 0, 70); // Dark rim under every wire, keeps them readable on bright backgrounds
    static constexpr float LINE_RIM_EXTRA = 2.0f;
    static constexpr float SELECTED_THICKNESS = 1.75f;
    static constexpr float UNSELECTED_THICKNESS = 1.25f;
    static constexpr ImU32 SELECTION_COLOR = IM_COL32(255, 168, 0, 255); // Matches the ImGuizmo selection color (see SceneViewport::SetupManipulatorStyle)
    static constexpr ImU32 AUDIO_COLOR = IM_COL32(120, 205, 255, 255);

    // ConvexColliderComponent hulls are expensive to build, rebuilt only when the component is dirty or the selection changed
    static JPH::ShapeRefC sConvexShapeCache = nullptr;
    static entt::entity sConvexShapeOwner = entt::null;

    static ImU32 ScaleAlpha(ImU32 color, float factor)
    {
        const ImU32 alpha = (ImU32)((float)((color >> IM_COL32_A_SHIFT) & 0xFF) * glm::clamp(factor, 0.0f, 1.0f));
        return (color & ~IM_COL32_A_MASK) | (alpha << IM_COL32_A_SHIFT);
    }

    static ImU32 ToU32(const glm::vec3& color, float alpha)
    {
        return ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, alpha));
    }

    // Keeps the hue of a light color but lifts dark/low intensity ones so the gizmo stays visible
    static glm::vec3 GetDisplayColor(const glm::vec3& color)
    {
        const float maxComponent = glm::max(glm::max(color.r, color.g), glm::max(color.b, 1e-4f));
        return glm::mix(glm::clamp(color / maxComponent, 0.0f, 1.0f), glm::vec3(1.0f), 0.3f);
    }

    static ImVec2 ClipToScreen(const glm::vec4& clip, const ImVec2& viewportMin, const ImVec2& viewportSize)
    {
        // Vulkan clip space: y points down, same as ImGui
        const float invW = 1.0f / clip.w;
        return ImVec2(viewportMin.x + (clip.x * invW * 0.5f + 0.5f) * viewportSize.x, viewportMin.y + (clip.y * invW * 0.5f + 0.5f) * viewportSize.y);
    }

    static bool ClipSegmentToNear(glm::vec4& a, glm::vec4& b, float nearW)
    {
        if(a.w < nearW && b.w < nearW)
            return false;

        if(a.w < nearW)
            a = glm::mix(a, b, (nearW - a.w) / (b.w - a.w));
        else if(b.w < nearW)
            b = glm::mix(b, a, (nearW - b.w) / (a.w - b.w));
        return true;
    }

    static void GetPerpendicularBasis(const glm::vec3& n, glm::vec3& outU, glm::vec3& outV)
    {
        const glm::vec3 reference = glm::abs(n.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
        outU = glm::normalize(glm::cross(n, reference));
        outV = glm::cross(n, outU);
    }

    static bool IsColliderShown(Entity entity)
    {
        if(const BoxColliderComponent* c = entity.TryGetComponent<BoxColliderComponent>()) return c->ShowCollider;
        if(const SphereColliderComponent* c = entity.TryGetComponent<SphereColliderComponent>()) return c->ShowCollider;
        if(const CapsuleColliderComponent* c = entity.TryGetComponent<CapsuleColliderComponent>()) return c->ShowCollider;
        if(const CylinderColliderComponent* c = entity.TryGetComponent<CylinderColliderComponent>()) return c->ShowCollider;
        if(const ConvexColliderComponent* c = entity.TryGetComponent<ConvexColliderComponent>()) return c->ShowCollider;
        if(const MeshColliderComponent* c = entity.TryGetComponent<MeshColliderComponent>()) return c->ShowCollider;
        return false;
    }

    void SceneGizmos::Shutdown()
    {
        sConvexShapeCache = nullptr; // Before Physics shuts Jolt down
        sConvexShapeOwner = entt::null;
        mIcons.clear();
        mHoveredEntity = {};
    }

    //////////////////////////////////////////////////////////////////////////
    // Renderer side (inside the edit scene's frame)
    //////////////////////////////////////////////////////////////////////////

    void SceneGizmos::SubmitSceneDebug(Scene& scene, Entity selected)
    {
        if(!selected || selected.GetScene() != &scene)
            return;

        Renderer* renderer = Core::GetRenderer();
        const TransformComponent& transformComp = selected.GetComponent<TransformComponent>();

        if(mSettings.ShowSelectionOutline)
        {
            const MeshComponent* meshComp = selected.TryGetComponent<MeshComponent>();
            if(meshComp && meshComp->MeshAsset)
            {
                Ref<Mesh> mesh = meshComp->MeshAsset.As<Mesh>();
                if(mesh) // Asset might be missing/corrupted
                    renderer->SubmitMeshOutline(transformComp.GetTransform(), mesh.Raw());
            }
        }

        if(mSettings.ShowColliders && IsColliderShown(selected))
        {
            JPH::ShapeRefC shape;
            if(ConvexColliderComponent* convexComp = selected.HasComponent<ConvexColliderComponent>() ? &selected.GetComponent<ConvexColliderComponent>() : nullptr)
            {
                if(!sConvexShapeCache || convexComp->IsDirty || sConvexShapeOwner != (entt::entity)selected)
                {
                    sConvexShapeCache = Core::GetPhysics()->CreateShape(selected);
                    sConvexShapeOwner = (entt::entity)selected;
                    convexComp->IsDirty = false;
                }
                shape = sConvexShapeCache;
            }
            else
                shape = Core::GetPhysics()->CreateShape(selected);

            if(shape)
            {
                // Same placement as the runtime body: local transform, center of mass offset rotated into place
                const glm::quat q = glm::quat(glm::radians(transformComp.Rotation));
                const JPH::Quat joltRotation = JPH::Quat(q.x, q.y, q.z, q.w);
                JPH::Vec3 joltPosition = JPH::Vec3(transformComp.Position.x, transformComp.Position.y, transformComp.Position.z);
                joltPosition = joltPosition + joltRotation * shape->GetCenterOfMass();

                const JPH::RMat44 joltTransform = JPH::RMat44::sRotationTranslation(joltRotation, joltPosition);
                shape->Draw(Core::GetPhysics()->GetDebugRenderer(), joltTransform, JPH::Vec3::sReplicate(1.0f), JPH::Color::sGreen, false, true);
            }
        }
    }

    ///////////////////
    // ImGui overlay //
    ///////////////////

    void SceneGizmos::DrawOverlay(Scene& scene, Entity selected, const glm::mat4& view, const glm::mat4& projection, float nearClip, const ImVec2& viewportMin, const ImVec2& viewportSize, bool allowHover)
    {
        mIcons.clear();
        mHoveredEntity = {};
        if(viewportSize.x <= 0.0f || viewportSize.y <= 0.0f)
            return;

        mDrawList = ImGui::GetWindowDrawList();
        mViewProjection = projection * view;
        mCameraPosition = glm::vec3(glm::inverse(view)[3]);
        mNearClip = glm::max(nearClip, 1e-3f);
        mProjectionScaleY = glm::max(glm::abs(projection[1][1]), 1e-4f);
        mViewportMin = viewportMin;
        mViewportSize = viewportSize;

        mDrawList->PushClipRect(viewportMin, ImVec2(viewportMin.x + viewportSize.x, viewportMin.y + viewportSize.y), true);

        entt::registry& registry = scene.GetRegistry();
        const entt::entity selectedHandle = selected ? (entt::entity)selected : entt::null;
        const bool wireForAll = mSettings.ShowWireForAll;

        if(mSettings.ShowCameraFrustums)
        {
            for(const auto& [entity, transform, cameraComp] : registry.view<TransformComponent, CameraComponent>().each())
            {
                const bool isSelected = entity == selectedHandle;
                if(!isSelected && !wireForAll)
                    continue;

                const ImU32 color = isSelected ? IM_COL32(235, 235, 235, 255) : IM_COL32(200, 200, 200, 110);
                DrawCameraFrustum(transform.GetTransform(), cameraComp.Camera, color, isSelected ? SELECTED_THICKNESS : UNSELECTED_THICKNESS);
            }
        }

        if(mSettings.ShowLightGizmos)
        {
            for(const auto& [entity, transform, light] : registry.view<TransformComponent, LightComponent>().each())
            {
                const bool isSelected = entity == selectedHandle;
                if(!isSelected && !wireForAll)
                    continue;

                const float alpha = (isSelected ? 0.9f : 0.45f) * (light.Active ? 1.0f : 0.5f);
                const ImU32 color = ToU32(GetDisplayColor(light.Color), alpha);
                const float thickness = isSelected ? SELECTED_THICKNESS : UNSELECTED_THICKNESS;
                const glm::mat4& world = transform.GetTransform();

                if(light.Type == LightType::POINT)
                    DrawWireSphere(glm::vec3(world[3]), light.Radius, color, thickness);
                else
                    DrawDirectionalLight(world, color, thickness);
            }
        }

        if(mSettings.ShowAudioGizmos)
        {
            for(const auto& [entity, transform, audioSrc] : registry.view<TransformComponent, AudioSourceComponent>().each())
            {
                const bool isSelected = entity == selectedHandle;
                if(!audioSrc.IsSpatialized || (!isSelected && !wireForAll))
                    continue;

                const float alpha = (isSelected ? 0.85f : 0.4f) * (audioSrc.Active ? 1.0f : 0.5f);
                const float thickness = isSelected ? SELECTED_THICKNESS : UNSELECTED_THICKNESS;
                const glm::vec3 position = glm::vec3(transform.GetTransform()[3]);
                DrawWireSphere(position, audioSrc.MinDistance, ScaleAlpha(AUDIO_COLOR, alpha), thickness);
                DrawWireSphere(position, audioSrc.MaxDistance, ScaleAlpha(IM_COL32(70, 140, 235, 255), alpha * 0.8f), thickness);
            }
        }

        if(mSettings.ShowIcons)
            DrawIcons(scene, selectedHandle, allowHover);

        mDrawList->PopClipRect();
        mDrawList = nullptr;
    }

    void SceneGizmos::DrawIcons(Scene& scene, entt::entity selectedHandle, bool allowHover)
    {
        entt::registry& registry = scene.GetRegistry();

        // One icon per entity, the first matching component in this order wins
        auto addIcon = [&](entt::entity entity, const TransformComponent& transform, GizmoIcon icon, ImU32 tint, bool inactive) {
            ImVec2 screenPos;
            float depth;
            if(ProjectPoint(glm::vec3(transform.GetTransform()[3]), screenPos, &depth))
                mIcons.push_back(IconCmd { entity, screenPos, depth, icon, tint, inactive });
        };

        for(const auto& [entity, transform, cameraComp] : registry.view<TransformComponent, CameraComponent>().each())
            addIcon(entity, transform, GizmoIcon::CAMERA, cameraComp.Primary ? IM_COL32(245, 245, 245, 255) : IM_COL32(185, 185, 185, 255), !cameraComp.Active);

        for(const auto& [entity, transform, light] : registry.view<TransformComponent, LightComponent>().each())
        {
            if(!registry.any_of<CameraComponent>(entity))
                addIcon(entity, transform, light.Type == LightType::DIRECTIONAL ? GizmoIcon::DIRECTIONAL_LIGHT : GizmoIcon::POINT_LIGHT, ToU32(GetDisplayColor(light.Color), 1.0f), !light.Active);
        }

        for(const auto& [entity, transform, audioSrc] : registry.view<TransformComponent, AudioSourceComponent>().each())
        {
            if(!registry.any_of<CameraComponent, LightComponent>(entity))
                addIcon(entity, transform, GizmoIcon::AUDIO_SOURCE, AUDIO_COLOR, !audioSrc.Active);
        }

        for(const auto& [entity, transform, listener] : registry.view<TransformComponent, AudioListenerComponent>().each())
        {
            if(!registry.any_of<CameraComponent, LightComponent, AudioSourceComponent>(entity))
                addIcon(entity, transform, GizmoIcon::AUDIO_LISTENER, AUDIO_COLOR, !listener.Active);
        }

        for(const auto& [entity, transform, env] : registry.view<TransformComponent, EnvironmentComponent>().each())
        {
            if(!registry.any_of<CameraComponent, LightComponent, AudioSourceComponent, AudioListenerComponent>(entity))
                addIcon(entity, transform, GizmoIcon::ENVIRONMENT, IM_COL32(255, 200, 110, 255), !env.Active);
        }

        if(mIcons.empty())
            return;

        // Far to near so the closest icon ends up on top
        std::sort(mIcons.begin(), mIcons.end(), [](const IconCmd& a, const IconCmd& b) { return a.Depth > b.Depth; });

        const float radius = mSettings.IconSize * 0.5f;
        int hoveredIndex = -1;
        if(allowHover)
        {
            const ImVec2 mouse = ImGui::GetMousePos();
            for(int i = static_cast<int>(mIcons.size()) - 1; i >= 0; i--)
            {
                const float dx = mouse.x - mIcons[i].ScreenPos.x;
                const float dy = mouse.y - mIcons[i].ScreenPos.y;
                if(dx * dx + dy * dy <= radius * radius)
                {
                    hoveredIndex = i;
                    break;
                }
            }
        }

        for(int i = 0; i < static_cast<int>(mIcons.size()); i++)
        {
            const IconCmd& icon = mIcons[i];
            const bool isSelected = icon.Handle == selectedHandle;
            const bool isHovered = i == hoveredIndex;

            // Fades out right in front of the camera instead of covering the view
            float alpha = glm::clamp((icon.Depth - mNearClip) / 1.5f, 0.0f, 1.0f);
            if(icon.Inactive)
                alpha *= 0.45f;
            if(alpha <= 0.01f)
                continue;

            GizmoIcons::BadgeStyle style;
            style.Background = ScaleAlpha(isHovered ? IM_COL32(58, 58, 64, 240) : IM_COL32(26, 26, 30, 220), alpha);
            style.Rim = ScaleAlpha(isSelected ? SELECTION_COLOR : (isHovered ? IM_COL32(255, 255, 255, 160) : IM_COL32(255, 255, 255, 45)), alpha);
            style.RimThickness = isSelected ? 2.0f : 1.0f;
            style.Shadow = ScaleAlpha(IM_COL32(0, 0, 0, 90), alpha);
            GizmoIcons::DrawBadge(mDrawList, icon.ScreenPos, radius, icon.Icon, ScaleAlpha(icon.Tint, alpha), style);
        }

        if(hoveredIndex >= 0)
        {
            const IconCmd& icon = mIcons[hoveredIndex];
            mHoveredEntity = Entity(icon.Handle, &scene);

            if(const NameComponent* nameComp = registry.try_get<NameComponent>(icon.Handle))
            {
                const char* name = nameComp->Name.c_str();
                const ImVec2 textSize = ImGui::CalcTextSize(name);
                const ImVec2 padding(6.0f, 3.0f);
                const ImVec2 labelMin(icon.ScreenPos.x - textSize.x * 0.5f - padding.x, icon.ScreenPos.y + radius + 5.0f);
                const ImVec2 labelMax(labelMin.x + textSize.x + padding.x * 2.0f, labelMin.y + textSize.y + padding.y * 2.0f);
                mDrawList->AddRectFilled(labelMin, labelMax, IM_COL32(15, 15, 15, 225), 2.0f);
                mDrawList->AddText(ImVec2(labelMin.x + padding.x, labelMin.y + padding.y), IM_COL32(235, 235, 235, 255), name);
            }
        }
    }

    void SceneGizmos::DrawCameraFrustum(const glm::mat4& cameraWorld, const RuntimeCamera& camera, ImU32 color, float thickness)
    {
        // Frustum corners from the camera's own projection: Vulkan clip space (y down, depth 0..1), so index 0/1 is the top edge
        static const glm::vec2 ndcCorners[4] = { { -1.0f, -1.0f }, { 1.0f, -1.0f }, { 1.0f, 1.0f }, { -1.0f, 1.0f } };
        const glm::mat4 inverseProjection = glm::inverse(camera.GetProjectionMatrix());

        glm::vec3 localNear[4], nearCorners[4], farCorners[4];
        for(int i = 0; i < 4; i++)
        {
            glm::vec4 n = inverseProjection * glm::vec4(ndcCorners[i], 0.0f, 1.0f);
            glm::vec4 f = inverseProjection * glm::vec4(ndcCorners[i], 1.0f, 1.0f);
            n /= n.w;
            f /= f.w;
            localNear[i] = glm::vec3(n);
            nearCorners[i] = glm::vec3(cameraWorld * glm::vec4(glm::vec3(n), 1.0f));
            farCorners[i] = glm::vec3(cameraWorld * glm::vec4(glm::vec3(f), 1.0f));
        }

        const ImU32 faintColor = ScaleAlpha(color, 0.6f);
        DrawPolyline3D(nearCorners, 4, true, color, thickness);
        DrawPolyline3D(farCorners, 4, true, faintColor, thickness);
        for(int i = 0; i < 4; i++)
            DrawLine3D(nearCorners[i], farCorners[i], faintColor, thickness);

        // Viewfinder: a one unit deep pyramid (perspective) or the near plane (orthographic), with an "up" marker on top
        const bool isPerspective = camera.GetProjectionType() == RuntimeCamera::ProjectionType::Perspective;
        glm::vec3 body[4];
        if(isPerspective)
        {
            const float bodyDepth = glm::min(1.0f, camera.GetPerspectiveFarClip());
            const glm::vec3 origin = glm::vec3(cameraWorld[3]);
            for(int i = 0; i < 4; i++)
            {
                const glm::vec3 local = localNear[i] * (bodyDepth / glm::max(-localNear[i].z, 1e-4f));
                body[i] = glm::vec3(cameraWorld * glm::vec4(local, 1.0f));
                DrawLine3D(origin, body[i], color, thickness);
            }
            DrawPolyline3D(body, 4, true, color, thickness);
        }
        else
        {
            for(int i = 0; i < 4; i++)
                body[i] = nearCorners[i];
        }

        const glm::vec3 up = glm::normalize(glm::vec3(cameraWorld[1]));
        const glm::vec3 topEdge = body[1] - body[0];
        const glm::vec3 triangle[3] = { body[0] + topEdge * 0.3f, body[0] + topEdge * 0.7f, body[0] + topEdge * 0.5f + up * glm::length(topEdge) * 0.25f };

        ImVec2 screen[3];
        if(ProjectPoint(triangle[0], screen[0]) && ProjectPoint(triangle[1], screen[1]) && ProjectPoint(triangle[2], screen[2]))
        {
            mDrawList->AddTriangleFilled(screen[0], screen[1], screen[2], ScaleAlpha(color, 0.45f));
            mDrawList->AddTriangle(screen[0], screen[1], screen[2], color, thickness);
        }
    }

    void SceneGizmos::DrawDirectionalLight(const glm::mat4& lightWorld, ImU32 color, float thickness)
    {
        // Constant size on screen: a disc facing the light direction with parallel rays and a center arrow
        const glm::vec3 origin = glm::vec3(lightWorld[3]);
        const glm::vec3 forward = glm::normalize(glm::vec3(lightWorld[2]));
        glm::vec3 u, v;
        GetPerpendicularBasis(forward, u, v);

        const float unit = GetWorldPerPixel(origin) * mSettings.IconSize;
        const float discRadius = unit * 0.9f;
        const float rayLength = unit * 2.2f;

        DrawCircle3D(origin, u, v, discRadius, color, thickness);
        for(int i = 0; i < 8; i++)
        {
            const float angle = static_cast<float>(i) * glm::pi<float>() * 0.25f;
            const glm::vec3 start = origin + (u * glm::cos(angle) + v * glm::sin(angle)) * discRadius;
            DrawLine3D(start, start + forward * rayLength, ScaleAlpha(color, 0.8f), thickness);
        }

        const glm::vec3 tip = origin + forward * rayLength * 1.35f;
        const glm::vec3 headBase = tip - forward * unit * 0.4f;
        DrawLine3D(origin, tip, color, thickness);
        DrawLine3D(tip, headBase + u * unit * 0.2f, color, thickness);
        DrawLine3D(tip, headBase - u * unit * 0.2f, color, thickness);
        DrawLine3D(tip, headBase + v * unit * 0.2f, color, thickness);
        DrawLine3D(tip, headBase - v * unit * 0.2f, color, thickness);
    }

    void SceneGizmos::DrawWireSphere(const glm::vec3& center, float radius, ImU32 color, float thickness)
    {
        if(radius <= 0.0f)
            return;

        // Faint great circles for the volume, the silhouette (what the eye reads as the sphere outline) at full strength
        const ImU32 axisColor = ScaleAlpha(color, 0.5f);
        DrawCircle3D(center, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), radius, axisColor, thickness);
        DrawCircle3D(center, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), radius, axisColor, thickness);
        DrawCircle3D(center, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), radius, axisColor, thickness);

        const glm::vec3 toCamera = mCameraPosition - center;
        const float distance = glm::length(toCamera);
        if(distance <= radius)
            return; // Inside the sphere, there is no silhouette

        const glm::vec3 n = toCamera / distance;
        glm::vec3 u, v;
        GetPerpendicularBasis(n, u, v);
        const float silhouetteRadius = radius * glm::sqrt(distance * distance - radius * radius) / distance;
        DrawCircle3D(center + n * (radius * radius / distance), u, v, silhouetteRadius, color, thickness);
    }

    void SceneGizmos::DrawCircle3D(const glm::vec3& center, const glm::vec3& axisU, const glm::vec3& axisV, float radius, ImU32 color, float thickness)
    {
        glm::vec3 points[CIRCLE_SEGMENTS];
        for(int i = 0; i < CIRCLE_SEGMENTS; i++)
        {
            const float angle = static_cast<float>(i) / static_cast<float>(CIRCLE_SEGMENTS) * glm::two_pi<float>();
            points[i] = center + (axisU * glm::cos(angle) + axisV * glm::sin(angle)) * radius;
        }
        DrawPolyline3D(points, CIRCLE_SEGMENTS, true, color, thickness);
    }

    void SceneGizmos::DrawPolyline3D(const glm::vec3* points, int count, bool closed, ImU32 color, float thickness)
    {
        SG_ASSERT(count >= 2 && count <= CIRCLE_SEGMENTS, "[SceneGizmos] Polyline point count out of range");

        glm::vec4 clip[CIRCLE_SEGMENTS];
        bool allVisible = true;
        for(int i = 0; i < count; i++)
        {
            clip[i] = mViewProjection * glm::vec4(points[i], 1.0f);
            allVisible &= clip[i].w >= mNearClip;
        }

        ImVec2 strip[CIRCLE_SEGMENTS + 1];
        auto flush = [&](int pointCount, ImDrawFlags flags) {
            if(pointCount < 2)
                return;
            mDrawList->AddPolyline(strip, pointCount, LINE_RIM_COLOR, flags, thickness + LINE_RIM_EXTRA);
            mDrawList->AddPolyline(strip, pointCount, color, flags, thickness);
        };

        if(allVisible) // Common case: one polyline with proper joints
        {
            for(int i = 0; i < count; i++)
                strip[i] = ClipToScreen(clip[i], mViewportMin, mViewportSize);
            flush(count, closed ? ImDrawFlags_Closed : ImDrawFlags_None);
            return;
        }

        // Crosses the near plane: split into visible runs
        int stripCount = 0;
        const int segmentCount = closed ? count : count - 1;
        for(int i = 0; i < segmentCount; i++)
        {
            glm::vec4 a = clip[i];
            glm::vec4 b = clip[(i + 1) % count];
            const bool aVisible = a.w >= mNearClip;
            const bool bVisible = b.w >= mNearClip;
            if(!ClipSegmentToNear(a, b, mNearClip))
            {
                flush(stripCount, ImDrawFlags_None);
                stripCount = 0;
                continue;
            }

            if(stripCount == 0 || !aVisible)
            {
                flush(stripCount, ImDrawFlags_None);
                stripCount = 0;
                strip[stripCount++] = ClipToScreen(a, mViewportMin, mViewportSize);
            }
            strip[stripCount++] = ClipToScreen(b, mViewportMin, mViewportSize);

            if(!bVisible)
            {
                flush(stripCount, ImDrawFlags_None);
                stripCount = 0;
            }
        }
        flush(stripCount, ImDrawFlags_None);
    }

    void SceneGizmos::DrawLine3D(const glm::vec3& a, const glm::vec3& b, ImU32 color, float thickness)
    {
        glm::vec4 clipA = mViewProjection * glm::vec4(a, 1.0f);
        glm::vec4 clipB = mViewProjection * glm::vec4(b, 1.0f);
        if(!ClipSegmentToNear(clipA, clipB, mNearClip))
            return;

        const ImVec2 screenA = ClipToScreen(clipA, mViewportMin, mViewportSize);
        const ImVec2 screenB = ClipToScreen(clipB, mViewportMin, mViewportSize);
        mDrawList->AddLine(screenA, screenB, LINE_RIM_COLOR, thickness + LINE_RIM_EXTRA);
        mDrawList->AddLine(screenA, screenB, color, thickness);
    }

    bool SceneGizmos::ProjectPoint(const glm::vec3& worldPos, ImVec2& outScreen, float* outDepth) const
    {
        const glm::vec4 clip = mViewProjection * glm::vec4(worldPos, 1.0f);
        if(clip.w < mNearClip)
            return false;

        outScreen = ClipToScreen(clip, mViewportMin, mViewportSize);
        if(outDepth)
            *outDepth = clip.w;
        return true;
    }

    float SceneGizmos::GetWorldPerPixel(const glm::vec3& worldPos) const
    {
        // Perspective: clip.w is the view depth, the visible height at that depth is 2 * w / P[1][1]
        const float depth = glm::max((mViewProjection * glm::vec4(worldPos, 1.0f)).w, mNearClip);
        return (2.0f * depth) / (mProjectionScaleY * mViewportSize.y);
    }

    void SceneGizmos::OnSettingsGUI()
    {
        ImGui::SeparatorText("Gizmos");
        ImGui::Checkbox("Icons", &mSettings.ShowIcons);
        ImGui::BeginDisabled(!mSettings.ShowIcons);
        ImGui::SliderFloat("Icon Size", &mSettings.IconSize, 16.0f, 64.0f, "%.0f px");
        ImGui::EndDisabled();
        ImGui::Checkbox("Camera Frustums", &mSettings.ShowCameraFrustums);
        ImGui::Checkbox("Light Gizmos", &mSettings.ShowLightGizmos);
        ImGui::Checkbox("Audio Gizmos", &mSettings.ShowAudioGizmos);
        ImGui::Checkbox("Wire Gizmos For All", &mSettings.ShowWireForAll);
        ImGui::SetItemTooltip("Frustums and light/audio ranges for every entity instead of only the selected one");
        ImGui::Checkbox("Selection Outline", &mSettings.ShowSelectionOutline);
        ImGui::Checkbox("Colliders", &mSettings.ShowColliders);
    }

} // namespace Surge
