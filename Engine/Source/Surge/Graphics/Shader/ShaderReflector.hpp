// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Shader/ReflectionData.hpp"

namespace Surge
{
    class SURGE_API ShaderReflector
    {
    public:
        ShaderReflector() = default;
        ShaderReflectionData Reflect(const Vector<SPIRVHandle>& spirvHandles);
    };
} // namespace Surge
