// Copyright (c) - SurgeTechnologies - All rights reserved
#include "ReflectionData.hpp"

namespace Surge
{
    // We hate the warnings
    static ShaderBuffer sDummyBuffer = ShaderBuffer();
    static ShaderBufferMember sDummyBufferMember = ShaderBufferMember();

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

    void ShaderReflectionData::CalculateDescriptorSetCount()
    {
        // Adding all the sets used in the shader needed to make the amount of descriptor layout/sets
        for (const ShaderBuffer& buffer : mShaderBuffers)
        {
            // Check if the number of the set is already mentioned in the vector
            if (std::find(mDescriptorSets.begin(), mDescriptorSets.end(), buffer.Set) == mDescriptorSets.end())
                mDescriptorSets.push_back(buffer.Set);
        }

        for (const ShaderResource& res : mShaderResources)
        {
            // Check if the number of the set is already mentioned in the vector
            if (std::find(mDescriptorSets.begin(), mDescriptorSets.end(), res.Set) == mDescriptorSets.end())
                mDescriptorSets.push_back(res.Set);
        }
    }

} // namespace Surge
