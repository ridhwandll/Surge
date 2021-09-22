// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include "Surge/Graphics/Shader.hpp"
#include "Surge/Graphics/RenderCommandBuffer.hpp"

namespace Surge
{
    enum class PrimitiveTopology
    {
        None,
        Points,
        Lines,
        LineStrip,
        Triangles,
        TriangleStrip
    };

    struct GraphicsPipelineSpecification
    {
        Ref<Shader> Shader;
        PrimitiveTopology Topology = PrimitiveTopology::Triangles;
        float LineWidth = 1.0f;
        bool UseDepth = true;
        bool UseStencil = false;
        String DebugName = "";
    };

    class GraphicsPipeline : public RefCounted
    {
    public:
        GraphicsPipeline() = default;
        virtual ~GraphicsPipeline() = default;

        virtual const GraphicsPipelineSpecification& GetPipelineSpecification() const = 0;
        virtual void Bind(const Ref<RenderCommandBuffer>& cmdBuffer) = 0;
        virtual void SetPushConstantData(const Ref<RenderCommandBuffer>& cmdBuffer, const String& bufferName, void* data) = 0;

        virtual void DrawIndexed(const Ref<RenderCommandBuffer>& cmdBuffer, Uint indicesCount) = 0;

        static Ref<GraphicsPipeline> Create(const GraphicsPipelineSpecification& pipelineSpec);
    };
}