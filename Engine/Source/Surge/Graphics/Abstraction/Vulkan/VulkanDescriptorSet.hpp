// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Interface/DescriptorSet.hpp"
#include <volk/volk.h>

namespace Surge
{
    class VulkanDescriptorSet : public DescriptorSet
    {
    public:
        VulkanDescriptorSet(const Ref<Shader>& shader, bool resetEveryFrame, int index = -1);
        ~VulkanDescriptorSet();

        virtual void Bind(const Ref<RenderCommandBuffer>& commandBuffer, const Ref<GraphicsPipeline>& pipeline) override;
        virtual void Update(const Ref<UniformBuffer>& dataBuffer) override;

        Vector<VkDescriptorSet> GetVulkanDescriptorSets() { return mDescriptorSets; }

    private:
        Vector<VkDescriptorSet> mDescriptorSets;
    };

} // namespace Surge
