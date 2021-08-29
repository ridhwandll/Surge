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
            vkGetPhysicalDeviceProperties(mPhysicalDevice, &mProperties);
            vkGetPhysicalDeviceFeatures(mPhysicalDevice, &mFeatures);
            DumpPhysicalDeviceProperties(mProperties);
            Log<LogSeverity::Info>("Surge Device Score: {0}", candidates.rbegin()->first);
        }
        else
        {
            // TODO: Assert
            Log<LogSeverity::Error>("No discrete Graphics Processing Unit(GPU) found!");
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
        deviceCreateInfo.pEnabledFeatures = &enabledFeatures;
        deviceCreateInfo.queueCreateInfoCount = static_cast<Uint>(queueCreateInfos.size());;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        deviceCreateInfo.pNext = nullptr;

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
                Log<LogSeverity::Trace>("{0} has {1} extensions, they are:", mProperties.deviceName, extensions.size());
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
