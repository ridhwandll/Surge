// Copyright (c) - SurgeTechnologies - All rights reserved
#include "VulkanDescriptorSet.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDescriptorSet.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanShader.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUniformBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanGraphicsPipeline.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanImage.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanStorageBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanComputePipeline.hpp"

namespace Surge
{
    VulkanDescriptorSet::VulkanDescriptorSet(const Ref<Shader>& shader, Uint setNumber, bool resetEveryFrame, int index)
        : mSetNumber(setNumber)
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);

        VkDescriptorSetAllocateInfo allocInfo {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &shader.As<VulkanShader>()->GetDescriptorSetLayouts().at(setNumber);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();

        mDescriptorSets.resize(FRAMES_IN_FLIGHT);
        for (int index = 0; index < FRAMES_IN_FLIGHT; index++)
        {
            allocInfo.descriptorPool = renderContext->GetNonResetableDescriptorPools()[index];
            VK_CALL(vkAllocateDescriptorSets(device, &allocInfo, &mDescriptorSets[index]));
        }
    }

    VulkanDescriptorSet::~VulkanDescriptorSet()
    {
        VulkanRenderContext* renderContext = nullptr;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();

        for (int index = 0; index < mDescriptorSets.size(); index++)
        {
            VkDescriptorPool pool = renderContext->GetNonResetableDescriptorPools()[index];
            vkFreeDescriptorSets(device, pool, 1, &mDescriptorSets[index]);
            mDescriptorSets[index] = VK_NULL_HANDLE;
        }
    }

    void VulkanDescriptorSet::Bind(const Ref<RenderCommandBuffer>& commandBuffer, const Ref<GraphicsPipeline>& pipeline)
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        Uint frameIndex = renderContext->GetFrameIndex();
        VkCommandBuffer vulkanCmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex);
        vkCmdBindDescriptorSets(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.As<VulkanGraphicsPipeline>()->GetPipelineLayout(), mSetNumber, 1, &mDescriptorSets[frameIndex], 0, nullptr);
    }

    void VulkanDescriptorSet::Bind(const Ref<RenderCommandBuffer>& commandBuffer, const Ref<ComputePipeline>& pipeline)
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        Uint frameIndex = renderContext->GetFrameIndex();
        VkCommandBuffer vulkanCmdBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex);
        vkCmdBindDescriptorSets(vulkanCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.As<VulkanComputePipeline>()->GetPipelineLayout(), mSetNumber, 1, &mDescriptorSets[frameIndex], 0, nullptr);
    }

    void VulkanDescriptorSet::UpdateForRendering()
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();
        Uint frameIndex = renderContext->GetFrameIndex();

        // TODO: Check for previous resources
        if (!mPendingBuffers.empty() || !mPendingImages.empty() || !mPendingStorageBuffers.empty())
        {
            Vector<VkWriteDescriptorSet> writeDescriptorSets;
            for (auto& [binding, buffer] : mPendingBuffers)
            {
                VkWriteDescriptorSet writeDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                writeDescriptorSet.dstBinding = binding;
                writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                writeDescriptorSet.pBufferInfo = &buffer.As<VulkanUniformBuffer>()->GetVulkanDescriptorBufferInfo();
                writeDescriptorSet.descriptorCount = 1;
                writeDescriptorSet.dstSet = mDescriptorSets[frameIndex];
                writeDescriptorSets.push_back(writeDescriptorSet);
            }
            for (auto& [binding, buffer] : mPendingStorageBuffers)
            {
                VkWriteDescriptorSet writeDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                writeDescriptorSet.dstBinding = binding;
                writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                writeDescriptorSet.pBufferInfo = &buffer.As<VulkanStorageBuffer>()->GetVulkanDescriptorBufferInfo();
                writeDescriptorSet.descriptorCount = 1;
                writeDescriptorSet.dstSet = mDescriptorSets[frameIndex];
                writeDescriptorSets.push_back(writeDescriptorSet);
            }
            for (auto& [binding, image] : mPendingImages)
            {
                const ImageSpecification& spec = image->GetSpecification();
                VkWriteDescriptorSet writeDescriptorSet = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
                writeDescriptorSet.dstBinding = binding;
                writeDescriptorSet.descriptorType = spec.Usage == ImageUsage::Storage ? VK_DESCRIPTOR_TYPE_STORAGE_IMAGE : VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                writeDescriptorSet.pImageInfo = &image.As<VulkanImage2D>()->GetVulkanDescriptorImageInfo();
                writeDescriptorSet.descriptorCount = 1;
                writeDescriptorSet.dstSet = mDescriptorSets[frameIndex];
                writeDescriptorSets.push_back(writeDescriptorSet);
            }
            vkUpdateDescriptorSets(device, static_cast<Uint>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

            mPendingStorageBuffers.clear();
            mPendingBuffers.clear();
            mPendingImages.clear();
        }
    }

} // namespace Surge
