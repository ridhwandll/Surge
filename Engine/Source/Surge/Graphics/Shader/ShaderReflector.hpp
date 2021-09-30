// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Shader/ReflectionData.hpp"

namespace Surge
{
    class ShaderReflector
    {
    public:
        ShaderReflector() = default;
        ShaderReflectionData Reflect(const Vector<SPIRVHandle>& spirvHandles);
    };
}
