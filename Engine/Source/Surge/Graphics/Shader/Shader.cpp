// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Shader/Shader.hpp"
#include "Surge/Graphics/Abstraction/Vulkan/VulkanShader.hpp"

namespace Surge
{
    Ref<Shader> Shader::Create(const Path& path)
    {
        return Ref<VulkanShader>::Create(path);
    }
}
