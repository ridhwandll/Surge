// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include "Surge/Graphics/Interface/UniformBuffer.hpp"

namespace Surge
{
    class DescriptorSet : public RefCounted
    {
    public:
        DescriptorSet() = default;
        virtual ~DescriptorSet() = default;

        virtual void Bind(const Ref<RenderCommandBuffer>& commandBuffer, const Ref<GraphicsPipeline>& pipeline) = 0;
        virtual void UpdateForRendering() = 0;
        virtual void SetBuffer(const Ref<UniformBuffer>& dataBuffer, Uint binding) = 0;
        virtual void SetImage2D(const Ref<Image2D>& image, Uint binding) = 0;

        static Ref<DescriptorSet> Create(const Ref<Shader>& shader, Uint setNumber, bool resetEveryFrame, int index = -1);
    };

} // namespace Surge
