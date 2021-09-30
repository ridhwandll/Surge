// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include "Surge/Graphics/Shader/Shader.hpp"
#include "Surge/Graphics/Camera/EditorCamera.hpp"
#include "Surge/Graphics/Texture.hpp"
#include "Surge/Graphics/Mesh.hpp"
#include "Surge/Graphics/Shader/ShaderSet.hpp"

//TODO: Remove
#include <volk.h> 
#include "RenderCommandBuffer.hpp"

#define FRAMES_IN_FLIGHT 3
#define BASE_SHADER_PATH "Engine/Assets/Shaders"

namespace Surge
{
    struct RendererData
    {
        Ref<RenderCommandBuffer> RenderCmdBuffer;
        Ref<Mesh> CubeMesh; //TODO: Remove, have a Mesh DrawList

        ShaderSet ShaderSet;

        glm::mat4 ViewMatrix;
        glm::mat4 ProjectionMatrix;
    };

    class Renderer
    {
    public:
        Renderer() = default;
        ~Renderer() = default;

        void Initialize();
        void Shutdown();

        void BeginFrame(const EditorCamera& camera);
        void EndFrame();
        void RenderRectangle(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale); // TODO: Remove

        Ref<Shader>& GetShader(const String& name);
    private:
        Scope<RendererData> mData;
    };
}
