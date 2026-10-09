// Copyright (c) - SurgeTechnologies - All rights reserved
#include "VulkanDevice.hpp"
#include "VulkanDebugger.hpp"
#include "Surge/Core/Logger/Logger.hpp"
#include "VulkanFramePacing.hpp"

namespace Surge
{
    static void OnVmaAllocate([[maybe_unused]] VmaAllocator allocator, [[maybe_unused]] Uint memoryType, [[maybe_unused]] VkDeviceMemory memory,VkDeviceSize size,void* userData)
    {
        GPUMemoryStats* stats = static_cast<GPUMemoryStats*>(userData);
        stats->AllocatedBytes.fetch_add(size, std::memory_order_relaxed);
        stats->AllocationCount.fetch_add(1, std::memory_order_relaxed);
    }

    static void OnVmaFree([[maybe_unused]] VmaAllocator allocator, [[maybe_unused]] Uint memoryType, [[maybe_unused]] VkDeviceMemory memory, VkDeviceSize size,void* userData)
    {
        GPUMemoryStats* stats = static_cast<GPUMemoryStats*>(userData);
        stats->AllocatedBytes.fetch_sub(size, std::memory_order_relaxed);
        stats->AllocationCount.fetch_sub(1, std::memory_order_relaxed);
    }

    void VulkanDevice::Initialize(VkInstance instance, VkSurfaceKHR surface)
    {
        Log<Severity::Info>("Initializing Vulkan Device");

        Uint gpuCount = 0;
        VK_CALL(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));

        if (gpuCount < 1)
            throw std::runtime_error("No physical device found.");
        else
            Log<Severity::Info>("Found {0} GPU(s)", gpuCount);

        Vector<VkPhysicalDevice> gpus(gpuCount);
        VK_CALL(vkEnumeratePhysicalDevices(instance, &gpuCount, gpus.data()));

        // Just grab the first GPU with a queue that supports graphics and presentation. A more robust implementation would rate each GPU and select the best one
        // TODO: Implement a more robust GPU selection algorithm that rates each GPU and selects the best one based on features, performance, etc.
        for (size_t i = 0; i < gpuCount && (mGraphicsQueueIndex < 0); i++)
        {
            mGPU = gpus[i];

            Uint queueFamilyCount;
            vkGetPhysicalDeviceQueueFamilyProperties(mGPU, &queueFamilyCount, nullptr);

            if (queueFamilyCount < 1)
            {
                throw std::runtime_error("No queue family found.");
            }

            Vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(mGPU, &queueFamilyCount, queueFamilyProperties.data());

            for (Uint i = 0; i < queueFamilyCount; i++)
            {
                VkBool32 supportsPresent;
                vkGetPhysicalDeviceSurfaceSupportKHR(mGPU, i, surface, &supportsPresent);

                // Find a queue family which supports graphics and presentation, prefer one that also supports compute
                // (Vulkan guarantees a graphics+compute family exists, in practice it is always the first graphics family)
                const VkQueueFlags flags = queueFamilyProperties[i].queueFlags;
                if ((flags & VK_QUEUE_GRAPHICS_BIT) && supportsPresent)
                {
                    const bool supportsCompute = (flags & VK_QUEUE_COMPUTE_BIT) != 0;
                    if (mGraphicsQueueIndex < 0 || (supportsCompute && !mSupportsCompute))
                    {
                        mGraphicsQueueIndex = i;
                        mSupportsCompute = supportsCompute;
                    }

                    if (mSupportsCompute)
                        break;
                }
            }
        }

        if (mGraphicsQueueIndex < 0)
            throw std::runtime_error("Did not find suitable device with a queue that supports graphics and presentation.");

        Uint deviceExtensionCount;
        VK_CALL(vkEnumerateDeviceExtensionProperties(mGPU, nullptr, &deviceExtensionCount, nullptr));
        Vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
        VK_CALL(vkEnumerateDeviceExtensionProperties(mGPU, nullptr, &deviceExtensionCount, deviceExtensions.data()));
            
        Vector<const char*> requiredDeviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        // Framepacing (Swappy on Android) asks for optional extensions such as VK_GOOGLE_display_timing
        Vector<String> framePacingExtensions = VulkanFramePacing::GetRequiredDeviceExtensions(mGPU, deviceExtensions);
        for(const String& ext : framePacingExtensions)
        {
            const bool alreadyRequested = std::any_of(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end(), [&](const char* e) { return ext == e; });
            if(!alreadyRequested)
                requiredDeviceExtensions.push_back(ext.c_str());
        }

        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        // Uses a single graphics queue for now, but a more robust implementation would use separate queues for graphics, compute, and transfer operations if available
        const float queuePriority = 0.5f;
        VkDeviceQueueCreateInfo queueInfo {};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.pNext = nullptr;
        queueInfo.queueFamilyIndex = static_cast<uint32_t>(mGraphicsQueueIndex);
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;

