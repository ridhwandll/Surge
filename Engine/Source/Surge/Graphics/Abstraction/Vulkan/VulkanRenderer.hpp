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
        virtual void SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform) override;

        virtual void BeginRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Framebuffer>& framebuffer) override;
        virtual void EndRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer) override;

        VkDescriptorSet AllocateDescriptorSet(VkDescriptorSetAllocateInfo allocInfo);
        VkDescriptorPool GetDescriptorPool()
        {
            Uint frameIndex = SurgeCore::GetRenderContext()->GetFrameIndex();
            return mDescriptorPool[frameIndex];
        }

    private:
        Vector<VkDescriptorPool> mDescriptorPool;
    };

} // namespace Surge
