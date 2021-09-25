// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Pch.hpp"
//#include "Image.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanImage.hpp"

namespace Surge
{
    Ref<Image2D> Image2D::Create(const ImageSpecification& specification)
    {
        return Ref<VulkanImage2D>::Create(specification);
    }

    Ref<Image2D> Image2D::Create(const ImageSpecification& specification, const void* data)
    {
        return Ref<VulkanImage2D>::Create(specification, data);
    }

}
