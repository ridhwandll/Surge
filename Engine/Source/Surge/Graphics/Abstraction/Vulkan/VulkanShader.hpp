// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Shader.hpp"
#include <volk.h>

namespace Surge
{
    class VulkanShader : public Shader
    {
    public:
        VulkanShader(const Path& path);
        virtual ~VulkanShader() override;

        virtual const Path& GetPath() const override { return mPath; }
    private:
        void Reload();
        void ParseShader();

        // Compiles to SPIR-V
        void Compile();
    private:
        Path mPath;
        std::unordered_map<ShaderType, String> mShaderSources{};
        std::unordered_map<ShaderType, Vector<Uint>> mShaderSPIRVs{};
    };
}
