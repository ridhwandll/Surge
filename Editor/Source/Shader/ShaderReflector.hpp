// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Shader/ReflectionData.hpp"

namespace Surge
{
    // Cook time only (ShaderCompiler), the reflection ships inside the cooked shader so the runtime doesn't need SPIRV-Cross
    class ShaderReflector
    {
    public:
        ShaderReflector() = default;
        ShaderReflectionData Reflect(const String& shaderName, const Vector<SPIRVHandle>& spirvHandles);
    };

} // namespace Surge
