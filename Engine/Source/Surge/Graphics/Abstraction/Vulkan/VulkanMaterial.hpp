// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Material.hpp"
#include <volk/volk.h>

namespace Surge
{
    class SURGE_API VulkanMaterial : public Material
    {
    public:
        VulkanMaterial(const Ref<Shader>& shader, const String& materialName);
        ~VulkanMaterial();

        virtual void UpdateForRendering() override;
        virtual void Bind(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<GraphicsPipeline>& gfxPipeline) const override;
        virtual void Load() override;
        virtual void Release() override;

    private:
        Vector<VkDescriptorSet> mDescriptorSets;
        Vector<VkDescriptorSet> mTextureDescriptorSets;
        Uint mBinding;
    };

} // namespace Surge
