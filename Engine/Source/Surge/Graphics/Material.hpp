// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Buffer.hpp"
#include "Surge/Graphics/Shader/Shader.hpp"
#include "Surge/Graphics/Shader/ReflectionData.hpp"
#include "Surge/Graphics/Interface/UniformBuffer.hpp"
#include "Surge/Graphics/Interface/Texture.hpp"

namespace Surge
{
    class Material : public RefCounted
    {
    public:
        Material() = default;
        virtual ~Material() = default;

        virtual void UpdateForRendering() = 0;
        virtual void Bind(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<GraphicsPipeline>& gfxPipeline) const = 0;
        virtual void Load() = 0;
        virtual void Release() = 0;

        template <typename T>
        FORCEINLINE void Set(const String& name, const T& data)
        {
            if constexpr (std::is_same_v<T, Ref<Texture2D>>)
            {
                for (auto& [binding, res] : mShaderResources)
                {
                    if (res.Name == name)
                    {
                        mTextures[res.Binding] = data;
                        //Surge::Core::AddFrameEndCallback([&]() {
                        mUpdatePendingTextures.push_back({res.Binding, mTextures.at(res.Binding).Raw()});
                        //});
                    }
                }
            }
            else
            {
                const ShaderBufferMember* member = mShaderBuffer.GetMember(name);
                SG_ASSERT_NOMSG(member);
                SG_ASSERT(sizeof(data) == member->Size, "The size of the shader member and the size of the input data doesn't match!");
                mBufferMemory.Write((Byte*)&data, sizeof(data), member->MemoryOffset);
            }
        }

        template <typename T>
        FORCEINLINE T& Get(const String& name)
        {
            if constexpr (std::is_same_v<T, Ref<Texture2D>>)
            {
                for (auto& [binding, res] : mShaderResources)
                {
                    if (res.Name == name)
                    {
                        return mTextures[res.Binding];
                    }
                }
            }
            else
            {
                const ShaderBufferMember* member = mShaderBuffer.GetMember(name);
                SG_ASSERT_NOMSG(member);
                return mBufferMemory.Read<T>(member->MemoryOffset);
            }
        }

        const String& GetName() const { return mName; }
        const ShaderBuffer& GetShaderBuffer() const { return mShaderBuffer; }
        static Ref<Material> Create(const String& shaderName, const String& materialName);

    protected:
        Ref<Shader> mShader;
        String mName;

        // Buffer
        Buffer mBufferMemory;
        ShaderBuffer mShaderBuffer;
        Ref<UniformBuffer> mUniformBuffer;

        // Textures
        //   Binding - Res
        HashMap<Uint, ShaderResource> mShaderResources;
        HashMap<Uint, Ref<Texture2D>> mTextures;
        Vector<Pair<Uint, Texture2D*>> mUpdatePendingTextures;

        CallbackID mShaderReloadID;
    };

} // namespace Surge
