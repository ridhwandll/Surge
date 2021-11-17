// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Material.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMaterial.hpp"

namespace Surge
{
    Ref<Material> Material::Create(const String& shaderName)
    {
        return Ref<VulkanMaterial>::Create(Core::GetRenderer()->GetShader(shaderName));
    }
} // namespace Surge