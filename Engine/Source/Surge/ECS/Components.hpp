// Copyright (c) - SurgeTechnologies - All rights reserved
#include "SurgeReflect/SurgeReflect.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Surge
{
    struct TransformComponent
    {
        TransformComponent() {}
        TransformComponent(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
            : Position(position), Rotation(rotation), Scale(scale) {}

        glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 Scale = glm::vec3(1.0f, 1.0f, 1.0f);

        glm::mat4 GetTransform()
        {
            const glm::mat4 rot = glm::toMat4(glm::quat(Rotation));
            glm::mat4 result = glm::translate(glm::mat4(1.0f), Position) * rot * glm::scale(glm::mat4(1.0f), Scale);
            return result;
        }

        SURGE_REFLECTION_ENABLE;
    };

    struct MeshComponent
    {
        MeshComponent() = default;
        MeshComponent(const Ref<Mesh>& mesh)
            : Mesh(mesh) {}

        Ref<Surge::Mesh> Mesh;

        SURGE_REFLECTION_ENABLE;
    };

} // namespace Surge
