// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include "Surge/Graphics/RenderCommandBuffer.hpp"
#include "Surge/Graphics/Shader/Shader.hpp"
#include "Surge/Graphics/Framebuffer.hpp"

namespace Surge
{
    enum class PrimitiveTopology
    {
        None = 0,
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip
    };

    enum class CompareOperation
    {
        Never = 0,
        Less,
        Equal,
        LessOrEqual,
        Greater,
        NotEqual,
        GreaterOrEqual,
        Always
    };

    enum class PolygonMode
    {
        None = 0,
        Fill,
        Line,
        Point
    };

    enum class CullMode
    {
        None = 0,
        Front,
        Back,
        FrontAndBack
    };

    struct GraphicsPipelineSpecification
    {
        Ref<Shader> Shader;
        PrimitiveTopology Topology = PrimitiveTopology::TriangleList;
        PolygonMode PolygonMode = PolygonMode::Fill;
        CullMode CullingMode = CullMode::Back;
        CompareOperation DepthCompOperation = CompareOperation::Less;
        Ref<Framebuffer> TargetFramebuffer = nullptr;
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
        virtual void Bind(const Ref<RenderCommandBuffer>& cmdBuffer) const = 0;
        virtual void SetPushConstantData(const Ref<RenderCommandBuffer>& cmdBuffer, const String& bufferName, void* data) const = 0;
        virtual void DrawIndexed(const Ref<RenderCommandBuffer>& cmdBuffer, Uint indicesCount, Uint baseIndex, Uint baseVertex) const = 0;

        static Ref<GraphicsPipeline> Create(const GraphicsPipelineSpecification& pipelineSpec);
    };
} // namespace Surge