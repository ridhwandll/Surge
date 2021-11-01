// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "VulkanRenderContext.hpp"
#include "Surge/Graphics/UniformBuffer.hpp"

namespace Surge
{
    class VulkanRenderer : public Renderer
    {
    public:
        virtual ~VulkanRenderer() = default;
        virtual void Initialize() override;
        virtual void Shutdown() override;

        virtual void EndFrame() override;

        virtual void BeginRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Framebuffer>& framebuffer) override;
        virtual void EndRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer) override;

        // Vulkan Specific
        VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetAllocateInfo allocInfo, bool resetEveryFrame, int index = -1);
        void FreeDescriptorSet(VkDescriptorSet& set, bool resetEveryFrame, int index = -1);

        Vector<VkDescriptorPool> GetDescriptorPools() { return mDescriptorPools; }

    private:
        Vector<VkDescriptorPool> mDescriptorPools;
        Vector<VkDescriptorPool> mNonResetableDescriptorPools;

        Ref<UniformBuffer> mLightUniformBuffer;
        Vector<VkDescriptorSet> mLightsDescriptorSets; // TODO: Come up with a better way of managing descriptor sets
    };
} // namespace Surge