// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

#include "Surge/Renderer/Device.h"

#include <volk.h>

namespace Surge
{
    class VulkanDevice : public Device
    {
    public:
        VulkanDevice(bool requestRaytracing);
        ~VulkanDevice();

        VkInstance& GetInstance() { return mInstance; }
        VkSurfaceKHR& GetSurface() { return mSurface; }
        VkPhysicalDevice& GetPhysicalDevice() { return mPhysicalDevice; }
        VkQueue& GetGraphicsQueue() { return mGraphicsQueue; }
        VkDevice& GetDevice() { return mDevice; }
    private:
        VkInstance mInstance;
        VkSurfaceKHR mSurface;
        VkPhysicalDevice mPhysicalDevice;
        VkQueue mGraphicsQueue;
        VkDevice mDevice;
        VkDebugUtilsMessengerEXT mDebugMessenger;

        Uint mGraphicsQueueIndex;

        void CreateInstance();
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateDevice(bool requestRaytracing);
    };
}