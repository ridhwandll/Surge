// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Buffer.hpp"
#include "Surge/Graphics/Shader/Shader.hpp"
#include "Surge/Graphics/Shader/ReflectionData.hpp"

namespace Surge
{
    class Material : public RefCounted
    {
    public:
        Material() = default;
        Material(const Ref<Shader>& shader);
        ~Material();

        void Bind();
        const ShaderBuffer& GetShaderBuffer() const { return mShaderBuffer; }

        template <typename T>
        void Set(const String& name, const T& data)
        {
            const ShaderBufferMember* member = mShaderBuffer.GetMember(name);
            SG_ASSERT_NOMSG(member);
            mBufferMemory.Write((byte*)&data, sizeof(value), member.MemoryOffset);
        }

        template <typename T>
        T& Get(const String& name)
        {
            const ShaderBufferMember* member = mShaderBuffer.GetMember(name);
            SG_ASSERT_NOMSG(member);
            return mBufferMemory.Read<T>(member->MemoryOffset);
        }

    private:
        void Load();

    private:
        Ref<Shader> mShader;
        Buffer mBufferMemory;
        ShaderBuffer mShaderBuffer;
        CallbackID mShaderReloadID;
    };

} // namespace Surge
