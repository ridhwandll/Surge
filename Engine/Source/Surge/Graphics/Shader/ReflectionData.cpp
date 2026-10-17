// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Shader/ReflectionData.hpp"
#include "Surge/Graphics/RHI/Vulkan/VulkanUtils.hpp"
#include "Surge/Asset/Serializer/BinaryHelpers.hpp"
#include <SurgeReflect/Enum.hpp>

namespace Surge
{
    // We hate the warnings
    static ShaderBuffer sDummyBuffer = ShaderBuffer();
    static ShaderBufferMember sDummyBufferMember = ShaderBufferMember();

    /////
    // ShaderReflectionData
    /////

    const ShaderBuffer& ShaderReflectionData::GetBuffer(const String& name) const
    {
        for (const ShaderBuffer& buffer : mShaderBuffers)
            if (buffer.BufferName == name)
                return buffer;

        SG_ASSERT_INTERNAL("ShaderBuffer with name {0} doesn't exist in shader!", name);
        return sDummyBuffer;
    }

    const ShaderBufferMember& ShaderReflectionData::GetBufferMember(const ShaderBuffer& buffer, const String& memberName) const
    {
        for (const ShaderBufferMember& member : buffer.Members)
            if (member.Name == memberName)
                return member;

        SG_ASSERT_INTERNAL("ShaderBufferMember with name {0} doesn't exist in {1} buffer!", memberName, buffer.BufferName);
        return sDummyBufferMember;
    }

    void ShaderReflectionData::LogAll() const
    {
        String header = std::format("------------------ {} ------------------", mShaderName);
        Log<Severity::Debug>(header);
        for (const auto& [stage, inputs] : mStageInputs)
        {
            for (const auto& [location, input] : inputs)
            {
                Log<Severity::Info>("Stage Input: {0}, Location: {1}, Type: {2}, Stage: {3}", input.Name, location, SurgeReflect::EnumToString(input.DataType), VulkanUtils::ShaderTypeToString(stage));
            }
        }
        for(const ShaderBuffer& buffer : mShaderBuffers)
        {
            Log<Severity::Info>("Buffer: {0}, Set: {1}, Binding: {2}, Size: {3}, Usage: {4}, Stages: {5}", buffer.BufferName, buffer.Set, buffer.Binding, buffer.Size,
                buffer.ShaderUsage == ShaderBuffer::Usage::STORAGE ? "Storage" : "Uniform", VulkanUtils::ShaderTypeToString(buffer.ShaderStages));
            for (const ShaderBufferMember& member : buffer.Members)
            {
                Log<Severity::Info>("\tMember: {0}, Offset: {1}, Size: {2}, Type: {3}", member.Name, member.MemoryOffset, member.Size, SurgeReflect::EnumToString(member.DataType));
            }
        }
        for (const ShaderResource& res : mShaderResources)
        {
            Log<Severity::Info>("Resource: {0}, Set: {1}, Binding: {2}, Usage: {3}, Stages: {4}", res.Name, res.Set, res.Binding,
                res.ShaderUsage == ShaderResource::Usage::SAMPLED ? "Sampled" : "Storage", VulkanUtils::ShaderTypeToString(res.ShaderStages));
        }
        for (const ShaderPushConstant& pushConstant : mPushConstants)
        {
            Log<Severity::Info>("Push Constant: {0}, Size: {1}, Stages: {2}", pushConstant.BufferName, pushConstant.Size, VulkanUtils::ShaderTypeToString(pushConstant.ShaderStages));
        }


        String footer(header.length(), '-');		
        Log<Severity::Debug>(footer);
    }

