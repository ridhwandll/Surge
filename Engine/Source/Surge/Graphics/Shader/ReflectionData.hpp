// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/Graphics/Shader/Shader.hpp"
#include <map>

namespace Surge
{
    enum class ShaderType;

    // Represents Textures
    struct ShaderResource
    {
        enum class Usage
        {
            Sampled, Storage
        };

        Uint Set = 0;
        Uint Binding = 0;
        String Name = "";
        ShaderResource::Usage Type;
        Vector<ShaderType> ShaderStages; // Specify what shader stages the buffer is being used for
    };

    struct ShaderStageInput
    {
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
        enum class Usage
        {
            Storage, Uniform
        };

        Uint Set = 0;
        Uint Binding = 0;
        String BufferName = "";
        Uint Size = 0;
        Vector<ShaderBufferMember> Members = {};
        ShaderBuffer::Usage Type;
        Vector<ShaderType> ShaderStages;  // Specify what shader stages the buffer is being used for
    };

    struct ShaderPushConstant
    {
        String BufferName = "";
        Uint Size = 0;
        Vector<ShaderType> ShaderStages;  // Specify what shader stages the buffer is being used for
    };

    class ShaderReflectionData
    {
    public:
        ShaderReflectionData() = default;
        ~ShaderReflectionData() = default;

        void PushResource(const ShaderResource& res) { mShaderResources.push_back(res); }
        void PushStageInput(const ShaderStageInput& input, const ShaderType& stage, Uint location) { mStageInputs[stage][location] = input; }
        void PushBuffer(const ShaderBuffer& buffer)
        {
            SG_ASSERT(!buffer.BufferName.empty() || buffer.Members.size() != 0 || buffer.Size != 0, "ShaderBuffer is invalid!");
            mShaderBuffers.push_back(buffer);
        }
        void PushBufferPushConstant(const ShaderPushConstant& pushConstant) { mPushConstants.push_back(pushConstant); }

        const ShaderBuffer& GetBuffer(const String& name) const;
        const Vector<ShaderBuffer>& GetBuffers() const { return mShaderBuffers; }
        const Vector<ShaderPushConstant> GetPushConstantBuffers() const { return mPushConstants; }
        const ShaderBufferMember& GetBufferMember(const ShaderBuffer& buffer, const String& memberName) const;
        const Vector<ShaderResource>& GetResources() const { return mShaderResources; }
        const HashMap<ShaderType, std::map<Uint, ShaderStageInput>>& GetStageInputs() const { return mStageInputs; }
        const Vector<Uint> GetDescriptorSetCount() const { return mDescriptorSetsCount; }
    private:
        void ClearRepeatedMembers();
        void CalculateDescriptorSetCount();
    private:
        Vector<ShaderResource> mShaderResources{};
        Vector<ShaderBuffer> mShaderBuffers{};
        Vector<ShaderPushConstant> mPushConstants;

        // NOTE(AC3R): Keeping track of how many descriptor set we will need for the descriptor layout
        Vector<Uint> mDescriptorSetsCount;

        // Stage inputs, per shader stage
        HashMap<ShaderType, std::map<Uint/*location*/, ShaderStageInput/*Data*/>> mStageInputs{};

        friend class ShaderReflector;
    };
}