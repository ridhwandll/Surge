// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include <glm/glm.hpp>

namespace Surge
{
    struct PointLight
    {
        PointLight() = default;

        glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f);
        float Intensity = 1.0f;
        glm::vec3 Color = glm::vec3(1.0f, 1.0f, 1.0f);
        float Radius = 3.0f;
    };
    static_assert(sizeof(PointLight) % 16 == 0, "Size of 'PointLight' struct must be 16 bytes aligned!");

    struct LightUniformBufferData
    {
        glm::vec3 CameraPosition;
        Uint PointLightCount = 0;

        PointLight PointLights[100] = {};
    };
    static_assert(sizeof(LightUniformBufferData) % 16 == 0, "Size of 'Lights' struct must be 16 bytes aligned!");
} // namespace Surge