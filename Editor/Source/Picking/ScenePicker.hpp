// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/ECS/Scene.hpp"
#include <glm/glm.hpp>

namespace Surge
{
    struct PickRay
    {
        glm::vec3 Origin;
        glm::vec3 Direction; // Normalized
    };

    // Editor-only viewport selection: CPU ray casts against the geometry the renderer draws (mesh triangles, sprite quads, text blocks).
    // Runs once per click, never per frame
    class ScenePicker
    {
    public:
        // screenPos and the viewport rect are in the same (ImGui screen) space, view/projection are the camera's (Vulkan clip space)
        static PickRay ScreenToRay(const glm::vec2& screenPos, const glm::vec2& viewportMin, const glm::vec2& viewportSize, const glm::mat4& view, const glm::mat4& projection);

        // Closest entity under the ray, invalid when nothing is hit. Clicking the same spot again walks through
        // everything under the cursor (nearest first, wrapping around) starting after currentSelection
        Entity Pick(Scene& scene, const PickRay& ray, const glm::mat4& view, const glm::vec2& clickPos, Entity currentSelection);

    private:
        struct Hit
        {
            entt::entity Handle;
            float Distance;
        };

        void CollectHits(Scene& scene, const PickRay& ray, const glm::mat4& view);

    private:
        Vector<Hit> mHits; // Reused between clicks
        glm::vec2 mLastClickPos = glm::vec2(0.0f);
        bool mHasLastClick = false;
    };
} // namespace Surge
