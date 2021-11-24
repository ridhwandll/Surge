// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Material.hpp"
#include <volk/volk.h>

namespace Surge
{
    class VulkanMaterial : public Material
    {
    public:
        VulkanMaterial(const Ref<Shader>& shader);
        ~VulkanMaterial();

        virtual void Bind(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<GraphicsPipeline>& gfxPipeline) const override;
        virtual void Load() override;
        virtual void Release() override;

    private:
        Vector<VkDescriptorSet> mDescriptorSets;
    };

} // namespace Surge
