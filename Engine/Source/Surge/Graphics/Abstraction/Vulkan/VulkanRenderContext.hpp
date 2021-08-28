// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Abstraction/RenderContext.hpp"
#include <volk.h>

namespace Surge
{
    class SURGE_API VulkanRenderContext : public RenderContext
    {
    public:
        virtual void Initialize(Window* window) override;
        virtual void Shutdown() override;
    private:
        Vector<const char*> GetRequiredInstanceExtensions();
        Vector<const char*> GetRequiredInstanceLayers();
    private:
        struct PriveteData
        {
            VkInstance VulkanInstance;
        };
        PriveteData mPrivateData;
    };
}
