// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Graphics/Shader.hpp"

namespace Surge
{
    enum class ShaderType;

    // Represents Textures
    struct ShaderResource
    {
        Uint Binding = 0;
        String Name = "";
    };

    // Represents a ConstantBuffer Member
    struct ShaderBufferMember
    {
        String Name = "";
        Uint MemoryOffset = 0;
    };

    // Represents a ConstantBuffer
    struct ShaderBuffer
    {
        Uint Binding = 0;
        String BufferName = "";
        Uint Size = 0;
        Vector<ShaderBufferMember> Members = {};
    };

    class ShaderReflectionData
    {
    public:
        ShaderReflectionData() = default;
        ~ShaderReflectionData() = default;

        void SetDomain(const ShaderType& shaderType) { mShaderType = shaderType; }
        void PushResource(const ShaderResource& res);
        void PushBuffer(const ShaderBuffer& buffer);
        void ValidateBuffer(const ShaderBuffer& buffer);

        const ShaderBuffer& GetBuffer(const String& name) const;
        const ShaderBufferMember& GetBufferMember(const ShaderBuffer& buffer, const String& memberName) const;
        const Vector<ShaderResource>& GetResources() const { return mShaderResources; }
        const Vector<ShaderBuffer>& GetBuffers() const { return mShaderBuffers; }
    private:
        ShaderType mShaderType = ShaderType::None;
        Vector<ShaderResource> mShaderResources;
        Vector<ShaderBuffer> mShaderBuffers;
    };
}
