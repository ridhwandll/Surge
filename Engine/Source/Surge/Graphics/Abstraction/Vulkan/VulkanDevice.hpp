// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include <volk.h>
#include <unordered_set>
#include <optional>

namespace Surge
{
    struct VulkanQueueFamilyIndices
    {
        std::optional<int32_t> GraphicsQueue;
        std::optional<int32_t> ComputeQueue;
        std::optional<int32_t> TransferQueue;
    };

    class SURGE_API VulkanDevice
    {
    public:
        VulkanDevice() = default;
        VulkanDevice(VkInstance& instance);
        ~VulkanDevice() = default;

        VkDevice GetLogicaldevice() { return mLogicalDevice; }
        VkPhysicalDevice GetSelectedPhysicalDevice() { return mPhysicalDevice; }
        bool IsExtensionSupported(const String& extensionName) { return mSupportedExtensions.find(extensionName) != mSupportedExtensions.end(); };
        void Destroy();
    private:
        void QueryDeviceExtensions();
        void DumpPhysicalDeviceProperties(VkPhysicalDeviceProperties physicalDeviceProperties);
        void FillQueueFamilyIndicesAndStructures(int flags, VulkanQueueFamilyIndices& outQueueFamilyIndices, Vector<VkDeviceQueueCreateInfo>& outQueueInfo);
        int RatePhysicalDevice(VkPhysicalDevice physicalDevice);
    private:
        VulkanQueueFamilyIndices mQueueFamilyIndices;
        Vector<VkQueueFamilyProperties> mQueueFamilyProperties;

        // PhysicalDevice
        VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties mProperties{};
        VkPhysicalDeviceFeatures mFeatures{};

        // LogicalDevice
        VkDevice mLogicalDevice;

        std::unordered_set<String> mSupportedExtensions;
    };
}
