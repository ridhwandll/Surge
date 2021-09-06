// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"

namespace Surge
{
    enum class ShaderType
    {
        None = 0, VertexShader, PixelShader, ComputeShader
    };

    enum class ShaderDataType
    {
        None, Int, UInt, Float, Float2, Float3, Float4, Mat2, Mat4, Mat3, Bool
    };

    inline const char* ShaderDataTypeToString(const ShaderDataType& type)
    {
        switch (type)
        {
        case ShaderDataType::Int:    return "Int";
        case ShaderDataType::UInt:   return "UInt";
        case ShaderDataType::Float:  return "Float";
        case ShaderDataType::Float2: return "Float2";
        case ShaderDataType::Float3: return "Float3";
        case ShaderDataType::Float4: return "Float4";
        case ShaderDataType::Mat2:   return "Mat2";
        case ShaderDataType::Mat4:   return "Mat4";
        case ShaderDataType::Mat3:   return "Mat3";
        case ShaderDataType::Bool:   return "Bool";
        case ShaderDataType::None: SG_ASSERT_INTERNAL("ShaderDataType::None is invalid in this case!");
        }
        SG_ASSERT_INTERNAL("Unknown ShaderDataType!");
        return "None";
    }

    inline Uint ShaderDataTypeSize(ShaderDataType type)
    {
        switch (type)
        {
        case ShaderDataType::Float:    return 4;
        case ShaderDataType::Float2:   return 4 * 2;
        case ShaderDataType::Float3:   return 4 * 3;
        case ShaderDataType::Float4:   return 4 * 4;
        case ShaderDataType::Mat3:     return 4 * 3 * 3;
        case ShaderDataType::Mat4:     return 4 * 4 * 4;
        case ShaderDataType::Int:      return 4;
        case ShaderDataType::Bool:     return 4;
        default: SG_ASSERT_INTERNAL("Invalid case!");
        }

        SG_ASSERT_INTERNAL("Unknown ShaderDataType!");
        return 0;
    }

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
