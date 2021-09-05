// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"

namespace Surge
{
    enum class ShaderType
    {
        None = 0,
        VertexShader,
        PixelShader,
        ComputeShader
    };

    struct SPIRVHandle
    {
        Vector<Uint> SPIRV;
        ShaderType Type;
    };

    class ShaderReflectionData;
    class Shader : public RefCounted
    {
    public:
        Shader() = default;
        virtual ~Shader() = default;

        virtual void Reload() = 0;
        virtual const std::unordered_map<ShaderType, ShaderReflectionData>& GetReflectionData() const = 0;
        virtual const Vector<SPIRVHandle>& GetSPIRVs() const = 0;
        virtual const Path& GetPath() const = 0;

        static Ref<Shader> Create(const Path& path);
    };
}
