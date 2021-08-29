// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"

namespace Surge
{
    static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
    {
        switch (messageSeverity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            Log<LogSeverity::Warn>("[VulkanDiagnostics] {0}", pCallbackData->pMessage);
            Log<LogSeverity::Debug>("------------------------------------------------");
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            Log<LogSeverity::Error>("[VulkanDiagnostics] {0}", pCallbackData->pMessage);
            Log<LogSeverity::Debug>("------------------------------------------------");
            break;
        }

        return VK_FALSE;
    }

    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        else
            return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr)
            func(instance, debugMessenger, pAllocator);
    }

    void VulkanDiagnostics::Create(VkInstanceCreateInfo& vkInstanceCreateInfo)
    {
        PopulateDebugCreateInfo(mDebugCreateInfo);
        vkInstanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&mDebugCreateInfo;
    }

    void VulkanDiagnostics::AddValidationLayers(Vector<const char*>& outInstanceLayers)
    {
        const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
        uint32_t instanceLayerCount;
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
        Vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
        vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());

        bool validationLayerPresent = false;
        Log<LogSeverity::Trace>("{0} Vulkan Instance Layers:", instanceLayerCount);
        for (const VkLayerProperties& layer : instanceLayerProperties)
        {
            Log<LogSeverity::Trace>("  {0}", layer.layerName);
            if (strcmp(layer.layerName, validationLayerName) == 0)
            {
                validationLayerPresent = true;
                outInstanceLayers.push_back(validationLayerName);
            }
        }

        if (!validationLayerPresent)
            Log<LogSeverity::Error>("Validation layer {0} not present, validation is disabled", validationLayerName);
    }

    void VulkanDiagnostics::AddValidationExtensions(Vector<const char*>& outInstanceExtensions)
    {
        outInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    void VulkanDiagnostics::StartDiagnostics(VkInstance& instance)
    {
        VK_CALL(CreateDebugUtilsMessengerEXT(instance, &mDebugCreateInfo, nullptr, &mDebugMessenger))
    }

    void VulkanDiagnostics::EndDiagnostics(VkInstance& instance)
    {
        DestroyDebugUtilsMessengerEXT(instance, mDebugMessenger, nullptr);
    }

    void VulkanDiagnostics::PopulateDebugCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& debugCreateInfo)
    {
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = VulkanDebugCallback;
        debugCreateInfo.pUserData = nullptr;
    }
}
