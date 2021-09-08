// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include <shaderc/shaderc.h>
#include "Surge/Graphics/Shader.hpp"

namespace Surge::VulkanUtils
{
    ShaderType ShaderTypeFromString(const String& type);
    String ShaderTypeToString(const ShaderType& type);
    shaderc_shader_kind ShadercShaderKindFromSurgeShaderType(const ShaderType& type);
}
