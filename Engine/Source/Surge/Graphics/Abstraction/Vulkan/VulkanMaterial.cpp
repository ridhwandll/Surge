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
        mShader = shader;
        mShaderReloadID = mShader->AddReloadCallback([&]() { Load(); });
        Load();
    }

    VulkanMaterial::~VulkanMaterial()
    {
        Release();
        mShader->RemoveReloadCallback(mShaderReloadID);
    }

    void VulkanMaterial::Load()
    {
        Release();

        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        Ref<VulkanShader> vulkanShader = mShader.As<VulkanShader>();

        Uint set;
        const Vector<ShaderBuffer>& shaderBuffers = vulkanShader->GetReflectionData().GetBuffers();
        for (auto& shaderBuffer : shaderBuffers)
        {
            if (shaderBuffer.Set == 0) // Descriptor set 0 is the material
            {
                mBinding = shaderBuffer.Binding;
                set = shaderBuffer.Set;
                break;
            }
            //SG_ASSERT_INTERNAL("Cannot find a suitable uniform buffer, on which Material should work on!")
        }

        mShaderBuffer = mShader->GetReflectionData().GetBuffer("Material");
        mBufferMemory.Allocate(mShaderBuffer.Size);
        mBufferMemory.ZeroInitialize();
        mUniformBuffer = UniformBuffer::Create(mShaderBuffer.Size);

        VkDescriptorSetAllocateInfo allocInfo {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &vulkanShader->GetDescriptorSetLayouts().at(set);

        mDescriptorSets.resize(FRAMES_IN_FLIGHT);
        for (Uint i = 0; i < mDescriptorSets.size(); i++)
            mDescriptorSets[i] = static_cast<VulkanRenderer*>(SurgeCore::GetRenderer())->AllocateDescriptorSet(allocInfo, false, i);
    }

    void VulkanMaterial::Bind(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<GraphicsPipeline>& gfxPipeline) const
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);

        Uint frameIndex = SurgeCore::GetRenderContext()->GetFrameIndex();
        VkCommandBuffer vulkanCommandBuffer = cmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex);
        Ref<VulkanGraphicsPipeline> vulkanPipeline = gfxPipeline.As<VulkanGraphicsPipeline>();

        VkWriteDescriptorSet writeDescriptorSet {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        writeDescriptorSet.dstBinding = mBinding;
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.pBufferInfo = &mUniformBuffer.As<VulkanUniformBuffer>()->GetDescriptorBufferInfo();
        writeDescriptorSet.descriptorCount = 1;
        writeDescriptorSet.dstSet = mDescriptorSets[frameIndex];

        mUniformBuffer->SetData(mBufferMemory);
        vkUpdateDescriptorSets(renderContext->GetDevice()->GetLogicalDevice(), 1, &writeDescriptorSet, 0, nullptr);
        vkCmdBindDescriptorSets(vulkanCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetPipelineLayout(), 0, 1, &mDescriptorSets[frameIndex], 0, nullptr);
    }

    void VulkanMaterial::Release()
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice logicalDevice = renderContext->GetDevice()->GetLogicalDevice();
        vkDeviceWaitIdle(logicalDevice);

        for (Uint i = 0; i < mDescriptorSets.size(); i++)
            static_cast<VulkanRenderer*>(SurgeCore::GetRenderer())->FreeDescriptorSet(mDescriptorSets[i], false, i);

        mBufferMemory.Release();
    }
} // namespace Surge