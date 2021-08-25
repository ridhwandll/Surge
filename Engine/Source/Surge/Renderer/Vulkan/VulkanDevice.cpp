// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
#include "VulkanDevice.hpp"

#ifdef SURGE_WINDOWS
#include <vulkan/vulkan_win32.h>
#include "Surge/Platform/Windows/WindowsWindow.hpp"
#elif SURGE_APPLE
#include <vulkan/vulkan_metal.h>
#elif SURGE_LINUX
#include <vulkan/vulkan_wayland.h>
#endif

#include "Surge/Core/Core.hpp"

namespace Surge
{
    static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        switch (messageSeverity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            Surge::Log<Surge::LogSeverity::Error>(pCallbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            Surge::Log<Surge::LogSeverity::Warn>(pCallbackData->pMessage);
            break;
        }

        return VK_FALSE;
    }

    static VkResult CreateDebugMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    VulkanDevice::VulkanDevice(bool requestRaytracing)
    {
        CreateInstance();
        CreateSurface();
        PickPhysicalDevice();
        CreateDevice(requestRaytracing);
    }

    VulkanDevice::~VulkanDevice()
    {
        vkDestroyDevice(mDevice, nullptr);
        vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
#ifdef _DEBUG
        DestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
#endif
        vkDestroyInstance(mInstance, nullptr);
    }

    void VulkanDevice::CreateInstance()
    {
        if (volkInitialize() != VK_SUCCESS)
            Surge::Log<Surge::LogSeverity::Fatal>("Failed to initialise volk!");

        std::vector<const char*> instanceExtensions;
        std::vector<const char*> instanceLayers;

        instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef SURGE_WINDOWS
        instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif SURGE_APPLE
        instanceExtensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#elif SURGE_LINUX
        instanceExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif

#ifdef _DEBUG
        instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Surge Application";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Surge Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledLayerCount = static_cast<Uint>(instanceLayers.size());
        createInfo.ppEnabledLayerNames = instanceLayers.data();
        createInfo.enabledExtensionCount = static_cast<Uint>(instanceExtensions.size());
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();

#ifdef _DEBUG
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = VulkanDebugCallback;

        createInfo.pNext = &debugCreateInfo;
#endif

        if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS)
            Surge::Log<Surge::LogSeverity::Error>("Failed to create Vulkan instance!");

        volkLoadInstance(mInstance);
#ifdef _DEBUG
        if (CreateDebugMessenger(mInstance, &debugCreateInfo, nullptr, &mDebugMessenger) != VK_SUCCESS)
            Surge::Log<Surge::LogSeverity::Error>("Failed to create Vulkan debug messenger!");
#endif

    }

    void VulkanDevice::CreateSurface()
    {
        Scope<Window>& window = Surge::GetWindow();

#ifdef SURGE_WINDOWS
        WindowsWindow* win32 = reinterpret_cast<WindowsWindow*>(window.get());
        
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hinstance = GetModuleHandle(NULL);
        createInfo.hwnd = win32->GetHWND();

        PFN_vkCreateWin32SurfaceKHR func = reinterpret_cast<PFN_vkCreateWin32SurfaceKHR>(vkGetInstanceProcAddr(mInstance, "vkCreateWin32SurfaceKHR"));
        if (func(mInstance, &createInfo, nullptr, &mSurface) != VK_SUCCESS)
            Surge::Log<Surge::LogSeverity::Error>("Failed to create Vulkan window surface!");
#endif
    }

    void VulkanDevice::PickPhysicalDevice()
    {
        Uint deviceCount = 0;
        vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
        
        if (deviceCount == 0)
            Surge::Log<Surge::LogSeverity::Error>("No suitable vulkan physical devices found!");

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

        mPhysicalDevice = devices[0];

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                mGraphicsQueueIndex = i;
            }

            if (mGraphicsQueueIndex) {
                break;
            }

            i++;
        }
    }

    void VulkanDevice::CreateDevice(bool requestRaytracing)
    {
        std::vector<const char*> deviceExtensions;
        deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        if (requestRaytracing)
        {
            deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
            deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            deviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
            deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
            deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
            deviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
            deviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
        }

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = mGraphicsQueueIndex;
        queueCreateInfo.queueCount = 1;

        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.samplerAnisotropy = 1;
        deviceFeatures.fillModeNonSolid = 1; // wireframe
        deviceFeatures.pipelineStatisticsQuery = 1; // fetch pipeline stats directly

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = deviceExtensions.size();
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        const char* validationLayer = "VK_LAYER_KHRONOS_validation";

#ifdef _DEBUG
        createInfo.enabledLayerCount = 1;
        createInfo.ppEnabledLayerNames = &validationLayer;
#endif

        if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS)
            Surge::Log<Surge::LogSeverity::Error>("Failed to create Vulkan device:");

        volkLoadDevice(mDevice);
        
        vkGetDeviceQueue(mDevice, mGraphicsQueueIndex, 0, &mGraphicsQueue);
    }
}