// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include "Surge/Graphics/Camera/EditorCamera.hpp"
#include "Surge/Graphics/Mesh.hpp"
#include "Surge/Graphics/RenderCommandBuffer.hpp"
#include "Surge/Graphics/Shader/Shader.hpp"
#include "Surge/Graphics/Shader/ShaderSet.hpp"
#include "Surge/Graphics/Texture.hpp"

#define FRAMES_IN_FLIGHT 3
#define BASE_SHADER_PATH "Engine/Assets/Shaders"

namespace Surge
{
    struct DrawCommand
    {
        DrawCommand(const Ref<Mesh>& mesh, const glm::mat4& transform) : Mesh(mesh), Transform(transform) {}

        Ref<Surge::Mesh> Mesh;
        glm::mat4 Transform;
    };

    struct RendererData
    {
        Ref<Framebuffer> OutputFrambuffer;
        Ref<RenderCommandBuffer> RenderCmdBuffer;
        Vector<DrawCommand> DrawList;

        Surge::ShaderSet ShaderSet;

        glm::mat4 ViewMatrix;
        glm::mat4 ProjectionMatrix;
        glm::mat4 ViewProjection;
    };

    class Renderer
    {
    public:
        Renderer() = default;
        virtual ~Renderer() = default;

        virtual void Initialize() = 0;
        virtual void Shutdown() = 0;

        virtual void BeginFrame(const Camera& camera, const glm::mat4& transform) = 0;
        virtual void BeginFrame(const EditorCamera& camera) = 0;
        virtual void EndFrame() = 0;
        virtual void SubmitMesh(const Ref<Mesh>& mesh, const glm::mat4& transform) = 0;

        virtual void BeginRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Framebuffer>& framebuffer) = 0;
        virtual void EndRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer) = 0;

        RendererData* GetData() { return mData.get(); }
        Ref<Shader>& GetShader(const String& name);
        Ref<Framebuffer>& GetFramebuffer(); //TODO REMOVE: Have something like FramebufferSet(similar to ShaderSet)

    protected:
        Scope<RendererData> mData;
    };

} // namespace Surge