// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMaterial.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanShader.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUniformBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanGraphicsPipeline.hpp"

namespace Surge
{
    VulkanMaterial::VulkanMaterial(const Ref<Shader>& shader)
    {
        mIsLoaded = false;
        mShader = shader;
        mShaderReloadID = mShader->AddReloadCallback([&]() { Load(); });
        Load();
    }

    VulkanMaterial::~VulkanMaterial()
    {
        Release();
        mShader->RemoveReloadCallback(mShaderReloadID);
    }

    void VulkanMaterial::Bind(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<GraphicsPipeline>& gfxPipeline) const
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);

        Uint frameIndex = SurgeCore::GetRenderContext()->GetFrameIndex();
        VkCommandBuffer vulkanCommandBuffer = cmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex);
        Ref<VulkanGraphicsPipeline> vulkanPipeline = gfxPipeline.As<VulkanGraphicsPipeline>();

        VkWriteDescriptorSet writeDescriptorSet {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        writeDescriptorSet.dstBinding = 0;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.pBufferInfo = &mUniformBuffer.As<VulkanUniformBuffer>()->GetDescriptorBufferInfo();
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.dstSet = mDescriptorSets[frameIndex];

        mUniformBuffer->SetData(mBufferMemory);
        vkUpdateDescriptorSets(renderContext->GetDevice()->GetLogicalDevice(), 1, &writeDescriptorSet, 0, nullptr);
        vkCmdBindDescriptorSets(vulkanCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetPipelineLayout(), 0, 1, &mDescriptorSets[frameIndex], 0, nullptr);
    }

    void VulkanMaterial::Load()
    {
        SG_ASSERT(!mIsLoaded, "Material Already Loaded! Release it to Load again xD");

        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);

        mShaderBuffer = mShader->GetReflectionData().GetBuffer("Material");
        mBufferMemory.Allocate(mShaderBuffer.Size);
        mBufferMemory.ZeroInitialize();
        mUniformBuffer = UniformBuffer::Create(mShaderBuffer.Size, 0);

        mDescriptorSets.resize(FRAMES_IN_FLIGHT);
        for (Uint i = 0; i < mDescriptorSets.size(); i++)
        {
            VkDescriptorSetAllocateInfo allocInfo {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &mShader.As<VulkanShader>()->GetDescriptorSetLayouts().at(0);
            allocInfo.descriptorPool = static_cast<VulkanRenderer*>(SurgeCore::GetRenderer())->GetDescriptorPools()[i];
            VK_CALL(vkAllocateDescriptorSets(renderContext->GetDevice()->GetLogicalDevice(), &allocInfo, &mDescriptorSets[i]));
        }

        mIsLoaded = true;
    }

    void VulkanMaterial::Release()
    {
        SG_ASSERT(mIsLoaded, "Material is already released! Load the material to Release it again lol");

        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice logicalDevice = renderContext->GetDevice()->GetLogicalDevice();
        vkDeviceWaitIdle(logicalDevice);

        for (Uint i = 0; i < mDescriptorSets.size(); i++)
        {
            VkDescriptorPool descriptorPool = static_cast<VulkanRenderer*>(SurgeCore::GetRenderer())->GetDescriptorPools()[i];
            vkFreeDescriptorSets(logicalDevice, descriptorPool, 1, &mDescriptorSets[i]);
        }

        mBufferMemory.Release();
        mIsLoaded = false;
    }

} // namespace Surge
