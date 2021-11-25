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
        virtual void Update(const Ref<UniformBuffer>& dataBuffer) = 0;

        static Ref<DescriptorSet> Create(const Ref<Shader>& shader, bool resetEveryFrame, int index = -1);
    };

} // namespace Surge
