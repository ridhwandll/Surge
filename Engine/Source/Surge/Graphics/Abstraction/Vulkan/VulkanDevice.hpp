// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include <volk.h>
#include <unordered_set>
#include <optional>

namespace Surge
{
    struct VulkanQueueFamilyIndices
    {
        int32_t GraphicsQueue = -1;
        int32_t ComputeQueue = -1;
        int32_t TransferQueue = -1;
    };

    enum class VulkanQueueType
    {
        Graphics = 0,
        Compute,
        Transfer
    };

    class VulkanDevice
    {
    public:
        VulkanDevice() = default;
        ~VulkanDevice() = default;

        void Initialize(VkInstance instance);
        void Destroy();

        VkPhysicalDevice GetPhysicalDevice() { return mPhysicalDevice; }
        VkDevice GetLogicalDevice() { return mLogicalDevice; }
        VulkanQueueFamilyIndices GetQueueFamilyIndices() { return mQueueFamilyIndices; }

        void BeginOneTimeCmdBuffer(VkCommandBuffer& commandBuffer, VulkanQueueType type);
        void EndOneTimeCmdBuffer(VkCommandBuffer commandBuffer, VulkanQueueType type);

        VkQueue GetGraphicsQueue() { return mGraphicsQueue; }
        VkQueue GetComputeQueue() { return mComputeQueue; }
        VkQueue GetTransferQueue() { return mTransferQueue; }

        VkCommandPool GetGraphicsCommandPool() { return mGraphicsCommandPool; }
        VkCommandPool GetComputeCommandPool() { return mComputeCommandPool; }
        VkCommandPool GetTransferCommandPool() { return mTransferCommandPool; }

        bool IsExtensionSupported(const String& extensionName) { return mSupportedExtensions.find(extensionName) != mSupportedExtensions.end(); };
    private:
        void QueryDeviceExtensions();
        void QueryPhysicalDeviceFeatures();
        void QueryPhysicalDeviceProperties();
        void DumpPhysicalDeviceProperties();
        void FillQueueFamilyIndicesAndStructures(int flags, VulkanQueueFamilyIndices& outQueueFamilyIndices, Vector<VkDeviceQueueCreateInfo>& outQueueInfo);
        void CreateCommandPools();
        int32_t RatePhysicalDevice(VkPhysicalDevice physicalDevice);
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

        VkQueue mGraphicsQueue;
        VkQueue mComputeQueue;
        VkQueue mTransferQueue;

        VkCommandPool mGraphicsCommandPool;
        VkCommandPool mComputeCommandPool;
        VkCommandPool mTransferCommandPool;
    };
}
