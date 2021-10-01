// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include "Surge/Graphics/Shader/Shader.hpp"
#include "Surge/Graphics/Camera/EditorCamera.hpp"
#include "Surge/Graphics/Texture.hpp"
#include "Surge/Graphics/Mesh.hpp"
#include "Surge/Graphics/Shader/ShaderSet.hpp"
#include "Surge/Graphics/RenderCommandBuffer.hpp"

#define FRAMES_IN_FLIGHT 3
#define BASE_SHADER_PATH "Engine/Assets/Shaders"

namespace Surge
{
    struct DrawCommand
    {
        DrawCommand(const Ref<Mesh>& mesh, const glm::mat4& transform)
            : Mesh(mesh), Transform(transform) {}

        Ref<Surge::Mesh> Mesh;
        glm::mat4 Transform;
    };

    struct RendererData
    {
        Ref<RenderCommandBuffer> RenderCmdBuffer;
        Vector<DrawCommand> DrawList;

        Surge::ShaderSet ShaderSet;

        glm::mat4 ViewMatrix;
        glm::mat4 ProjectionMatrix;
        glm::mat4 ViewProjection;
    };

    // Simply a class that renders to a texture
    class Renderer
    {
    public:
        Renderer() = default;
        virtual ~Renderer() = default;

        virtual void Initialize() = 0;
        virtual void Shutdown() = 0;

        virtual void BeginFrame(const EditorCamera& camera) = 0;
        virtual void EndFrame() = 0;
        virtual void SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform) = 0;

        Ref<Shader>& GetShader(const String& name);
    protected:
        Scope<RendererData> mData;
    };
}
