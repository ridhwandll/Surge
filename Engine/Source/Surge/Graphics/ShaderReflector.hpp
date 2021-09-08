// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/ReflectionData.hpp"

namespace Surge
{
    class ShaderReflector
    {
    public:
        ShaderReflector() = default;
        ShaderReflectionData Reflect(const Vector<SPIRVHandle>& spirvHandles);
    private:
        Ref<Shader> mShader;
    };
}
