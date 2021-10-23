// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanFramebuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"

namespace Surge
{
    void VulkanRenderer::Initialize()
    {
        SURGE_PROFILE_FUNC("VulkanRenderer::Initialize()");
        mData = CreateScope<RendererData>();
        mData->RenderCmdBuffer = RenderCommandBuffer::Create(false);
        mData->ShaderSet.Initialize(BASE_SHADER_PATH);
        mData->ShaderSet.AddShader("Simple.glsl");
        mData->ShaderSet.LoadAll();

        FramebufferSpecification spec = {};
        spec.Formats = {ImageFormat::RGBA8, ImageFormat::Depth32};
        spec.Width = 1280;
        spec.Height = 720;
        mData->OutputFrambuffer = Framebuffer::Create(spec);

        // Creating n descriptor pools for allocating descriptor sets by the application (n = frames in flight)
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);

        mDescriptorPool.resize(FRAMES_IN_FLIGHT);
        for (auto& descriptorPool : mDescriptorPool)
        {
            VkDescriptorPoolSize poolSizes[] =
                {{VK_DESCRIPTOR_TYPE_SAMPLER, 100},
                 {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100},
                 {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100},
                 {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100},
                 {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100},
                 {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100},
                 {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
                 {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
                 {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100},
                 {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100},
                 {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100}};

            VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
            poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            poolInfo.maxSets = 100 * (sizeof(poolSizes) / sizeof(VkDescriptorPoolSize));
            poolInfo.poolSizeCount = (Uint)(sizeof(poolSizes) / sizeof(VkDescriptorPoolSize));
            poolInfo.pPoolSizes = poolSizes;
            VK_CALL(vkCreateDescriptorPool(renderContext->GetDevice()->GetLogicalDevice(), &poolInfo, nullptr, &descriptorPool));
        }

        // Geometry Pipeline
        GraphicsPipelineSpecification pipelineSpec {};
        pipelineSpec.Shader = SurgeCore::GetRenderer()->GetShader("Simple"); // TODO: Should be handled by material
        pipelineSpec.Topology = PrimitiveTopology::TriangleList;
        pipelineSpec.CullingMode = CullMode::Back;
        pipelineSpec.UseDepth = true;
        pipelineSpec.UseStencil = false;
        pipelineSpec.DebugName = "MeshPipeline";
        pipelineSpec.LineWidth = 1.0f;
        pipelineSpec.TargetFramebuffer = SurgeCore::GetRenderer()->GetFramebuffer();
        mData->mGeometryPipeline = GraphicsPipeline::Create(pipelineSpec);
    }

    void VulkanRenderer::Shutdown()
    {
        SURGE_PROFILE_FUNC("VulkanRenderer::Shutdown()");
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();

        vkDeviceWaitIdle(device);
        for (auto& descriptorPool : mDescriptorPool)
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        mData->ShaderSet.Shutdown();
        mData.reset();
    }

    void VulkanRenderer::BeginFrame(const Camera& camera, const glm::mat4& transform)
    {
        SURGE_PROFILE_FUNC("VulkanRenderer::BeginFrame(Camera)");
        mData->ViewMatrix = glm::inverse(transform);
        mData->ProjectionMatrix = camera.GetProjectionMatrix();
        mData->ViewProjection = mData->ProjectionMatrix * mData->ViewMatrix;
        mData->DrawList.clear();
    }

    void VulkanRenderer::BeginFrame(const EditorCamera& camera)
    {
        SURGE_PROFILE_FUNC("VulkanRenderer::BeginFrame(EditorCamera)");
        mData->ViewMatrix = camera.GetViewMatrix();
        mData->ProjectionMatrix = camera.GetProjectionMatrix();
        mData->ViewProjection = mData->ProjectionMatrix * mData->ViewMatrix;
        mData->DrawList.clear();
    }

    void VulkanRenderer::EndFrame()
    {
        SURGE_PROFILE_FUNC("VulkanRenderer::EndFrame()");
        mData->RenderCmdBuffer->BeginRecording();

        // ViewProjection and Transform
        glm::mat4 pushConstantData[2] = {};
        pushConstantData[0] = mData->ViewProjection;
        BeginRenderPass(mData->RenderCmdBuffer, mData->OutputFrambuffer);
        for (const DrawCommand& object : mData->DrawList)
        {
            const Ref<Mesh>& mesh = object.Mesh;
            mData->mGeometryPipeline->Bind(mData->RenderCmdBuffer);
            mesh->GetVertexBuffer()->Bind(mData->RenderCmdBuffer);
            mesh->GetIndexBuffer()->Bind(mData->RenderCmdBuffer);

            const Submesh* submeshes = mesh->GetSubmeshes().data();
            for (Uint i = 0; i < mesh->GetSubmeshes().size(); i++)
            {
                const Submesh& submesh = submeshes[i];
                pushConstantData[1] = object.Transform * submesh.Transform;
                mData->mGeometryPipeline->SetPushConstantData(mData->RenderCmdBuffer, "uFrameData", &pushConstantData);
                mData->mGeometryPipeline->DrawIndexed(mData->RenderCmdBuffer, submesh.IndexCount, submesh.BaseIndex, submesh.BaseVertex);
            }
        }
        EndRenderPass(mData->RenderCmdBuffer);

        mData->RenderCmdBuffer->EndRecording();
        mData->RenderCmdBuffer->Submit();
    }

    void VulkanRenderer::SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform)
    {
        mData->DrawList.emplace_back(mesh, transform);
    }

