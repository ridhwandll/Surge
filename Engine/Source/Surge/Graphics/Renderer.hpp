// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include "Surge/Graphics/RendererBase.hpp"

#define FRAMES_IN_FLIGHT 3

namespace Surge
{
    class Renderer
    {
    public:
        Renderer() = default;
        ~Renderer() = default;

        void Initialize();

        void RenderRectangle(const glm::vec3& position, const glm::vec3& scale); // TODO: Remove

        void Shutdown();
    private:
        RendererBase mBase;
    };
}
