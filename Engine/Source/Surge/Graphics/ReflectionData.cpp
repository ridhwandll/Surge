// Copyright (c) - SurgeTechnologies - All rights reserved
#include "ReflectionData.hpp"

namespace Surge
{
    ShaderBuffer dummyBuffer = ShaderBuffer();
    ShaderBufferMember dummyBufferMember = ShaderBufferMember();

    void ShaderReflectionData::PushResource(const ShaderResource& res)
    {
        mShaderResources.push_back(res);
    }

    void ShaderReflectionData::PushBuffer(const ShaderBuffer& buffer)
    {
        mShaderBuffers.push_back(buffer);
    }

    const ShaderBuffer& ShaderReflectionData::GetBuffer(const String& name) const
    {
        for (const ShaderBuffer& buffer : mShaderBuffers)
            if (buffer.BufferName == name)
                return buffer;

        SG_ASSERT_INTERNAL("ShaderBuffer with name {0} doesn't exist in shader!", name);
        return dummyBuffer;
    }

    const ShaderBufferMember& ShaderReflectionData::GetBufferMember(const ShaderBuffer& buffer, const String& memberName) const
    {
        for (const ShaderBufferMember& member : buffer.Members)
            if (member.Name == memberName)
                return member;

        SG_ASSERT_INTERNAL("ShaderBufferMember with name {0} doesn't exist in {1} buffer!", memberName, buffer.BufferName);
        return dummyBufferMember;
    }

    void ShaderReflectionData::ValidateBuffer(const ShaderBuffer& buffer)
    {
        if (buffer.BufferName.empty() || buffer.Members.size() == 0 || buffer.Size == 0)
            SG_ASSERT_INTERNAL("ShaderBuffer is invalid!");
    }
}
