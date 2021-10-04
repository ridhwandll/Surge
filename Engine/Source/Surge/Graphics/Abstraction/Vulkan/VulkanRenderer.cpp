// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderer.hpp"
#include "VulkanRenderContext.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "Surge/Graphics/IndexBuffer.hpp"
#include "Surge/Graphics/VertexBuffer.hpp"
#include "VulkanGraphicsPipeline.hpp"
#include "VulkanImage.hpp"
#include "VulkanRenderCommandBuffer.hpp"
#include "VulkanShader.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Surge
{
    struct Vertexkek
    {
        glm::vec3 Pos;
        glm::vec2 TexCoord;
    };
    const std::vector<Vertexkek> vertices = {
        // positions         // texture coords
        {{
             0.5f,
             0.5f,
             0.0f,
         },
         {1.0f, 0.0f}}, // top right
        {{
             0.5f,
             -0.5f,
             0.0f,
         },
         {1.0f, 1.0f}}, // bottom right
        {{
             -0.5f,
             -0.5f,
             0.0f,
         },
         {0.0f, 1.0f}}, // bottom left
        {{
             -0.5f,
             0.5f,
             0.0f,
         },
         {0.0f, 0.0f}} // top left
    };
    const std::vector<Uint> indices = {0, 1, 2, 2, 3, 0};

    struct TextureDataTemp
    {
        Ref<VertexBuffer> sVertexBuffer;
        Ref<IndexBuffer> sIndexBuffer;
        Ref<GraphicsPipeline> Pipeline;
        Ref<Texture2D> TestTexture;
        VkDescriptorPool DescriptorPool;
        VkDescriptorSet DescriptorSet;
    };
    TextureDataTemp* sTempData = new TextureDataTemp;

    void VulkanRenderer::Initialize()
    {
        mData = CreateScope<RendererData>();
        mData->RenderCmdBuffer = RenderCommandBuffer::Create(true);
        mData->ShaderSet.Initialize(BASE_SHADER_PATH);
        mData->ShaderSet.AddShader("Simple.glsl");
        mData->ShaderSet.LoadAll();

        sTempData->sVertexBuffer = VertexBuffer::Create(vertices.data(), Uint(sizeof(Vertexkek) * vertices.size()));
        sTempData->sIndexBuffer = IndexBuffer::Create(indices.data(), Uint(sizeof(indices[0]) * indices.size()));
        GraphicsPipelineSpecification pipelineSpec {};
        pipelineSpec.Shader = GetShader("Simple");
        pipelineSpec.Topology = PrimitiveTopology::TriangleStrip;
        pipelineSpec.UseDepth = true;
        pipelineSpec.UseStencil = false;
        pipelineSpec.DebugName = "TestPipeline";
        pipelineSpec.CullingMode = CullMode::None;
        sTempData->Pipeline = GraphicsPipeline::Create(pipelineSpec);

        {
            VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
            VkDevice device = vkContext->GetDevice()->GetLogicalDevice();

            TextureSpecification textureSpec {};
            textureSpec.UseMips = true;
            textureSpec.ShaderUsage = {ShaderType::Pixel};
            sTempData->TestTexture = Texture2D::Create("Engine/Assets/Textures/kekwCool.png", textureSpec);

            VkDescriptorPoolSize pool_sizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 100},
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
            VkDescriptorPoolCreateInfo pool_info = {};
            pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            pool_info.maxSets = 100 * (sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize));
            pool_info.poolSizeCount = (uint32_t)(sizeof(pool_sizes) / sizeof(VkDescriptorPoolSize));
            pool_info.pPoolSizes = pool_sizes;
            VK_CALL(vkCreateDescriptorPool(device, &pool_info, nullptr, &sTempData->DescriptorPool));

            auto descriptorSetLayout = GetShader("Simple").As<VulkanShader>()->GetDescriptorSetLayouts();

            VkDescriptorSetAllocateInfo allocInfo {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            allocInfo.descriptorPool = sTempData->DescriptorPool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &descriptorSetLayout[0];
            VK_CALL(vkAllocateDescriptorSets(device, &allocInfo, &sTempData->DescriptorSet));

            VkDescriptorImageInfo imageInfo = sTempData->TestTexture->GetImage2D().As<VulkanImage2D>()->GetVulkanDescriptorInfo();

            VkWriteDescriptorSet writeDescriptorSet {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            writeDescriptorSet.dstBinding = 0;
            writeDescriptorSet.dstArrayElement = 0;
            writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            writeDescriptorSet.pImageInfo = &imageInfo;
            writeDescriptorSet.descriptorCount = 1;
            writeDescriptorSet.dstSet = sTempData->DescriptorSet;

            vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
        }
    }

    void VulkanRenderer::Shutdown()
    {
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VkDevice device = vkContext->GetDevice()->GetLogicalDevice();
        vkDeviceWaitIdle(device);

        vkDestroyDescriptorPool(device, sTempData->DescriptorPool, nullptr);
        mData->ShaderSet.Shutdown();
        mData.reset();

        delete sTempData;
    }

    void VulkanRenderer::BeginFrame(const EditorCamera& camera)
    {
        mData->ViewMatrix = camera.GetViewMatrix();
        mData->ProjectionMatrix = camera.GetProjectionMatrix();
        mData->ViewProjection = mData->ProjectionMatrix * mData->ViewMatrix;
        mData->DrawList.clear();
    }

    void VulkanRenderer::EndFrame()
    {
#if TODO
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanSwapChain* swapchain = static_cast<VulkanSwapChain*>(vkContext->GetSwapChain());

        const Ref<RenderCommandBuffer>& cmd = mData->RenderCmdBuffer;
        mData->RenderCmdBuffer->BeginRecording();
        swapchain->BeginRenderPass();

        // ViewProjection and Transform
        glm::mat4 pushConstantData[2] = {};
        pushConstantData[0] = mData->ViewProjection;

        for (const DrawCommand& object: mData->DrawList)
        {
            const Ref<Mesh>& mesh = object.Mesh;
            const Ref<GraphicsPipeline>& graphicsPipeline = mesh->GetPipeline();
            graphicsPipeline->Bind(cmd);
            mesh->GetVertexBuffer()->Bind(cmd);
            mesh->GetIndexBuffer()->Bind(cmd);

            const Submesh* submeshes = mesh->GetSubmeshes().data();
            for (Uint i = 0; i < mesh->GetSubmeshes().size(); i++)
            {
                const Submesh& submesh = submeshes[i];
                pushConstantData[1] = object.Transform * submesh.Transform;
                graphicsPipeline->SetPushConstantData(cmd, "uFrameData", &pushConstantData);
                graphicsPipeline->DrawIndexed(cmd, submesh.IndexCount, submesh.BaseIndex, submesh.BaseVertex);
            }
        }

        vkContext->RenderImGui();
        swapchain->EndRenderPass();
        mData->RenderCmdBuffer->EndRecording();
#endif
        VulkanRenderContext* vkContext = static_cast<VulkanRenderContext*>(CoreGetRenderContext().get());
        VulkanSwapChain* swapchain = static_cast<VulkanSwapChain*>(vkContext->GetSwapChain());
        Uint frameIndex = vkContext->GetFrameIndex();

        struct FrameData
        {
            glm::mat4 ViewProjection;
            glm::mat4 Transform;
        } uFrameData;
        uFrameData.ViewProjection = mData->ViewProjection;
        uFrameData.Transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

        // Begin command buffer recording
        const Ref<RenderCommandBuffer>& cmd = mData->RenderCmdBuffer;
        mData->RenderCmdBuffer->BeginRecording();
        swapchain->BeginRenderPass();

        sTempData->Pipeline->Bind(cmd);
        sTempData->sVertexBuffer->Bind(cmd);
        sTempData->sIndexBuffer->Bind(cmd);
        sTempData->Pipeline->SetPushConstantData(cmd, "uFrameData", (void*)&uFrameData);

        VkCommandBuffer vulkanCmdBuffer = mData->RenderCmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex);
        VkPipelineLayout pipelineLayout = sTempData->Pipeline.As<VulkanGraphicsPipeline>()->GetPipelineLayout();
        vkCmdBindDescriptorSets(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &sTempData->DescriptorSet, 0, nullptr);

        sTempData->Pipeline->DrawIndexed(cmd, indices.size(), 0, 0);

        vkContext->RenderImGui();

        swapchain->EndRenderPass();
        mData->RenderCmdBuffer->EndRecording();
    }

    void VulkanRenderer::SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform) { mData->DrawList.emplace_back(mesh, transform); }
} // namespace Surge
