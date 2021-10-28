// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "Surge/Core/UUID.hpp"
#include "Surge/Graphics/Material.hpp"
#include "Surge/Graphics/Mesh.hpp"
#include "Surge/Graphics/Camera/RuntimeCamera.hpp"
#include "SurgeReflect/SurgeReflect.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Surge
{
    struct IDComponent
    {
        IDComponent() = default;
        IDComponent(const UUID& id)
            : ID(id) {}

        UUID ID;

        SURGE_REFLECTION_ENABLE;
    };

    struct NameComponent
    {
        NameComponent() = default;
        NameComponent(const String& name)
            : Name(name) {}

        String Name;

        SURGE_REFLECTION_ENABLE;
    };

    struct TransformComponent
    {
        TransformComponent() = default;
        TransformComponent(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
            : Position(position), Rotation(rotation), Scale(scale) {}

        glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 Rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 Scale = glm::vec3(1.0f, 1.0f, 1.0f);

        glm::mat4 GetTransform()
        {
            const glm::mat4 rot = glm::toMat4(glm::quat(glm::radians(Rotation)));
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

        // TODO: Make sure that material works for all types of shader
        Ref<Surge::Material> Material = Surge::Material::Create("Simple");

        SURGE_REFLECTION_ENABLE;
    };

    struct CameraComponent
    {
        CameraComponent() = default;
        CameraComponent(const RuntimeCamera& cam, bool primary, bool fixedAspectRatio)
            : Camera(cam), Primary(primary), FixedAspectRatio(fixedAspectRatio) {}

        RuntimeCamera Camera;
        bool Primary = true;
        bool FixedAspectRatio = false;

        SURGE_REFLECTION_ENABLE;
    };

    struct PointLightComponent
    {
        PointLightComponent() = default;
        PointLightComponent(glm::vec3 color, float intensity, float radius)
            : Color(color), Intensity(intensity), Radius(radius) {}

        glm::vec3 Color = {1.0f, 1.0f, 1.0f};
        float Intensity = 1.0f;
        float Radius = 3.0f;

        SURGE_REFLECTION_ENABLE;
    };
} // namespace Surge