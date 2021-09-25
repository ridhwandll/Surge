// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Abstraction/Vulkan/VulkanTexture.hpp"

namespace Surge
{
    Ref<Texture2D> Texture2D::Create(const String& filepath, TextureSpecification specification)
    {
        return Ref<VulkanTexture2D>::Create(filepath, specification);
    }
}