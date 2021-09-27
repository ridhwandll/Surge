// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Renderer.hpp"
#include "Surge/Graphics/VertexBuffer.hpp"
#include "Surge/Graphics/IndexBuffer.hpp"
#include "Surge/Graphics/GraphicsPipeline.hpp"
#include "Surge/Graphics/Texture.hpp"
#include "Abstraction/Vulkan/VulkanGraphicsPipeline.hpp"
#include "Abstraction/Vulkan/VulkanShader.hpp"
#include "Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanImage.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace Surge
{
    // TODO: Very temporary, move initialization to something like "RendererBase"
    struct RendererData
    {
        Ref<VertexBuffer> VertexBuffer;
        Ref<IndexBuffer> IndexBuffer;
        Ref<GraphicsPipeline> Pipeline;
        Ref<RenderCommandBuffer> RenderCmdBuffer;
        Ref<Texture2D> TestTexture;
        VkDescriptorPool DescriptorPool;
        VkDescriptorSet DescriptorSet;
    };

    // TODO: This should be per Renderer Instance!
    static Scope<RendererData> sData = CreateScope<RendererData>();

    // TODO: Remove this
    struct Vertex
    {
        glm::vec3 Pos;
        glm::vec2 TexCoord;
    };

    const std::vector<Vertex> vertices =
    {
        // positions         // texture coords
        {{  0.5f,  0.5f, 0.0f, }, { 1.0f, 0.0f }}, // top right
        {{  0.5f, -0.5f, 0.0f, }, { 1.0f, 1.0f }}, // bottom right
        {{ -0.5f, -0.5f, 0.0f, }, { 0.0f, 1.0f }}, // bottom left
        {{ -0.5f,  0.5f, 0.0f, }, { 0.0f, 0.0f }}  // top left 
    };
    const std::vector<Uint> indices = { 0, 1, 2, 2, 3, 0 };

    void Renderer::Initialize()
    {
        mBase.Initialize();
        sData->VertexBuffer = VertexBuffer::Create(vertices.data(), static_cast<Uint>(sizeof(Vertex) * vertices.size()));
        sData->IndexBuffer = IndexBuffer::Create(indices.data(), static_cast<Uint>(sizeof(indices[0]) * indices.size()));

        GraphicsPipelineSpecification pipelineSpec{};
        pipelineSpec.Shader = mBase.GetShader("Simple");
        pipelineSpec.Topology = PrimitiveTopology::TriangleStrip;
        pipelineSpec.UseDepth = true;
        pipelineSpec.UseStencil = false;
        pipelineSpec.DebugName = "TestPipeline";
        sData->Pipeline = GraphicsPipeline::Create(pipelineSpec);

        sData->RenderCmdBuffer = RenderCommandBuffer::Create(true);

        {
            // TODO(AC3R): Temporary
            VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
            VkDevice device = vkContext->GetDevice()->GetLogicalDevice();
            
            TextureSpecification textureSpec{};
            textureSpec.UseMips = true;
            textureSpec.ShaderUsage = { ShaderType::Pixel };
            textureSpec.Sampler.SamplerFilter = TextureFilter::Linear;
            textureSpec.Sampler.SamplerAddressMode = TextureAddressMode::Repeat;
            sData->TestTexture = Texture2D::Create("Engine/Assets/Textures/kekwCool.png", textureSpec);

            VkDescriptorPoolSize poolSizes[] =
            {
                { VK_DESCRIPTOR_TYPE_SAMPLER, 100 },
                { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
                { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 },
                { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100 },
                { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
                { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
                { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
                { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 }
            };
            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 100 * (sizeof(poolSizes) / sizeof(VkDescriptorPoolSize));
            pool_info.poolSizeCount = (uint32_t)(sizeof(poolSizes) / sizeof(VkDescriptorPoolSize));
            pool_info.pPoolSizes = poolSizes;
            VK_CALL(vkCreateDescriptorPool(device, &pool_info, nullptr, &sData->DescriptorPool));

            auto descriptorSetLayout = mBase.GetShader("Simple").As<VulkanShader>()->GetDescriptorSetLayouts();

            VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
            allocInfo.descriptorPool = sData->DescriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &descriptorSetLayout[0];
            VK_CALL(vkAllocateDescriptorSets(device, &allocInfo, &sData->DescriptorSet));

            VkDescriptorImageInfo imageInfo = sData->TestTexture->GetImage2D().As<VulkanImage2D>()->GetVulkanDescriptorInfo();

            VkWriteDescriptorSet writeDescriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
            writeDescriptorSet.dstBinding = 0;
            writeDescriptorSet.dstArrayElement = 0;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSet.pImageInfo = &imageInfo;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.dstSet = sData->DescriptorSet;

            vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
        }
    }

    void Renderer::RenderRectangle(const glm::vec3& position, const glm::vec3& scale)
    {
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanSwapChain* swapchain = static_cast<VulkanSwapChain*>(vkContext->GetSwapChain());
        VkExtent2D extent = swapchain->GetVulkanExtent2D();

        glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), extent.width / (float)extent.height, 0.1f, 1000.0f);
        projection[1][1] *= -1;

        struct FrameData
        {
            glm::mat4 ViewProjection;
            glm::mat4 Transform;
        } uFrameData;
        uFrameData.ViewProjection = projection * view;
        uFrameData.Transform = glm::translate(glm::mat4(1.0f), position) * glm::scale(glm::mat4(1.0f), scale);

        // Begin command buffer recording
        const Ref<RenderCommandBuffer>& cmd = sData->RenderCmdBuffer;
        sData->RenderCmdBuffer->BeginRecording();
        swapchain->BeginRenderPass();

        sData->Pipeline->Bind(cmd);
        sData->VertexBuffer->Bind(cmd);
        sData->IndexBuffer->Bind(cmd);
        sData->Pipeline->SetPushConstantData(cmd, "uFrameData", (void*)&uFrameData);

        // TODO(AC3R): Temporary
        Uint frameIndex = vkContext->GetFrameIndex();
        VkCommandBuffer vulkanCmdBuffer = sData->RenderCmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex);
        VkPipelineLayout pipelineLayout = sData->Pipeline.As<VulkanGraphicsPipeline>()->GetPipelineLayout();
        vkCmdBindDescriptorSets(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &sData->DescriptorSet, 0, nullptr);

        sData->Pipeline->DrawIndexed(cmd, indices.size());

        vkContext->RenderImGui();

        swapchain->EndRenderPass();
        sData->RenderCmdBuffer->EndRecording();
    }

    void Renderer::Shutdown()
    {
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VkDevice device = vkContext->GetDevice()->GetLogicalDevice();
        vkDeviceWaitIdle(device);
        vkDestroyDescriptorPool(device, sData->DescriptorPool, nullptr);
        sData.reset();
        mBase.Shutdown();
    }
}
