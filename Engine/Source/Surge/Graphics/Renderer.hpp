// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include "Surge/Graphics/Shader.hpp"
#include "Texture.hpp"
#include "Mesh.hpp"

//TODO: Remove
#include <volk.h> 
#include "RenderCommandBuffer.hpp"

#define FRAMES_IN_FLIGHT 3
#define BASE_SHADER_PATH "Engine/Assets/Shaders/"

namespace Surge
{
    struct RendererData
    {
        Ref<RenderCommandBuffer> RenderCmdBuffer;
        Ref<Texture2D> TestTexture;
        VkDescriptorPool DescriptorPool;
        VkDescriptorSet DescriptorSet;

        Ref<Mesh> CubeMesh;

        Ref<Shader> mDummyShader = nullptr;
        Vector<Ref<Shader>> mAllShaders;
    };

    class Renderer
    {
    public:
        Renderer() = default;
        ~Renderer() = default;

        void Initialize();

        void RenderRectangle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale); // TODO: Remove

        void Shutdown();

        Ref<Shader>& GetShader(const String& name);
    private:
        Scope<RendererData> mData;
    };
}
