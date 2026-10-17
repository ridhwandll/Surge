// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Shader/ShaderManager.hpp"
#include "Surge/Graphics/Shader/ShaderBinaryFormat.hpp"

namespace Surge
{
    void ShaderManager::Initialize(const String& baseShaderPath)
    {
        mBaseShaderPath = baseShaderPath;
    }

    void ShaderManager::Load(const String& shaderName, ShaderType types)
    {
        const String path = std::format("{0}/{1}", mBaseShaderPath, shaderName);
        if (sCompileHook)
            sCompileHook(path, ShaderBinary::GetCookedPath(path), types);

        mShaders.emplace_back(Shader()).Load(path, types);
    }

    void ShaderManager::Shutdown()
    {
    }

    const Shader& ShaderManager::Get(const String& shaderName) const
    {
        for(const Shader& shader : mShaders)
        {
            if(shader.GetName() == shaderName)
                return shader;
        }
        SG_ASSERT_INTERNAL("Shader not found!");
        return mDummyShader;
    }

} // namespace Surge
