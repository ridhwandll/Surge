// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Gizmos/GizmoCanvas.hpp"

namespace Surge
{
    struct ParticleShapeModule;
    struct ParticleCollisionPlane;

    // Particle emitter wireframes, shared by the Scene views (SceneGizmos) and the Particle Editor preview
    namespace ParticleGizmos
    {
        // Shape space relative to the system: Position, then Rotation (degrees). Particles leave along its +Y
        glm::mat4 GetShapeTransform(const ParticleShapeModule& shape);

        // Wire volume/area the particles spawn in. world = system transform (entity or preview, scale included)
        void DrawEmitterShape(GizmoCanvas& canvas, const ParticleShapeModule& shape, const glm::mat4& world, ImU32 color, float thickness);

        // Arrow along the emit direction (+Y of the shape space), constant size on screen. Spheres and circles emit radially: no arrow
        void DrawEmitDirection(GizmoCanvas& canvas, const ParticleShapeModule& shape, const glm::mat4& world, ImU32 color, float thickness);

        // World space collision plane as a grid patch around 'focus' (projected onto the plane) with its normal
        void DrawCollisionPlane(GizmoCanvas& canvas, const ParticleCollisionPlane& plane, const glm::vec3& focus, float halfSize, ImU32 color, float thickness);

    } // namespace ParticleGizmos

} // namespace Surge
