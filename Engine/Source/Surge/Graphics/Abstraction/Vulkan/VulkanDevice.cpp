// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDevice.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"
#include <map>

namespace Surge
{
    VulkanDevice::VulkanDevice(VkInstance& instance)
    {
        Uint deviceCount = 0;
        VK_CALL(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));
        Vector<VkPhysicalDevice> physicalDevices(deviceCount);
        VK_CALL(vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data()));

        std::multimap<int, VkPhysicalDevice> candidates;
        for (const auto& device : physicalDevices)
        {
            int score = RatePhysicalDevice(device);
            candidates.insert(std::make_pair(score, device));
        }

        if (candidates.rbegin()->first > 0)
        {
            mPhysicalDevice = candidates.rbegin()->second;
            
            QueryPhysicalDeviceProperties();
            QueryPhysicalDeviceFeatures();

            VkPhysicalDeviceProperties physicalDeviceFeatures;
            vkGetPhysicalDeviceProperties(mPhysicalDevice, &physicalDeviceFeatures);
            DumpPhysicalDeviceProperties(physicalDeviceFeatures);

            Log<LogSeverity::Info>("Surge Device Score: {0}", candidates.rbegin()->first);
        }
        else
        {
            SG_ASSERT_INTERNAL("No discrete Graphics Processing Unit(GPU) found!");
        }


        QueryDeviceExtensions();

        /// Queue families ///
        Uint queueFamilyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, nullptr);
        mQueueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, mQueueFamilyProperties.data());

        Vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
        int requiredQueueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT;
        FillQueueFamilyIndicesAndStructures(requiredQueueFlags, mQueueFamilyIndices, queueCreateInfos);

        /// Logical Device ///
        Vector<const char*> deviceExtensions;
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        VkPhysicalDeviceFeatures enabledFeatures;
        memset(&enabledFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
        enabledFeatures.samplerAnisotropy = true;
        enabledFeatures.wideLines = true;
        enabledFeatures.fillModeNonSolid = true;
        enabledFeatures.pipelineStatisticsQuery = true;

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.pEnabledFeatures = nullptr;
        deviceCreateInfo.queueCreateInfoCount = static_cast<Uint>(queueCreateInfos.size());;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pNext = &mFeatures;

        if (!deviceExtensions.empty())
        {
            deviceCreateInfo.enabledExtensionCount = static_cast<Uint>(deviceExtensions.size());
            deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
        }
        VK_CALL(vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mLogicalDevice));
        volkLoadDevice(mLogicalDevice);
    }

    void VulkanDevice::Destroy()
    {
        vkDeviceWaitIdle(mLogicalDevice);
        vkDestroyDevice(mLogicalDevice, nullptr);
    }

    void VulkanDevice::QueryDeviceExtensions()
    {
        Uint extCount = 0;
        vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &extCount, nullptr);
        if (extCount > 0)
        {
            Vector<VkExtensionProperties> extensions(extCount);
            if (vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
            {
                Log<LogSeverity::Trace>("{0} has {1} extensions, they are:", mProperties.vk10Properties.properties.deviceName, extensions.size());
                int i = 1;
                for (const auto& ext : extensions)
                {
                    mSupportedExtensions.emplace(ext.extensionName);
                    Log<LogSeverity::Trace>("  {0} - {1}", i, ext.extensionName);
                    i++;
                }
            }
        }
    }

    void VulkanDevice::QueryPhysicalDeviceProperties()
    {
        // Credit to: https://github.com/rtryan98/Yggdrasil

        VkPhysicalDeviceProperties2 props{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        mProperties.vk10Properties.pNext = &mProperties.vk11Properties;
        mProperties.vk11Properties.pNext = &mProperties.vk12Properties;
        mProperties.vk12Properties.pNext = nullptr;
        vkGetPhysicalDeviceProperties2(mPhysicalDevice, &props);
    }

    void VulkanDevice::QueryPhysicalDeviceFeatures()
    {
        // Credit to: https://github.com/rtryan98/Yggdrasil

        mFeatures.vk10Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        mFeatures.vk10Features.pNext = &mFeatures.vk11Features;
        mFeatures.vk11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        mFeatures.vk11Features.pNext = &mFeatures.vk12Features;
        mFeatures.vk12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        mFeatures.vk12Features.pNext = &mFeatures.sync2Features;
        mFeatures.sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
        mFeatures.sync2Features.pNext = nullptr;

        VkPhysicalDeviceFeatures requestedVulkan10Features{};
        requestedVulkan10Features.textureCompressionBC = VK_TRUE;
        requestedVulkan10Features.samplerAnisotropy = VK_TRUE;
        requestedVulkan10Features.multiDrawIndirect = VK_TRUE;
        requestedVulkan10Features.imageCubeArray = VK_TRUE;
        requestedVulkan10Features.shaderInt16 = VK_TRUE;
        requestedVulkan10Features.shaderInt64 = VK_TRUE;

        VkPhysicalDeviceVulkan11Features requestedVulkan11Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
        requestedVulkan11Features.shaderDrawParameters = VK_TRUE;

        VkPhysicalDeviceVulkan12Features requestedVulkan12Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        requestedVulkan12Features.storageBuffer8BitAccess = VK_TRUE;
        requestedVulkan12Features.uniformAndStorageBuffer8BitAccess = VK_TRUE;
        requestedVulkan12Features.drawIndirectCount = VK_TRUE;
        requestedVulkan12Features.imagelessFramebuffer = VK_TRUE;
        requestedVulkan12Features.shaderInt8 = VK_TRUE;
        requestedVulkan12Features.descriptorIndexing = VK_TRUE;
        requestedVulkan12Features.shaderInputAttachmentArrayDynamicIndexing = VK_TRUE;
        requestedVulkan12Features.shaderUniformTexelBufferArrayDynamicIndexing = VK_TRUE;
        requestedVulkan12Features.shaderStorageTexelBufferArrayDynamicIndexing = VK_TRUE;
        requestedVulkan12Features.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
        requestedVulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        requestedVulkan12Features.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
        requestedVulkan12Features.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
        requestedVulkan12Features.shaderInputAttachmentArrayNonUniformIndexing = VK_TRUE;
        requestedVulkan12Features.shaderUniformTexelBufferArrayNonUniformIndexing = VK_TRUE;
        requestedVulkan12Features.shaderStorageTexelBufferArrayNonUniformIndexing = VK_TRUE;
        requestedVulkan12Features.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
        requestedVulkan12Features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
        requestedVulkan12Features.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
        requestedVulkan12Features.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
        requestedVulkan12Features.descriptorBindingUniformTexelBufferUpdateAfterBind = VK_TRUE;
        requestedVulkan12Features.descriptorBindingStorageTexelBufferUpdateAfterBind = VK_TRUE;
        requestedVulkan12Features.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
        requestedVulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
        requestedVulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;
        requestedVulkan12Features.runtimeDescriptorArray = VK_TRUE;

        VkPhysicalDeviceSynchronization2FeaturesKHR requestedSync2Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR };
        requestedSync2Features.synchronization2 = VK_TRUE;

        VkPhysicalDeviceMeshShaderFeaturesNV requestedMeshShaderFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV };
        requestedMeshShaderFeatures.meshShader = VK_TRUE;
        requestedMeshShaderFeatures.taskShader = VK_TRUE;

        VkPhysicalDeviceFeatures2 availableVulkan10Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        VkPhysicalDeviceVulkan11Features availableVulkan11Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
        VkPhysicalDeviceVulkan12Features availableVulkan12Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        VkPhysicalDeviceSynchronization2FeaturesKHR availableSync2Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR };
        VkPhysicalDeviceMeshShaderFeaturesNV availableMeshShaderFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV };

        availableVulkan10Features.pNext = &availableVulkan11Features;
        availableVulkan11Features.pNext = &availableVulkan12Features;
        availableVulkan12Features.pNext = &availableSync2Features;

        vkGetPhysicalDeviceFeatures2(mPhysicalDevice, &availableVulkan10Features);

        VkBool32* requested{ nullptr };
        VkBool32* available{ nullptr };

        requested = &requestedVulkan10Features.robustBufferAccess;
        available = &availableVulkan10Features.features.robustBufferAccess;
        for (uint32_t i{ 0 }; i < (sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32)); i++)
        {
            if (requested[i] && available[i])
            {
                VkBool32* feature{ (&mFeatures.vk10Features.features.robustBufferAccess) + i };
                *feature = VK_TRUE;
            }
        }

        requested = &requestedVulkan11Features.storageBuffer16BitAccess;
        available = &availableVulkan11Features.storageBuffer16BitAccess;
        for (uint32_t i{ 0 }; i < ((sizeof(VkPhysicalDeviceVulkan11Features) - sizeof(void*) - sizeof(VkStructureType)) / sizeof(VkBool32)) - 1; i++)
        {
            if (requested[i] && available[i])
            {
                VkBool32* feature{ (&mFeatures.vk11Features.storageBuffer16BitAccess) + i };
                *feature = VK_TRUE;
            }
        }

        requested = &requestedVulkan12Features.samplerMirrorClampToEdge;
        available = &availableVulkan12Features.samplerMirrorClampToEdge;
        for (uint32_t i{ 0 }; i < ((sizeof(VkPhysicalDeviceVulkan12Features) - sizeof(void*) - sizeof(VkStructureType)) / sizeof(VkBool32)) - 1; i++)
        {
            if (requested[i] && available[i])
            {
                VkBool32* feature{ (&mFeatures.vk12Features.samplerMirrorClampToEdge) + i };
                *feature = VK_TRUE;
            }
        }
        if (requestedSync2Features.synchronization2 && availableSync2Features.synchronization2)
        {
            mFeatures.sync2Features.synchronization2 = VK_TRUE;
        }

    }

    void VulkanDevice::DumpPhysicalDeviceProperties(VkPhysicalDeviceProperties physicalDeviceProperties)
    {
        Log<LogSeverity::Info>("Picked PhysicalDevice Properties:");
        Log<LogSeverity::Info>("  Device Name   : {0}", physicalDeviceProperties.deviceName);
        Log<LogSeverity::Info>("  Device ID     : {0}", physicalDeviceProperties.deviceID);
        Log<LogSeverity::Info>("  Driver Version: {0}", physicalDeviceProperties.driverVersion);
    }

    void VulkanDevice::FillQueueFamilyIndicesAndStructures(int flags, VulkanQueueFamilyIndices& outQueueFamilyIndices, Vector<VkDeviceQueueCreateInfo>& outQueueInfo)
    {
        // Find a dedicated queue for compute queue, which doesn't have graphics in it
        if (flags & VK_QUEUE_COMPUTE_BIT)
        {
            for (Uint i = 0; i < mQueueFamilyProperties.size(); i++)
            {
                VkQueueFamilyProperties& queueFamilyProperties = mQueueFamilyProperties[i];
                if ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
                {
                    outQueueFamilyIndices.ComputeQueue = i;
                    break;
                }
            }
        }

        // Find a dedicated queue for transfer queue, which doesn't have graphics/compute in it
        if (flags & VK_QUEUE_TRANSFER_BIT)
        {
            for (Uint i = 0; i < mQueueFamilyProperties.size(); i++)
            {
                auto& queueFamilyProperties = mQueueFamilyProperties[i];
                if ((queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT) && ((queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
                {
                    outQueueFamilyIndices.TransferQueue = i;
                    break;
                }
            }
        }

        // For other queue types or if no separate compute queue is present, return the first one to support the requested flags
        for (Uint i = 0; i < mQueueFamilyProperties.size(); i++)
        {
            if ((flags & VK_QUEUE_TRANSFER_BIT) && outQueueFamilyIndices.TransferQueue == -1)
            {
                if (mQueueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
                    outQueueFamilyIndices.TransferQueue = i;
            }

            if ((flags & VK_QUEUE_COMPUTE_BIT) && outQueueFamilyIndices.ComputeQueue == -1)
            {
                if (mQueueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                    outQueueFamilyIndices.ComputeQueue = i;
            }

            // Fill the Graphics queue
            if (flags & VK_QUEUE_GRAPHICS_BIT)
            {
                if (mQueueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    outQueueFamilyIndices.GraphicsQueue = i;
            }
        }

        const float defaultQueuePriority = 1.0f;

        // Graphics queue
        if (flags & VK_QUEUE_GRAPHICS_BIT)
        {
            VkDeviceQueueCreateInfo queueInfo{};
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.queueFamilyIndex = *mQueueFamilyIndices.GraphicsQueue;
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &defaultQueuePriority;
            outQueueInfo.push_back(queueInfo);
        }

        // Dedicated compute queue
        if (flags & VK_QUEUE_COMPUTE_BIT)
        {
            if (mQueueFamilyIndices.ComputeQueue != mQueueFamilyIndices.GraphicsQueue)
            {
                // If compute family index differs, we need an additional queue create info for the compute queue
                VkDeviceQueueCreateInfo queueInfo{};
                queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfo.queueFamilyIndex = *mQueueFamilyIndices.ComputeQueue;
                queueInfo.queueCount = 1;
                queueInfo.pQueuePriorities = &defaultQueuePriority;
                outQueueInfo.push_back(queueInfo);
            }
        }

        // Dedicated transfer queue
        if (flags & VK_QUEUE_TRANSFER_BIT)
        {
            if ((mQueueFamilyIndices.TransferQueue != mQueueFamilyIndices.GraphicsQueue) && (mQueueFamilyIndices.TransferQueue != mQueueFamilyIndices.ComputeQueue))
            {
                // If compute family index differs, we need an additional queue create info for the compute queue
                VkDeviceQueueCreateInfo queueInfo{};
                queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfo.queueFamilyIndex = *mQueueFamilyIndices.TransferQueue;
                queueInfo.queueCount = 1;
                queueInfo.pQueuePriorities = &defaultQueuePriority;
                outQueueInfo.push_back(queueInfo);
            }
        }
    }

    int VulkanDevice::RatePhysicalDevice(VkPhysicalDevice physicalDevice)
    {
        VkPhysicalDeviceFeatures features{};
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        vkGetPhysicalDeviceFeatures(physicalDevice, &features);
        int score = -50;

        if (!features.geometryShader)
            return 0;

        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            score += 250;
        else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            score += 100;
        else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
            score += 50;

        // Bonus
        score += properties.limits.maxColorAttachments;
        score += properties.limits.framebufferColorSampleCounts;
        score += properties.limits.framebufferDepthSampleCounts;
        score += properties.limits.maxClipDistances;
        score += properties.limits.maxBoundDescriptorSets;
        score += properties.limits.maxMemoryAllocationCount / 8;
        score += properties.limits.maxPushConstantsSize / 2;
        score += properties.limits.maxPerStageResources;

        return score;
    }
}
