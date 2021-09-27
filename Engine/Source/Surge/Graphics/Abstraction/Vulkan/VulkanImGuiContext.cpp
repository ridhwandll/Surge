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
        VkDevice logicalDevice = vulkanDevice->GetLogicalDevice();

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
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows
        io.FontDefault = io.Fonts->AddFontFromFileTTF("Engine/Assets/Fonts/Ruda-Medium.ttf", 16.0f);
        ImGui::StyleColorsDark();

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
        initInfo.PhysicalDevice = vulkanDevice->GetPhysicalDevice();
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
        vulkanDevice->InstantSubmit(VulkanQueueType::Graphics, [&](VkCommandBuffer& cmd)
            {
                ImGui_ImplVulkan_CreateFontsTexture(cmd);
            });

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void VulkanImGuiContext::Destroy()
    {
        VulkanRenderContext* renderContext = static_cast<VulkanRenderContext*>(mVulkanRenderContext);
        vkDestroyDescriptorPool(renderContext->mDevice.GetLogicalDevice(), mImguiPool, nullptr);
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
        VulkanSwapChain* swapchain = ((VulkanRenderContext*)CoreGetRenderContext().get())->GetSwapChain();
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), swapchain->GetVulkanCommandBuffers()[CoreGetRenderContext()->GetFrameIndex()]);
    }

    void VulkanImGuiContext::EndFrame()
    {
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
}
