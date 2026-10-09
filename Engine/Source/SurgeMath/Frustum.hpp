// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "SurgeMath/AABB.hpp"
#include <glm/glm.hpp>

namespace Surge
{
    // View frustum as 6 planes (xyz = normal pointing inside, w = distance), extracted from a view-projection matrix
    // Works for perspective and orthographic projections with a [0, 1] depth range (GLM_FORCE_DEPTH_ZERO_TO_ONE)
    struct Frustum
    {
        glm::vec4 Planes[6];

        Frustum() = default;
        explicit Frustum(const glm::mat4& viewProjection) { Extract(viewProjection); }

        void Extract(const glm::mat4& m)
        {
            const glm::vec4 row0 = { m[0][0], m[1][0], m[2][0], m[3][0] };
            const glm::vec4 row1 = { m[0][1], m[1][1], m[2][1], m[3][1] };
            const glm::vec4 row2 = { m[0][2], m[1][2], m[2][2], m[3][2] };
            const glm::vec4 row3 = { m[0][3], m[1][3], m[2][3], m[3][3] };

            Planes[0] = row3 + row0; // Left
            Planes[1] = row3 - row0; // Right
            Planes[2] = row3 + row1; // Bottom
            Planes[3] = row3 - row1; // Top
            Planes[4] = row2;        // Near (0..1 depth)
            Planes[5] = row3 - row2; // Far

            for(glm::vec4& plane : Planes)
                plane /= glm::length(glm::vec3(plane));
        }

        // Conservative test of a local space AABB transformed by 'transform'
        bool IntersectsAABB(const AABB& localBounds, const glm::mat4& transform) const
        {
            const glm::vec3 localCenter = (localBounds.Min + localBounds.Max) * 0.5f;
            const glm::vec3 localExtents = (localBounds.Max - localBounds.Min) * 0.5f;

            // World space AABB enclosing the transformed box (Arvo's method)
            const glm::vec3 center = glm::vec3(transform * glm::vec4(localCenter, 1.0f));
            const glm::mat3 basis = glm::mat3(transform);
            const glm::vec3 extents = glm::abs(basis[0]) * localExtents.x + glm::abs(basis[1]) * localExtents.y + glm::abs(basis[2]) * localExtents.z;

            for(const glm::vec4& plane : Planes)
            {
                const glm::vec3 n = glm::vec3(plane);
                const float radius = glm::dot(extents, glm::abs(n));
                if(glm::dot(n, center) + plane.w < -radius)
                    return false;
            }
            return true;
        }

        bool IntersectsSphere(const glm::vec3& center, float radius) const
        {
            for(const glm::vec4& plane : Planes)
            {
                if(glm::dot(glm::vec3(plane), center) + plane.w < -radius)
                    return false;
            }
            return true;
        }
    };
} // namespace Surge
