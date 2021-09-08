// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "VulkanGraphicsPipeline.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanShader.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanSwapChain.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include "Surge/Graphics/ReflectionData.hpp"
#include <array>

namespace Surge
{
    namespace Utils
    {
        VkPrimitiveTopology GetVulkanPrimitiveTopology(PrimitiveTopology primitive)
        {
            switch (primitive)
            {
            case PrimitiveTopology::Points:         return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            case PrimitiveTopology::Lines:          return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case PrimitiveTopology::LineStrip:      return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            case PrimitiveTopology::Triangles:      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case PrimitiveTopology::TriangleStrip:  return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case PrimitiveTopology::None:           SG_ASSERT_INTERNAL("PrimitiveType::None is invalid!");
            }

            SG_ASSERT_INTERNAL("No Surge::PrimitiveType maps to VkPrimitiveTopology!");
            return VkPrimitiveTopology();
        }

        VkFormat ShaderDataTypeToVulkanFormat(ShaderDataType type)
        {
            switch (type)
            {
            case ShaderDataType::Float:     return VK_FORMAT_R32_SFLOAT;
            case ShaderDataType::Float2:    return VK_FORMAT_R32G32_SFLOAT;
            case ShaderDataType::Float3:    return VK_FORMAT_R32G32B32_SFLOAT;
            case ShaderDataType::Float4:    return VK_FORMAT_R32G32B32A32_SFLOAT;
            default: SG_ASSERT_INTERNAL("Undefined!");
            }

            SG_ASSERT_INTERNAL("No Surge::ShaderDataType maps to VkFormat!");
            return VK_FORMAT_UNDEFINED;
        }

        Vector<VkDescriptorSetLayout> GetDescriptorSetLayoutVectorFromHashMap(const HashMap<Uint, VkDescriptorSetLayout>& descriptorSetLayouts)
        {
            Vector<VkDescriptorSetLayout> descriptorSetLayout;
            for (auto layout : descriptorSetLayouts)
                descriptorSetLayout.push_back(layout.second);
            return descriptorSetLayout;
        }

        Vector<VkPushConstantRange> GetPushConstantRangesVectorFromHashMap(const HashMap<String, VkPushConstantRange>& pushConstants)
        {
            Vector<VkPushConstantRange> pushConstantsVector;
            for (auto pushConstant : pushConstants)
                pushConstantsVector.push_back(pushConstant.second);
            return pushConstantsVector;
        }

        VkDescriptorType ShaderBufferTypeToVulkan(ShaderBuffer::Usage type)
        {
            switch (type)
            {
            case ShaderBuffer::Usage::Storage: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case ShaderBuffer::Usage::Uniform: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            }
            SG_ASSERT(false, "ShaderBuffer::Usage is invalid");
            return VkDescriptorType();
        }

        VkDescriptorType ShaderImageTypeToVulkan(ShaderResource::Usage type)
        {
            switch (type)
            {
            case ShaderResource::Usage::Sampled: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case ShaderResource::Usage::Storage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            }
            SG_ASSERT(false, "ShaderResource::Usage is invalid");
            return VkDescriptorType();
        }

        VkShaderStageFlags GetShaderStagesFlagsFromShaderTypes(const Vector<ShaderType>& shaderStages)
        {
            VkShaderStageFlags stageFlags{};
            for (auto stage : shaderStages)
            {
                //None = 0, VertexShader, PixelShader, ComputeShader
                switch (stage)
                {
                case ShaderType::VertexShader:  stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;   break;
                case ShaderType::PixelShader:   stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT; break;
                case ShaderType::ComputeShader: stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;  break;
                case ShaderType::None: SG_ASSERT(false, "Shader::None is invalid!");
                }
            }
            return stageFlags;
        }
    }

