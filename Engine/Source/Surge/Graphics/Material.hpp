// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Buffer.hpp"
#include "Surge/Graphics/Shader/Shader.hpp"
#include "Surge/Graphics/Shader/ReflectionData.hpp"
#include "Surge/Graphics/UniformBuffer.hpp"

namespace Surge
{
    class Material : public RefCounted
    {
    public:
        Material() = default;
        virtual ~Material() = default;

        virtual void Bind(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<GraphicsPipeline>& gfxPipeline) const = 0;
        virtual void Load() = 0;
        virtual void Release() = 0;

        const ShaderBuffer& GetShaderBuffer() const { return mShaderBuffer; }

        template <typename T>
        void Set(const String& name, const T& data)
        {
            const ShaderBufferMember* member = mShaderBuffer.GetMember(name);
            SG_ASSERT_NOMSG(member);
            mBufferMemory.Write((Byte*)&data, sizeof(data), member->MemoryOffset);
        }

        template <typename T>
        T& Get(const String& name)
        {
            const ShaderBufferMember* member = mShaderBuffer.GetMember(name);
            SG_ASSERT_NOMSG(member);
            return mBufferMemory.Read<T>(member->MemoryOffset);
        }

        static Ref<Material> Create(const String& shaderName);

    protected:
        Ref<Shader> mShader;
        Buffer mBufferMemory;
        ShaderBuffer mShaderBuffer;
        Uint mBinding;
        Ref<UniformBuffer> mUniformBuffer;
        CallbackID mShaderReloadID;
    };
} // namespace Surge