        // Features
        VkPhysicalDeviceFeatures enabledFeatures {};
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(mGPU, &deviceFeatures);
        if(!deviceFeatures.samplerAnisotropy)
            Log<Severity::Warn>("VkPhysicalDeviceFeatures::samplerAnisotropy is unavailable!");
        else
            enabledFeatures.samplerAnisotropy = VK_TRUE;

        if(!deviceFeatures.wideLines)
            Log<Severity::Warn>("VkPhysicalDeviceFeatures::wideLines is unavailable!");
        else
            enabledFeatures.wideLines = VK_TRUE;

        // Block compressed textures, sampling ASTC/ETC2/BC images is invalid unless the matching feature is enabled
        mSupportsASTC = deviceFeatures.textureCompressionASTC_LDR == VK_TRUE;
        mSupportsETC2 = deviceFeatures.textureCompressionETC2 == VK_TRUE;
        mSupportsBC = deviceFeatures.textureCompressionBC == VK_TRUE;
        enabledFeatures.textureCompressionASTC_LDR = deviceFeatures.textureCompressionASTC_LDR;
        enabledFeatures.textureCompressionETC2 = deviceFeatures.textureCompressionETC2;
        enabledFeatures.textureCompressionBC = deviceFeatures.textureCompressionBC;
        Log<Severity::Info>("Texture compression support: ASTC: {}, ETC2: {}, BC: {}", mSupportsASTC, mSupportsETC2, mSupportsBC);

        deviceInfo.pEnabledFeatures = &enabledFeatures;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &queueInfo;
        deviceInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions.size());
        deviceInfo.ppEnabledExtensionNames = requiredDeviceExtensions.data();
        VK_CALL(vkCreateDevice(mGPU, &deviceInfo, nullptr, &mDevice));
        volkLoadDevice(mDevice);

        // Get the graphics queue
        // TODO: Move queue to a separate file and support multiple queues (graphics, compute, transfer)
        vkGetDeviceQueue(mDevice, mGraphicsQueueIndex, 0, &mQueue);
        VulkanFramePacing::OnDeviceCreated(mDevice, mQueue, static_cast<Uint>(mGraphicsQueueIndex));

        //Vulkan Memory Alloctor (VMA)
        VmaDeviceMemoryCallbacks memCallbacks = {};
        memCallbacks.pfnAllocate = OnVmaAllocate;
        memCallbacks.pfnFree = OnVmaFree;
        memCallbacks.pUserData = &mGPUMemoryStats;

        VmaVulkanFunctions vmaVulkanFunc = {};
        vmaVulkanFunc.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vmaVulkanFunc.vkGetDeviceProcAddr = vkGetDeviceProcAddr;

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_1;
        allocatorInfo.physicalDevice = mGPU;
        allocatorInfo.device = mDevice;
        allocatorInfo.instance = instance;
        allocatorInfo.pVulkanFunctions = &vmaVulkanFunc;
        allocatorInfo.pDeviceMemoryCallbacks = &memCallbacks;

        VK_CALL(vmaCreateAllocator(&allocatorInfo, &mVmaAllocator));

        QueryBudget();
    }

    void VulkanDevice::Destroy()
    {
        vkDeviceWaitIdle(mDevice);
        VulkanFramePacing::OnDeviceDestroyed(mDevice);
        vmaDestroyAllocator(mVmaAllocator);
        vkDestroyDevice(mDevice, nullptr);
    }
    void VulkanDevice::QueryBudget()
    {
        vkGetPhysicalDeviceMemoryProperties(mGPU, &mMemProps);
        std::array<VmaBudget, VK_MAX_MEMORY_HEAPS> budgets = {};
        vmaGetHeapBudgets(mVmaAllocator, budgets.data());

        const auto& props = mMemProps;
        for (Uint i = 0; i < props.memoryHeapCount; i++)
        {
            bool isDeviceLocal = props.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;

            // On UMA mobile memoryHeapCount may be 1 with no DEVICE_LOCAL
            if (isDeviceLocal || props.memoryHeapCount == 1)
                mGPUMemoryStats.BudgetBytes += budgets[i].budget;
        }
    }
}
