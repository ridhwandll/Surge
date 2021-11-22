// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanFramebuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanShader.hpp"
#include "VulkanUniformBuffer.hpp"
#include "VulkanGraphicsPipeline.hpp"
#include "VulkanVertexBuffer.hpp"
#include "VulkanIndexBuffer.hpp"

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

        VkDescriptorPoolSize poolSizes[] =
            {{VK_DESCRIPTOR_TYPE_SAMPLER, 10000},
             {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10000},
             {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100},
             {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100},
             {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100},
             {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100},
             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10000},
             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100},
             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100},
             {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100}};

        VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 100 * (sizeof(poolSizes) / sizeof(VkDescriptorPoolSize));
        poolInfo.poolSizeCount = (Uint)(sizeof(poolSizes) / sizeof(VkDescriptorPoolSize));
        poolInfo.pPoolSizes = poolSizes;

        mDescriptorPools.resize(FRAMES_IN_FLIGHT);
        for (auto& descriptorPool : mDescriptorPools)
        {
            VK_CALL(vkCreateDescriptorPool(renderContext->GetDevice()->GetLogicalDevice(), &poolInfo, nullptr, &descriptorPool));
            SET_VK_OBJECT_DEBUGNAME(descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "DescriptorPool");
        }
        mNonResetableDescriptorPools.resize(FRAMES_IN_FLIGHT);
        for (auto& descriptorPool : mNonResetableDescriptorPools)
        {
            VK_CALL(vkCreateDescriptorPool(renderContext->GetDevice()->GetLogicalDevice(), &poolInfo, nullptr, &descriptorPool));
            SET_VK_OBJECT_DEBUGNAME(descriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL, "NonResetable DescriptorPool");
        }

        // TODO: Come up with a better way of managing descriptor sets
        Ref<VulkanShader> mainPBRShader = Core::GetRenderer()->GetShader("Simple").As<VulkanShader>();
        VkDescriptorSetAllocateInfo allocInfo {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &mainPBRShader->GetDescriptorSetLayouts().at(1);

        mLightsDescriptorSets.resize(FRAMES_IN_FLIGHT);
        for (Uint i = 0; i < mLightsDescriptorSets.size(); i++)
            mLightsDescriptorSets[i] = AllocateDescriptorSet(allocInfo, false, i);
        mLightUniformBuffer = UniformBuffer::Create(sizeof(LightUniformBufferData));

        // Geometry Pipeline - TODO: Move to Render Procedure API
        GraphicsPipelineSpecification pipelineSpec {};
        pipelineSpec.Shader = mainPBRShader;
        pipelineSpec.Topology = PrimitiveTopology::TriangleList;
        pipelineSpec.CullingMode = CullMode::Back;
        pipelineSpec.UseDepth = true;
        pipelineSpec.UseStencil = false;
        pipelineSpec.DebugName = "MeshPipeline";
        pipelineSpec.LineWidth = 1.0f;
        pipelineSpec.TargetFramebuffer = Core::GetRenderer()->GetFramebuffer();
        mData->mGeometryPipeline = GraphicsPipeline::Create(pipelineSpec);
    }

    void VulkanRenderer::Shutdown()
    {
        SURGE_PROFILE_FUNC("VulkanRenderer::Shutdown()");
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();

        vkDeviceWaitIdle(device);
        for (auto& descriptorPool : mDescriptorPools)
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        for (auto& descriptorPool : mNonResetableDescriptorPools)
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        mData->ShaderSet.Shutdown();
        mData.reset();
    }

    void VulkanRenderer::EndFrame()
    {
        SURGE_PROFILE_FUNC("VulkanRenderer::EndFrame()");

        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();
        Uint frameIndex = renderContext->GetFrameIndex();
        VkCommandBuffer vulkanCmdBuffer = mData->RenderCmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex);

        mData->RenderCmdBuffer->BeginRecording();

        // ViewProjection and Transform
        glm::mat4 pushConstantData[2] = {};
        pushConstantData[0] = mData->ViewProjection;

        // Light UBO data
        VkWriteDescriptorSet writeDescriptorSet {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.pBufferInfo = &mLightUniformBuffer.As<VulkanUniformBuffer>()->GetDescriptorBufferInfo();
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.dstSet = mLightsDescriptorSets[frameIndex];
        mData->LightData.CameraPosition = mData->CameraPosition;
        mData->LightData.PointLightCount = Uint(mData->PointLights.size());
        for (Uint i = 0; i < mData->LightData.PointLightCount; i++)
            mData->LightData.PointLights[i] = mData->PointLights[i];

        mLightUniformBuffer->SetData(&mData->LightData);
        vkUpdateDescriptorSets(renderContext->GetDevice()->GetLogicalDevice(), 1, &writeDescriptorSet, 0, nullptr);
        vkCmdBindDescriptorSets(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mData->mGeometryPipeline.As<VulkanGraphicsPipeline>()->GetPipelineLayout(), 1, 1, &mLightsDescriptorSets[frameIndex], 0, nullptr);

        BeginRenderPass(vulkanCmdBuffer, mData->OutputFrambuffer);
        for (const DrawCommand& object : mData->DrawList)
        {
            const Ref<Mesh>& mesh = object.MeshComp->Mesh;
            mData->mGeometryPipeline->Bind(mData->RenderCmdBuffer);

            // Bind vertex buffer
            VkDeviceSize offset = 0;
            VkBuffer vertexBuffer = mesh->GetVertexBuffer().As<VulkanVertexBuffer>()->GetVulkanBuffer();
            vkCmdBindVertexBuffers(vulkanCmdBuffer, 0, 1, &vertexBuffer, &offset);

            // Bind index buffer
            VkBuffer indexBuffer = mesh->GetIndexBuffer().As<VulkanIndexBuffer>()->GetVulkanBuffer();
            vkCmdBindIndexBuffer(vulkanCmdBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            object.MeshComp->Material->Bind(mData->RenderCmdBuffer, mData->mGeometryPipeline);

            const Submesh* submeshes = mesh->GetSubmeshes().data();
            for (Uint i = 0; i < mesh->GetSubmeshes().size(); i++)
            {
                const Submesh& submesh = submeshes[i];
                pushConstantData[1] = object.Transform * submesh.Transform;
                mData->mGeometryPipeline->SetPushConstantData(mData->RenderCmdBuffer, "uFrameData", &pushConstantData);
                mData->mGeometryPipeline->DrawIndexed(mData->RenderCmdBuffer, submesh.IndexCount, submesh.BaseIndex, submesh.BaseVertex);
            }
        }
        EndRenderPass(vulkanCmdBuffer);

        mData->RenderCmdBuffer->EndRecording();
        mData->RenderCmdBuffer->Submit();

        mData->DrawList.clear();
        mData->PointLights.clear();
    }

    void VulkanRenderer::BeginRenderPass(VkCommandBuffer& cmdBuffer, const Ref<Framebuffer>& framebuffer)
    {
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

        vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
        vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
        vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }

    void VulkanRenderer::EndRenderPass(VkCommandBuffer& cmdBuffer)
    {
        vkCmdEndRenderPass(cmdBuffer);
    }

    VkDescriptorSet VulkanRenderer::AllocateDescriptorSet(VkDescriptorSetAllocateInfo allocInfo, bool resetEveryFrame, int index)
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();

        int poolIndex = index == -1 ? renderContext->GetFrameIndex() : index;
        resetEveryFrame ? allocInfo.descriptorPool = mDescriptorPools[poolIndex] : allocInfo.descriptorPool = mNonResetableDescriptorPools[poolIndex];

        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        VK_CALL(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
        return descriptorSet;
    }

    void VulkanRenderer::FreeDescriptorSet(VkDescriptorSet& set, bool resetEveryFrame, int index)
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();

        VkDescriptorPool pool = VK_NULL_HANDLE;

        int poolIndex = index == -1 ? renderContext->GetFrameIndex() : index;
        resetEveryFrame ? pool = mDescriptorPools[poolIndex] : pool = mNonResetableDescriptorPools[poolIndex];

        vkFreeDescriptorSets(device, pool, 1, &set);
        set = VK_NULL_HANDLE;
    }
} // namespace Surge