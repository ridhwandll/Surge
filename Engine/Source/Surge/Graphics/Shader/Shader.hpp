// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Graphics/Shader/ReflectionData.hpp"

namespace Surge
{
    class Shader
    {
    public:
        Shader() = default;
        ~Shader() = default;

        // Loads the cooked shader of glslPath (ShaderBinary), the runtime never compiles GLSL
        bool Load(const String& glslPath, ShaderType type);

        const ShaderReflectionData& GetReflectionData() const { return mReflectionData; }
        const Vector<SPIRVHandle>& GetSPIRVs() const { return mShaderSPIRVs; }
        const String& GetPath() const { return mPath; }
        const String& GetName() const { return mReflectionData.GetShaderName(); }
        ShaderType GetTypes() const { return mTypesBit; }
    private:
        String mPath;
        Vector<SPIRVHandle> mShaderSPIRVs;
        ShaderType mTypesBit = ShaderType::NONE;
        ShaderReflectionData mReflectionData;
    };

} // namespace Surge
