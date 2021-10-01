// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Renderer/Renderer.hpp"

namespace Surge
{
    class VulkanRenderer : public Renderer
    {
    public:
        virtual void Initialize() override;
        virtual void Shutdown() override;

        virtual void BeginFrame(const EditorCamera& camera) override;
        virtual void EndFrame() override;
        virtual void SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform) override;
    };
}
