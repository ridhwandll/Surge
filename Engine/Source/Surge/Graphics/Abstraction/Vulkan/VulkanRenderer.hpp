// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "VulkanRenderContext.hpp"

namespace Surge
{
    class VulkanRenderer : public Renderer
    {
    public:
        virtual void Initialize() override;
        virtual void Shutdown() override;

        virtual void BeginFrame(const Camera& camera, const glm::mat4& transform) override;
        virtual void BeginFrame(const EditorCamera& camera) override;
        virtual void EndFrame() override;
        virtual void SubmitMesh(MeshComponent& meshComp, const glm::mat4& transform) override;

        virtual void BeginRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Framebuffer>& framebuffer) override;
        virtual void EndRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer) override;

        // Vulkan Specific
        VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetAllocateInfo allocInfo, bool resetEveryFrame, int index = -1);
        void FreeDescriptorSet(VkDescriptorSet& set, bool resetEveryFrame, int index = -1);

        Vector<VkDescriptorPool> GetDescriptorPools() { return mDescriptorPools; }

    private:
        Vector<VkDescriptorPool> mDescriptorPools;
        Vector<VkDescriptorPool> mNonResetableDescriptorPools;
    };

} // namespace Surge
