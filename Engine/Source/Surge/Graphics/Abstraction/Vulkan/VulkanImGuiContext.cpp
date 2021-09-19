// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanImGuiContext.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanRenderContext.hpp"
#include <ImGui/Backends/imgui_impl_vulkan.h>
#include <ImGui/Backends/imgui_impl_win32.h>

namespace Surge
{
    static void ImGuiCheckVkResult(VkResult err)
    {
        VK_CALL(err);
    }

    void VulkanImGuiContext::Initialize(void* vulkanRenderContext)
    {
        mVulkanRenderContext = vulkanRenderContext;
        VulkanRenderContext* renderContext = static_cast<VulkanRenderContext*>(vulkanRenderContext);
        VulkanDevice* vulkanDevice = &renderContext->mDevice;
        VkDevice logicalDevice = vulkanDevice->GetLogicaldevice();

        VkDescriptorPoolSize poolSizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 100 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 100 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 100 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 100 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 100 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 100 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 100 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100 }
        };
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 100 * IM_ARRAYSIZE(poolSizes);
        poolInfo.poolSizeCount = (Uint)IM_ARRAYSIZE(poolSizes);
        poolInfo.pPoolSizes = poolSizes;
        VK_CALL(vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &mImguiPool));

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        ImGui_ImplWin32_Init(CoreGetWindow()->GetNativeWindowHandle());
        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = renderContext->mVulkanInstance;
        initInfo.PhysicalDevice = vulkanDevice->GetPhysicaldevice();
        initInfo.Device = logicalDevice;
        initInfo.QueueFamily = vulkanDevice->GetQueueFamilyIndices().GraphicsQueue;
        initInfo.Queue = vulkanDevice->GetGraphicsQueue();
        initInfo.DescriptorPool = mImguiPool;
        initInfo.MinImageCount = 2;
        initInfo.ImageCount = 3;
        initInfo.Allocator = VK_NULL_HANDLE;
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.CheckVkResultFn = ImGuiCheckVkResult;
        ImGui_ImplVulkan_Init(&initInfo, renderContext->mSwapChain.GetVulkanRenderPass()); 

        VkCommandBuffer cmd = nullptr;
        vulkanDevice->BeginOneTimeCmdBuffer(cmd, VulkanQueueType::Graphics);
        ImGui_ImplVulkan_CreateFontsTexture(cmd);
        vulkanDevice->EndOneTimeCmdBuffer(cmd, VulkanQueueType::Graphics);

        ImGui_ImplVulkan_DestroyFontUploadObjects();
        VulkanSwapChain* swapchain = (VulkanSwapChain*)CoreGetRenderContext()->GetSwapChain();
    }

    void VulkanImGuiContext::Destroy()
    {
        VulkanRenderContext* renderContext = static_cast<VulkanRenderContext*>(mVulkanRenderContext);
        vkDestroyDescriptorPool(renderContext->mDevice.GetLogicaldevice(), mImguiPool, nullptr);
        ImGui_ImplWin32_Shutdown();
        ImGui_ImplVulkan_Shutdown();
        ImGui::DestroyContext();
    }

    void VulkanImGuiContext::BeginFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    void VulkanImGuiContext::Render()
    {
        VulkanSwapChain* swapchain = (VulkanSwapChain*)CoreGetRenderContext()->GetSwapChain();

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), swapchain->GetVulkanCommandBuffers()[CoreGetRenderContext()->GetFrameIndex()]);
    }

    void VulkanImGuiContext::EndFrame()
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
}
