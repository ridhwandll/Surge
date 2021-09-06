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
        Uint Set = 0;
        Uint Binding = 0;
        String Name = "";
    };

    struct ShaderStageInput
    {
        Uint Location = 0;
        String Name = "None";
        Uint Size = 0;
        Uint Offset;
        ShaderDataType DataType = ShaderDataType::None;
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
        Uint Set = 0;
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

        void PushResource(const ShaderResource& res) { mShaderResources.push_back(res); }
        void PushStageInput(const ShaderStageInput& input) { mStageInputs.push_back(input); }
        void PushBuffer(const ShaderBuffer& buffer)
        {
            SG_ASSERT(!buffer.BufferName.empty() || buffer.Members.size() != 0 || buffer.Size != 0, "ShaderBuffer is invalid!");
            mShaderBuffers.push_back(buffer);
        }

        const ShaderBuffer& GetBuffer(const String& name) const;
        const Vector<ShaderBuffer>& GetBuffers() const { return mShaderBuffers; }
        const ShaderBufferMember& GetBufferMember(const ShaderBuffer& buffer, const String& memberName) const;

        const Vector<ShaderResource>& GetResources() const { return mShaderResources; }
        const Vector<ShaderStageInput>& GetStageInputs() const { return mStageInputs; }
    private:
        ShaderType mShaderType = ShaderType::None;
        Vector<ShaderResource> mShaderResources{};
        Vector<ShaderBuffer> mShaderBuffers{};
        Vector<ShaderStageInput> mStageInputs{};
    };
}