    VulkanGraphicsPipeline::VulkanGraphicsPipeline(const GraphicsPipelineSpecification& pipelineSpec)
        : mSpecification(pipelineSpec)
    {
        SCOPED_TIMER("Pipeline Creation");
        VkDevice device = static_cast<VulkanDevice*>(GetRenderContext()->GetInteralDevice())->GetLogicaldevice();

        // Setting up all the shaders into a create info class
        HashMap<ShaderType, VkShaderModule> shaderModules = pipelineSpec.Shader.As<VulkanShader>()->GetVulkanShaderModules();
        Vector<VkPipelineShaderStageCreateInfo> shaderStages;
        for (const auto& shader : shaderModules)
        {
            VkPipelineShaderStageCreateInfo& shaderStageInfo = shaderStages.emplace_back();
            shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageInfo.stage = VulkanShader::GetVulkanShaderStage(shader.first);
            shaderStageInfo.module = shader.second;
            shaderStageInfo.pName = "main";
            shaderStageInfo.pSpecializationInfo = nullptr;
        }

        const ShaderReflectionData& reflectedData = mSpecification.Shader->GetReflectionData();

        // We only need the stage input of vertex shader to generate the input layout
        const Vector<ShaderStageInput>& stageInputs = reflectedData.GetStageInputs().at(ShaderType::VertexShader);

        Uint stride = 0;
        for (const ShaderStageInput& stageInput : stageInputs)
            stride += stageInput.Size;

        VkVertexInputBindingDescription vertexBindingDescriptions;
        vertexBindingDescriptions.binding = 0;
        vertexBindingDescriptions.stride = stride;
        vertexBindingDescriptions.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        Vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions(stageInputs.size());
        for (Uint i = 0; i < stageInputs.size(); i++)
        {
            const ShaderStageInput& stageInput = stageInputs[i];
            vertexAttributeDescriptions[i].binding = 0;
            vertexAttributeDescriptions[i].location = stageInput.Location;
            vertexAttributeDescriptions[i].format = Utils::ShaderDataTypeToVulkanFormat(stageInput.DataType);
            vertexAttributeDescriptions[i].offset = stageInput.Offset;
        }

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<Uint>(vertexAttributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescriptions;
        vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();

        // Input Assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssembly.topology = Utils::GetVulkanPrimitiveTopology(pipelineSpec.Topology);
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        inputAssembly.flags = 0;
        inputAssembly.pNext = nullptr;

        // Setting the viewport and scissors to nullptr because later we will use dynamic pipeline states that avoids pipeline recreation on window resize
        VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportState.viewportCount = 1;
        viewportState.pViewports = nullptr;
        viewportState.scissorCount = 1;
        viewportState.pScissors = nullptr;

        // Rasterizer stage
        VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = pipelineSpec.LineWidth;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        // TODO: We wont probably use MSAA, but it will be added later
        VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // Depth state
        VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthStencil.depthTestEnable = pipelineSpec.UseDepth;
        depthStencil.depthWriteEnable = pipelineSpec.UseDepth;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = pipelineSpec.UseStencil;

        // Blending
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE; //TODO: Enable blending

        VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        // Setting up the pipeline layout
        CreateVulkanDescriptorSetLayouts();
        CreatePushConstantRanges();
        Vector<VkDescriptorSetLayout> descriptorSetLayouts = Utils::GetDescriptorSetLayoutVectorFromHashMap(mDescriptorSetLayouts);
        Vector<VkPushConstantRange> pushConstants = Utils::GetPushConstantRangesVectorFromHashMap(mPushConstants);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = pushConstants.size();
        pipelineLayoutInfo.pPushConstantRanges = pushConstants.data();
        VK_CALL(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &mPipelineLayout));

        // Using dynamic pipeline states to avoid pipeline recreation when resizing
        std::array<VkDynamicState, 2> dynamicStates;
        dynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
        dynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;
        VkPipelineDynamicStateCreateInfo dynamicStatesCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamicStatesCreateInfo.dynamicStateCount = dynamicStates.size();
        dynamicStatesCreateInfo.pDynamicStates = dynamicStates.data();

        VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.stageCount = static_cast<Uint>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicStatesCreateInfo;
        pipelineInfo.layout = mPipelineLayout;

        // TODO(AC3R): This is temporary becase we dont have renderpass absctraction yet
        pipelineInfo.renderPass = static_cast<VulkanSwapChain*>(GetRenderContext()->GetSwapChain())->GetVulkanRenderPass();
        pipelineInfo.subpass = 0;

        VK_CALL(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline));
    }

    VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
    {
        VkDevice device = static_cast<VulkanDevice*>(GetRenderContext()->GetInteralDevice())->GetLogicaldevice();

        for (auto& descriptorSetLayout : mDescriptorSetLayouts)
            vkDestroyDescriptorSetLayout(device, descriptorSetLayout.second, nullptr);

        vkDestroyPipeline(device, mPipeline, nullptr);
        vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);
    }

    void VulkanGraphicsPipeline::Bind()
    {
        VkCommandBuffer cmdBuf = VK_NULL_HANDLE; // TODO: We will need to add a render commandbuffer to the swap chain
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
    }

    void VulkanGraphicsPipeline::SetPushConstantData(const String& bufferName, void* data)
    {
        VkCommandBuffer cmdBuf = VK_NULL_HANDLE; // TODO: We will need to add a render commandbuffer to the swap chain
        vkCmdPushConstants(cmdBuf, mPipelineLayout,
            mPushConstants[bufferName].stageFlags,
            mPushConstants[bufferName].offset,
            mPushConstants[bufferName].size,
            data
        );
    }

    void VulkanGraphicsPipeline::CreateVulkanDescriptorSetLayouts()
    {
        VkDevice device = static_cast<VulkanDevice*>(GetRenderContext()->GetInteralDevice())->GetLogicaldevice();
        ShaderReflectionData reflectedData = mSpecification.Shader->GetReflectionData();

        // Iterate through all the sets and creating the layouts
        // (descriptor layouts use HashMap<Uint, VkDescriptorSetLayout> because the Uint specifies at which set number the layout is going to be used
        for (auto& descriptorSet : reflectedData.GetDescriptorSetCount())
        {
            Vector<VkDescriptorSetLayoutBinding> layoutBindings;
            for (const ShaderBuffer& buffer : reflectedData.GetBuffers())
            {
                if (buffer.Set != descriptorSet) continue;;

                VkDescriptorSetLayoutBinding& LayoutBinding = layoutBindings.emplace_back();
                LayoutBinding.binding = buffer.Binding;
                LayoutBinding.descriptorCount = 1; // TODO: Need to add arrays
                LayoutBinding.descriptorType = Utils::ShaderBufferTypeToVulkan(buffer.Type);
                LayoutBinding.stageFlags = Utils::GetShaderStagesFlagsFromShaderTypes(buffer.ShaderStages);
            }

            for (const ShaderResource& texture : reflectedData.GetResources())
            {
                if (texture.Set != descriptorSet) continue;;

                VkDescriptorSetLayoutBinding& LayoutBinding = layoutBindings.emplace_back();
                LayoutBinding.binding = texture.Binding;
                LayoutBinding.descriptorCount = 1; // TODO: Need to add arrays
                LayoutBinding.descriptorType = Utils::ShaderImageTypeToVulkan(texture.Type);
                LayoutBinding.stageFlags = Utils::GetShaderStagesFlagsFromShaderTypes(texture.ShaderStages);
            }

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.flags = 0;
            layoutInfo.bindingCount = static_cast<Uint>(layoutBindings.size());
            layoutInfo.pBindings = layoutBindings.data();

            VK_CALL(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &mDescriptorSetLayouts[descriptorSet]));
        }
    }
    
    void VulkanGraphicsPipeline::CreatePushConstantRanges()
    {
        ShaderReflectionData reflectionData = mSpecification.Shader->GetReflectionData();
        for (auto& pushConstant : reflectionData.GetPushConstantBuffers())
        {
            VkPushConstantRange& pushConstantRange = mPushConstants[pushConstant.BufferName];
            pushConstantRange.offset = 0;
            pushConstantRange.size = pushConstant.Size;
            pushConstantRange.stageFlags = Utils::GetShaderStagesFlagsFromShaderTypes(pushConstant.ShaderStages);
        }
    }
}
