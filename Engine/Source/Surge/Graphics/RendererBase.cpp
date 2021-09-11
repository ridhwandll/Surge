// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/RendererBase.hpp"
#include "Surge/Utility/Filesystem.hpp"

namespace Surge
{
    void RendererBase::Initialize()
    {
        mAllShaders.emplace_back(Shader::Create(BASE_SHADER_PATH"Simple.glsl"));
    }

    void RendererBase::Shutdown()
    {
        mAllShaders.clear();
    }

    Ref<Shader>& RendererBase::GetShader(const String& name)
    {
        for (Ref<Shader>& shader : mAllShaders)
        {
            if (Filesystem::RemoveExtension(shader->GetPath()) == String(BASE_SHADER_PATH + name))
                return shader;
        }

        SG_ASSERT_INTERNAL("No shaders found with name: {0}", name);
        return mDummyShader;
    }
}
