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
    }

    VulkanGraphicsPipeline::VulkanGraphicsPipeline(const GraphicsPipelineSpecification& pipelineSpec)
        : mSpecification(pipelineSpec)
    {
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

        // TODO: Add vertex buffer layout

        ////////////////////////////////////////////////////////////////////////
        const ShaderReflectionData& reflectedData = mSpecification.Shader->GetReflectionData().at(ShaderType::VertexShader);
        const Vector<ShaderStageInput>& stageInputs = reflectedData.GetStageInputs();

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
        vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions.data();;
        ////////////////////////////////////////////////////////////////////////

        // Input Assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssembly.topology = Utils::GetVulkanPrimitiveTopology(pipelineSpec.Topology);
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        inputAssembly.flags = 0;

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

        // TODO: We wont probably use msaa, but it will be added later
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
        colorBlendAttachment.blendEnable = VK_FALSE;

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
        // TODO: Add descriptor set layouts and push constant
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        VK_CALL(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &mPipelineLayout));

        // Using dynamic pipeline states to avoid pipeline recreation when resizing
        std::array<VkDynamicState, 2> dynamicStates;
        dynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
        dynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;
        VkPipelineDynamicStateCreateInfo dynamicStatesCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamicStatesCreateInfo.dynamicStateCount = 2;
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
        vkDestroyPipeline(device, mPipeline, nullptr);
        vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);
    }

    void VulkanGraphicsPipeline::Bind()
    {
        VkCommandBuffer cmdBuf = VK_NULL_HANDLE; // TODO: We will need to add a render commandbuffer to the swap chain
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
    }

    void VulkanGraphicsPipeline::SetPushConstantData(void* data)
    {
        VkCommandBuffer cmdBuf = VK_NULL_HANDLE; // TODO: We will need to add a render commandbuffer to the swap chain
        vkCmdPushConstants(cmdBuf, mPipelineLayout,
            mPushConstantRangeCache.stageFlags,
            mPushConstantRangeCache.offset,
            mPushConstantRangeCache.size,
            data
        );
    }
}