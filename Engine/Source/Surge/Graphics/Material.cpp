// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Material.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanMaterial.hpp"

namespace Surge
{
    Ref<Material> Material::Create(const String& shaderName, const String& materialName)
    {
        return Ref<VulkanMaterial>::Create(Core::GetRenderer()->GetShader(shaderName), materialName);
    }
} // namespace Surge