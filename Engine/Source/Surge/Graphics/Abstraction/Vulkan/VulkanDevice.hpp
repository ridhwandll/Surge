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
        const VulkanQueueFamilyIndices& GetQueueFamilyIndices() const { return mQueueFamilyIndices;  }
        bool IsExtensionSupported(const String& extensionName) { return mSupportedExtensions.find(extensionName) != mSupportedExtensions.end(); };
        void Destroy();
    private:
        void QueryDeviceExtensions();
        void QueryPhysicalDeviceFeatures();
        void QueryPhysicalDeviceProperties();
        void DumpPhysicalDeviceProperties(VkPhysicalDeviceProperties physicalDeviceProperties);
        void FillQueueFamilyIndicesAndStructures(int flags, VulkanQueueFamilyIndices& outQueueFamilyIndices, Vector<VkDeviceQueueCreateInfo>& outQueueInfo);
        int RatePhysicalDevice(VkPhysicalDevice physicalDevice);
    private:
        VulkanQueueFamilyIndices mQueueFamilyIndices;
        Vector<VkQueueFamilyProperties> mQueueFamilyProperties;

        // PhysicalDevice
        VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;

        // LogicalDevice
        VkDevice mLogicalDevice;

        std::unordered_set<String> mSupportedExtensions;

        struct VkFeatures
        {
            VkPhysicalDeviceFeatures2 vk10Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
            VkPhysicalDeviceVulkan11Features vk11Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
            VkPhysicalDeviceVulkan12Features vk12Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
            VkPhysicalDeviceSynchronization2FeaturesKHR sync2Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR };
        } mFeatures{};

        struct VkProperties
        {
            VkPhysicalDeviceProperties2 vk10Properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
            VkPhysicalDeviceVulkan11Properties vk11Properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES };
            VkPhysicalDeviceVulkan12Properties vk12Properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES };
        } mProperties{};
    };
}