    void ShaderReflectionData::ClearRepeatedMembers()
    {
        // Just a simple O(n^2) algorithm to find the repeated members

        // For buffers
        for (Uint i = 0; i < mShaderBuffers.size(); i++)
        {
            for (Uint j = i + 1; j < mShaderBuffers.size(); j++)
            {
                ShaderBuffer& bufferData1 = mShaderBuffers[i];
                ShaderBuffer& bufferData2 = mShaderBuffers[j];

                // Check if the binding and set of the buffers is the same
                if (bufferData1.Set == bufferData2.Set && bufferData1.Binding == bufferData2.Binding)
                {
                    // If the binding and set of the buffer is the same, then we add the shaderstages from the second buffer into
                    // the first one (so like we combine them), and then erase the second buffer so we have only one combined
                    bufferData1.ShaderStages |= bufferData2.ShaderStages;

                    // Deleting the second copy of the same member
                    mShaderBuffers.erase(mShaderBuffers.begin() + j);
                }
            }
        }

        // For textures
        for (Uint i = 0; i < mShaderResources.size(); i++)
        {
            for (Uint j = i + 1; j < mShaderResources.size(); j++)
            {
                auto& textureData1 = mShaderResources[i];
                auto& textureData2 = mShaderResources[j];

                // Check if the binding and set of the textures is the same
                if (textureData1.Set == textureData2.Set && textureData1.Binding == textureData2.Binding)
                {
                    // If the binding and set of the texture is the same, then we add the shaderstages from the second texture into
                    // the first one (so like we combine them), and then erase the second texture so we have only one combined
                    textureData1.ShaderStages |= textureData2.ShaderStages;

                    // Deleting the second copy of the same member
                    mShaderResources.erase(mShaderResources.begin() + j);
                }
            }
        }

        // For Push Constants
        for (Uint i = 0; i < mPushConstants.size(); i++)
        {
            for (Uint j = i + 1; j < mPushConstants.size(); j++)
            {
                auto& pushConstant1 = mPushConstants[i];
                auto& pushConstant2 = mPushConstants[j];

                // Check if the size and the name of the push constants are the same
                if (pushConstant1.BufferName == pushConstant2.BufferName && pushConstant1.Size == pushConstant2.Size)
                {
                    // If the size and the name of the push constants are the same, then we add the shaderstages from the second push
                    // constant buffer into the first one (so like we combine them), and then erase the second pushconstant buffer so
                    // we have only one combined |Explanation 100|
                    pushConstant1.ShaderStages |= pushConstant2.ShaderStages;

                    // Deleting the second copy of the same member
                    mPushConstants.erase(mPushConstants.begin() + j);
                }
            }
        }
    }

    // Enums are stored as Uint and range checked on read, a stale/corrupt file must not produce out of range values
    static bool ReadEnum(BinaryReader& reader, Uint maxValue, Uint& outValue)
    {
        reader.Read(outValue);
        return !reader.Failed && outValue <= maxValue;
    }

    static constexpr Uint MAX_SHADER_TYPE_BITS = static_cast<Uint>(ShaderType::VERTEX) | static_cast<Uint>(ShaderType::FRAGMENT) | static_cast<Uint>(ShaderType::COMPUTE);

    void ShaderReflectionData::Serialize(Vector<Byte>& out) const
    {
        WriteStr(out, mShaderName);

        WriteData(out, static_cast<Uint>(mShaderResources.size()));
        for (const ShaderResource& res : mShaderResources)
        {
            WriteData(out, res.Set);
            WriteData(out, res.Binding);
            WriteData(out, res.ArraySize);
            WriteStr(out, res.Name);
            WriteData(out, static_cast<Uint>(res.ShaderUsage));
            WriteData(out, static_cast<Uint>(res.ShaderStages));
        }

        WriteData(out, static_cast<Uint>(mShaderBuffers.size()));
        for (const ShaderBuffer& buffer : mShaderBuffers)
        {
            WriteData(out, buffer.Set);
            WriteData(out, buffer.Binding);
            WriteStr(out, buffer.BufferName);
            WriteData(out, buffer.Size);
            WriteData(out, static_cast<Uint>(buffer.ShaderUsage));
            WriteData(out, static_cast<Uint>(buffer.ShaderStages));
            WriteData(out, static_cast<Uint>(buffer.Members.size()));
            for (const ShaderBufferMember& member : buffer.Members)
            {
                WriteStr(out, member.Name);
                WriteData(out, member.MemoryOffset);
                WriteData(out, static_cast<Uint>(member.DataType));
                WriteData(out, member.Size);
            }
        }

        WriteData(out, static_cast<Uint>(mPushConstants.size()));
        for (const ShaderPushConstant& pushConstant : mPushConstants)
        {
            WriteStr(out, pushConstant.BufferName);
            WriteData(out, pushConstant.Size);
            WriteData(out, static_cast<Uint>(pushConstant.ShaderStages));
        }

        WriteData(out, static_cast<Uint>(mStageInputs.size()));
        for (const auto& [stage, inputs] : mStageInputs)
        {
            WriteData(out, static_cast<Uint>(stage));
            WriteData(out, static_cast<Uint>(inputs.size()));
            for (const auto& [location, input] : inputs)
            {
                WriteData(out, location);
                WriteStr(out, input.Name);
                WriteData(out, input.Size);
                WriteData(out, input.Offset);
                WriteData(out, static_cast<Uint>(input.DataType));
            }
        }
    }

