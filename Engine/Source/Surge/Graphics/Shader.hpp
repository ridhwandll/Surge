// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"

namespace Surge
{
    enum class ShaderType
    {
        None = -1,
        VertexShader,
        PixelShader,
    };

    class Shader : public RefCounted
    {
    public:
        Shader() = default;
        virtual ~Shader() = default;

        virtual const Path& GetPath() const = 0;

        static Ref<Shader> Create(const Path& path);
    };
}
