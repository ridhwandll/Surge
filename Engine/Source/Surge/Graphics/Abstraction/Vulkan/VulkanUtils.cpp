// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanUtils.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"

namespace Surge
{
    ShaderType VulkanUtils::ShaderTypeFromString(const String& type)
    {
        if (type == "Vertex]")  return ShaderType::Vertex;
        if (type == "Pixel]")   return ShaderType::Pixel;
        if (type == "Compute]") return ShaderType::Compute;

        return ShaderType::None;
    }

    String VulkanUtils::ShaderTypeToString(const ShaderType& type)
    {
        switch (type)
        {
        case ShaderType::Vertex:  return "Vertex";
        case ShaderType::Pixel:   return "Pixel";
        case ShaderType::Compute: return "Compute";
        case ShaderType::None: SG_ASSERT_INTERNAL("ShaderType::None is invalid in this case!");
        }
        return "";
    }

    shaderc_shader_kind VulkanUtils::ShadercShaderKindFromSurgeShaderType(const ShaderType& type)
    {
        switch (type)
        {
        case ShaderType::Vertex:  return shaderc_glsl_vertex_shader;
        case ShaderType::Pixel:   return shaderc_glsl_fragment_shader;
        case ShaderType::Compute: return shaderc_glsl_compute_shader;
        case ShaderType::None: SG_ASSERT_INTERNAL("ShaderType::None is invalid in this case!");
        }

        return static_cast<shaderc_shader_kind>(-1);
    }

    VkPrimitiveTopology VulkanUtils::GetVulkanPrimitiveTopology(PrimitiveTopology primitive)
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

    VkFormat VulkanUtils::ShaderDataTypeToVulkanFormat(ShaderDataType type)
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

    Vector<VkDescriptorSetLayout> VulkanUtils::GetDescriptorSetLayoutVectorFromHashMap(const HashMap<Uint, VkDescriptorSetLayout>& descriptorSetLayouts)
    {
        Vector<VkDescriptorSetLayout> descriptorSetLayout;
        for (auto& layout : descriptorSetLayouts)
            descriptorSetLayout.push_back(layout.second);
        return descriptorSetLayout;
    }

    Vector<VkPushConstantRange> VulkanUtils::GetPushConstantRangesVectorFromHashMap(const HashMap<String, VkPushConstantRange>& pushConstants)
    {
        Vector<VkPushConstantRange> pushConstantsVector;
        for (auto& pushConstant : pushConstants)
            pushConstantsVector.push_back(pushConstant.second);
        return pushConstantsVector;
    }

    VkDescriptorType VulkanUtils::ShaderBufferTypeToVulkan(ShaderBuffer::Usage type)
    {
        switch (type)
        {
        case ShaderBuffer::Usage::Storage: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case ShaderBuffer::Usage::Uniform: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }
        SG_ASSERT(false, "ShaderBuffer::Usage is invalid");
        return VkDescriptorType();
    }

    VkDescriptorType VulkanUtils::ShaderImageTypeToVulkan(ShaderResource::Usage type)
    {
        switch (type)
        {
        case ShaderResource::Usage::Sampled: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case ShaderResource::Usage::Storage: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        }
        SG_ASSERT(false, "ShaderResource::Usage is invalid");
        return VkDescriptorType();
    }

    VkShaderStageFlags VulkanUtils::GetShaderStagesFlagsFromShaderTypes(const Vector<ShaderType>& shaderStages)
    {
        VkShaderStageFlags stageFlags{};
        for (const ShaderType& stage : shaderStages)
        {
            //None = 0, VertexShader, PixelShader, ComputeShader
            switch (stage)
            {
            case ShaderType::Vertex:  stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;   break;
            case ShaderType::Pixel:   stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT; break;
            case ShaderType::Compute: stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;  break;
            case ShaderType::None: SG_ASSERT(false, "Shader::None is invalid!");
            }
        }
        return stageFlags;
    }

    void VulkanUtils::CreateWindowSurface(VkInstance instance, Window* windowHandle, VkSurfaceKHR* surface)
    {
    #ifdef SURGE_WINDOWS
        PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;

        // Getting the vkCreateWin32SurfaceKHR function pointer and assert if it doesnt exist
        vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
        if (!vkCreateWin32SurfaceKHR)
            SG_ASSERT_INTERNAL("[Win32] Vulkan instance missing VK_KHR_win32_surface extension");

        VkWin32SurfaceCreateInfoKHR sci;
        memset(&sci, 0, sizeof(sci)); // Clear the info
        sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        sci.hinstance = GetModuleHandle(nullptr);
        sci.hwnd = static_cast<HWND>(windowHandle->GetNativeWindowHandle());

        VK_CALL(vkCreateWin32SurfaceKHR(instance, &sci, nullptr, surface));
    #else
        SG_ASSERT_INTERNAL("Surge is currently Windows Only! :(");
    #endif
    }
}

