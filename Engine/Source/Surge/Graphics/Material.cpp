// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Material.hpp"

namespace Surge
{
    Material::Material(const Ref<Shader>& shader)
        : mShader(shader)
    {
        mShaderReloadID = mShader->AddReloadCallback([&]() { Load(); });
        Load();
    }

    void Material::Load()
    {
        mShaderBuffer = mShader->GetReflectionData().GetBuffer("Material");
        mBufferMemory.Allocate(mShaderBuffer.Size);
        mBufferMemory.ZeroInitialize();
    }

    Material::~Material()
    {
        mBufferMemory.Release();
        mShader->RemoveReloadCallback(mShaderReloadID);
    }

    void Material::Bind()
    {
    }

} // namespace Surge