    bool ShaderReflectionData::Deserialize(BinaryReader& reader)
    {
        *this = ShaderReflectionData();
        Uint value = 0;
        Uint count = 0;

        mShaderName = reader.ReadStr();

        reader.Read(count);
        if (!reader.Fits(count, sizeof(Uint) * 6))
            return false;
        mShaderResources.resize(count);
        for (ShaderResource& res : mShaderResources)
        {
            reader.Read(res.Set);
            reader.Read(res.Binding);
            reader.Read(res.ArraySize);
            res.Name = reader.ReadStr();
            if (!ReadEnum(reader, static_cast<Uint>(ShaderResource::Usage::STORAGE), value))
                return false;
            res.ShaderUsage = static_cast<ShaderResource::Usage>(value);
            if (!ReadEnum(reader, MAX_SHADER_TYPE_BITS, value))
                return false;
            res.ShaderStages = static_cast<ShaderType>(value);
        }

        reader.Read(count);
        if (!reader.Fits(count, sizeof(Uint) * 7))
            return false;
        mShaderBuffers.resize(count);
        for (ShaderBuffer& buffer : mShaderBuffers)
        {
            reader.Read(buffer.Set);
            reader.Read(buffer.Binding);
            buffer.BufferName = reader.ReadStr();
            reader.Read(buffer.Size);
            if (!ReadEnum(reader, static_cast<Uint>(ShaderBuffer::Usage::UNIFORM), value))
                return false;
            buffer.ShaderUsage = static_cast<ShaderBuffer::Usage>(value);
            if (!ReadEnum(reader, MAX_SHADER_TYPE_BITS, value))
                return false;
            buffer.ShaderStages = static_cast<ShaderType>(value);

            reader.Read(count);
            if (!reader.Fits(count, sizeof(Uint) * 4))
                return false;
            buffer.Members.resize(count);
            for (ShaderBufferMember& member : buffer.Members)
            {
                member.Name = reader.ReadStr();
                reader.Read(member.MemoryOffset);
                if (!ReadEnum(reader, static_cast<Uint>(ShaderDataType::STRUCT), value))
                    return false;
                member.DataType = static_cast<ShaderDataType>(value);
                reader.Read(member.Size);
            }
        }

        reader.Read(count);
        if (!reader.Fits(count, sizeof(Uint) * 3))
            return false;
        mPushConstants.resize(count);
        for (ShaderPushConstant& pushConstant : mPushConstants)
        {
            pushConstant.BufferName = reader.ReadStr();
            reader.Read(pushConstant.Size);
            if (!ReadEnum(reader, MAX_SHADER_TYPE_BITS, value))
                return false;
            pushConstant.ShaderStages = static_cast<ShaderType>(value);
        }

        Uint stageCount = 0;
        reader.Read(stageCount);
        if (!reader.Fits(stageCount, sizeof(Uint) * 2))
            return false;
        for (Uint i = 0; i < stageCount; i++)
        {
            if (!ReadEnum(reader, MAX_SHADER_TYPE_BITS, value))
                return false;
            std::map<Uint, ShaderStageInput>& inputs = mStageInputs[static_cast<ShaderType>(value)];

            reader.Read(count);
            if (!reader.Fits(count, sizeof(Uint) * 5))
                return false;
            for (Uint j = 0; j < count; j++)
            {
                Uint location = 0;
                reader.Read(location);
                ShaderStageInput& input = inputs[location];
                input.Name = reader.ReadStr();
                reader.Read(input.Size);
                reader.Read(input.Offset);
                if (!ReadEnum(reader, static_cast<Uint>(ShaderDataType::STRUCT), value))
                    return false;
                input.DataType = static_cast<ShaderDataType>(value);
            }
        }

        return !reader.Failed;
    }
} // namespace Surge
