// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMaterial.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanShader.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUniformBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderCommandBuffer.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanGraphicsPipeline.hpp"
#include "VulkanImage.hpp"
#define MATERIAL_SET 0

namespace Surge
{
    VulkanMaterial::VulkanMaterial(const Ref<Shader>& shader, const String& materialName)
    {
        mName = materialName;
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
        VkDevice device = renderContext->GetDevice()->GetLogicalDevice();

        Uint set;
        const Vector<ShaderBuffer>& shaderBuffers = vulkanShader->GetReflectionData().GetBuffers();
        for (auto& shaderBuffer : shaderBuffers)
        {
            if (shaderBuffer.Set == MATERIAL_SET) // Descriptor set MATERIAL_SET(0) is the material; TODO: Automate in some way in future!
            {
                mBinding = shaderBuffer.Binding;
                set = shaderBuffer.Set;
                break;
            }
        }

        const ShaderReflectionData& reflectionData = mShader->GetReflectionData();
        // Buffer
        mShaderBuffer = reflectionData.GetBuffer("Material");
        mBufferMemory.Allocate(mShaderBuffer.Size);
        mBufferMemory.ZeroInitialize();
        mUniformBuffer = UniformBuffer::Create(mShaderBuffer.Size);

        // Texture
        const Vector<ShaderResource>& ress = reflectionData.GetResources();
        for (const ShaderResource& res : ress)
        {
            if (res.Set == 2)
                mShaderResources[res.Binding] = res;
        }
        for (auto& [binding, res] : mShaderResources)
        {
            mTextures[binding] = Core::GetRenderer()->GetData()->WhiteTexture;
            mUpdatePendingTextures.push_back({binding, mTextures.at(binding).Raw()});
        }

        mDescriptorSets.resize(FRAMES_IN_FLIGHT);
        for (Uint i = 0; i < mDescriptorSets.size(); i++)
        {
            VkDescriptorSetAllocateInfo allocInfo {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &vulkanShader->GetDescriptorSetLayouts().at(set);
            allocInfo.descriptorPool = renderContext->GetNonResetableDescriptorPools()[i];
            VK_CALL(vkAllocateDescriptorSets(device, &allocInfo, &mDescriptorSets[i]));
        }
        mTextureDescriptorSets.resize(FRAMES_IN_FLIGHT);
        for (Uint i = 0; i < mTextureDescriptorSets.size(); i++)
        {
            VkDescriptorSetAllocateInfo allocInfo {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &vulkanShader->GetDescriptorSetLayouts().at(2);
            allocInfo.descriptorPool = renderContext->GetNonResetableDescriptorPools()[i];
            VK_CALL(vkAllocateDescriptorSets(device, &allocInfo, &mTextureDescriptorSets[i]));
        }

        // TODO: Remove
        Set<glm::vec3>("Material.Albedo", {1.0f, 1.0f, 1.0f});
    }

    void VulkanMaterial::UpdateForRendering()
    {
        SURGE_PROFILE_FUNC("VulkanMaterial::UpdateForRendering");
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice logicalDevice = renderContext->GetDevice()->GetLogicalDevice();
        Uint frameIndex = Core::GetRenderContext()->GetFrameIndex();

        Vector<VkWriteDescriptorSet> writeDescriptorSets;
        if (!mUpdatePendingTextures.empty())
        {
            // Texture
            for (auto& [binding, texture] : mUpdatePendingTextures)
            {
                if (!texture)
                    continue;

                VkWriteDescriptorSet& textureWriteDescriptorSet = writeDescriptorSets.emplace_back();
                textureWriteDescriptorSet = {};
                textureWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                textureWriteDescriptorSet.dstBinding = binding;
                textureWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                textureWriteDescriptorSet.pImageInfo = &texture->GetImage2D().As<VulkanImage2D>()->GetVulkanDescriptorInfo();
                textureWriteDescriptorSet.descriptorCount = 1;
                textureWriteDescriptorSet.dstSet = mTextureDescriptorSets[frameIndex];
            }
            Uint size = static_cast<Uint>(writeDescriptorSets.size());
            vkUpdateDescriptorSets(logicalDevice, size, writeDescriptorSets.data(), 0, nullptr);

            if (frameIndex == (FRAMES_IN_FLIGHT - 1))
                mUpdatePendingTextures.clear();
        }

        // Buffers
        VkWriteDescriptorSet bufferWriteDescriptorSet = {};
        bufferWriteDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        bufferWriteDescriptorSet.dstBinding = mBinding;
        bufferWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bufferWriteDescriptorSet.pBufferInfo = &mUniformBuffer.As<VulkanUniformBuffer>()->GetDescriptorBufferInfo();
        bufferWriteDescriptorSet.descriptorCount = 1;
        bufferWriteDescriptorSet.dstSet = mDescriptorSets[frameIndex];
        mUniformBuffer->SetData(mBufferMemory);
        vkUpdateDescriptorSets(logicalDevice, 1, &bufferWriteDescriptorSet, 0, nullptr);
    }

    void VulkanMaterial::Bind(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<GraphicsPipeline>& gfxPipeline) const
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        Uint frameIndex = Core::GetRenderContext()->GetFrameIndex();
        VkCommandBuffer vulkanCommandBuffer = cmdBuffer.As<VulkanRenderCommandBuffer>()->GetVulkanCommandBuffer(frameIndex);
        Ref<VulkanGraphicsPipeline> vulkanPipeline = gfxPipeline.As<VulkanGraphicsPipeline>();
        VkPipelineLayout pipelineLayout = vulkanPipeline->GetPipelineLayout();
        vkCmdBindDescriptorSets(vulkanCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &mTextureDescriptorSets[frameIndex], 0, nullptr);
        vkCmdBindDescriptorSets(vulkanCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &mDescriptorSets[frameIndex], 0, nullptr);
    }

    void VulkanMaterial::Release()
    {
        VulkanRenderContext* renderContext;
        SURGE_GET_VULKAN_CONTEXT(renderContext);
        VkDevice logicalDevice = renderContext->GetDevice()->GetLogicalDevice();
        vkDeviceWaitIdle(logicalDevice);

        for (Uint i = 0; i < mDescriptorSets.size(); i++)
        {
            vkFreeDescriptorSets(logicalDevice, renderContext->GetNonResetableDescriptorPools()[i], 1, &mDescriptorSets[i]);
            mDescriptorSets[i] = VK_NULL_HANDLE;
        }
        for (Uint i = 0; i < mTextureDescriptorSets.size(); i++)
        {
            vkFreeDescriptorSets(logicalDevice, renderContext->GetNonResetableDescriptorPools()[i], 1, &mTextureDescriptorSets[i]);
            mTextureDescriptorSets[i] = VK_NULL_HANDLE;
        }

        mBufferMemory.Release();
    }

} // namespace Surge