    void VulkanRenderer::BeginRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Framebuffer>& framebuffer)
    {
        SURGE_PROFILE_FUNC("VulkanRenderer::BeginRenderPass()");
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        Uint frameIndex = renderContext->GetFrameIndex();
        VkCommandBuffer vulkanCmdBuffer = cmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex);
        const FramebufferSpecification& framebufferSpec = framebuffer->GetSpecification();

        std::array<VkClearValue, 2> clearValues;
        clearValues[0].color = {framebufferSpec.ClearColor.r, framebufferSpec.ClearColor.g, framebufferSpec.ClearColor.b, framebufferSpec.ClearColor.a};
        clearValues[1].depthStencil = {1.0f, 0};

        VkViewport viewport = {};
        viewport.width = float(framebufferSpec.Width);
        viewport.height = float(framebufferSpec.Height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.extent = {framebufferSpec.Width, framebufferSpec.Height};
        scissor.offset = {0, 0};

        VkRenderPassBeginInfo renderPassBeginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
        renderPassBeginInfo.renderPass = framebuffer.As<VulkanFramebuffer>()->GetVulkanRenderPass();
        renderPassBeginInfo.renderArea.offset = {0, 0};
        renderPassBeginInfo.renderArea.extent = {framebufferSpec.Width, framebufferSpec.Height};
        renderPassBeginInfo.framebuffer = framebuffer.As<VulkanFramebuffer>()->GetVulkanFramebuffer();
        renderPassBeginInfo.clearValueCount = Uint(clearValues.size());
        renderPassBeginInfo.pClearValues = clearValues.data();

        vkCmdSetViewport(vulkanCmdBuffer, 0, 1, &viewport);
        vkCmdSetScissor(vulkanCmdBuffer, 0, 1, &scissor);
        vkCmdBeginRenderPass(vulkanCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanRenderer::EndRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer)
    {
        SURGE_PROFILE_FUNC("VulkanRenderer::EndRenderPass()");
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        Uint frameIndex = renderContext->GetFrameIndex();
        vkCmdEndRenderPass(cmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex));
    }

    VkDescriptorSet VulkanRenderer::AllocateDescriptorSet(VkDescriptorSetAllocateInfo allocInfo)
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();
        Uint currentFrameIndex = renderContext->GetFrameIndex();

        VkDescriptorSet descriptorSet;
        allocInfo.descriptorPool = mDescriptorPool[currentFrameIndex];
        VK_CALL(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));

        return descriptorSet;
    }

} // namespace Surge
