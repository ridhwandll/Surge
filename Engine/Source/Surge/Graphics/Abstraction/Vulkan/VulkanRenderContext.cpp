// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanDiagnostics.hpp"

namespace Surge
{
#ifdef SURGE_DEBUG
    static bool sValidation = true;
#else
    static bool sValidation = false;
#endif // SURGE_DEBUG

    void VulkanRenderContext::Initialize(Window* window)
    {
        VK_CALL(volkInitialize());

        /// VkApplicationInfo ///
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "SurgeProtector";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Surge Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2; // TODO(Rid): Check which version is available, use 1.1 if necessary

        /// VkInstanceCreateInfo ///
        Vector<const char*> instanceExtensions = GetRequiredInstanceExtensions();
        Vector<const char*> instanceLayers = GetRequiredInstanceLayers();
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledLayerCount = static_cast<Uint>(instanceLayers.size());
        createInfo.ppEnabledLayerNames = instanceLayers.data();
        createInfo.enabledExtensionCount = static_cast<Uint>(instanceExtensions.size());
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();

        VK_CALL(vkCreateInstance(&createInfo, nullptr, &mPrivateData.VulkanInstance));
        volkLoadInstance(mPrivateData.VulkanInstance);
    }

    void VulkanRenderContext::Shutdown()
    {

    }

    Vector<const char*> VulkanRenderContext::GetRequiredInstanceExtensions()
    {
        Vector<const char*> instanceExtensions;
        instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

#ifdef SURGE_WINDOWS
        instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif SURGE_APPLE
        instanceExtensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
#elif SURGE_LINUX
        instanceExtensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif

        if (sValidation)
        {
            instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
            instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }

        return instanceExtensions;
    }

    Vector<const char*> VulkanRenderContext::GetRequiredInstanceLayers()
    {
        Vector<const char*> instanceLayers;
        if (sValidation)
        {
            const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

            uint32_t instanceLayerCount;
            vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
            std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
            vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());

            bool validationLayerPresent = false;
            Log<LogSeverity::Trace>("{0} Vulkan Instance Layers:", instanceLayerCount);
            for (const VkLayerProperties& layer : instanceLayerProperties)
            {
                Log<LogSeverity::Trace>("  {0}", layer.layerName);
                if (strcmp(layer.layerName, validationLayerName) == 0)
                {
                    validationLayerPresent = true;
                    instanceLayers.push_back(validationLayerName);
                }
            }

            if (!validationLayerPresent)
                Log<LogSeverity::Error>("Validation layer {0} not present, validation is disabled", validationLayerName);
        }
        return instanceLayers;
    }
}
