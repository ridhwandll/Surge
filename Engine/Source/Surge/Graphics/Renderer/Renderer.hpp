// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include "Surge/Graphics/Camera/EditorCamera.hpp"
#include "Surge/Graphics/Mesh.hpp"
#include "Surge/Graphics/RenderCommandBuffer.hpp"
#include "Surge/Graphics/Shader/Shader.hpp"
#include "Surge/Graphics/Shader/ShaderSet.hpp"
#include "Surge/Graphics/Texture.hpp"
#include "Surge/Graphics/Renderer/Lights.hpp"
#include "Surge/ECS/Components.hpp"

#define FRAMES_IN_FLIGHT 3
#define BASE_SHADER_PATH "Engine/Assets/Shaders"

namespace Surge
{
    struct DrawCommand
    {
        DrawCommand(MeshComponent* meshComp, const glm::mat4& transform) : MeshComp(meshComp), Transform(transform) {}

        MeshComponent* MeshComp;
        glm::mat4 Transform;
    };

    struct RendererData
    {
        Ref<Framebuffer> OutputFrambuffer;
        Ref<RenderCommandBuffer> RenderCmdBuffer;
        Vector<DrawCommand> DrawList;
        Ref<GraphicsPipeline> mGeometryPipeline;
        Surge::ShaderSet ShaderSet;

        //Lights
        LightUniformBufferData LightData;
        Vector<PointLight> PointLights;

        // Camera
        glm::vec3 CameraPosition;
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

        void BeginFrame(const Camera& camera, const glm::mat4& transform);
        void BeginFrame(const EditorCamera& camera);
        virtual void EndFrame() = 0;

        FORCEINLINE void SubmitMesh(MeshComponent& meshComp, const glm::mat4& transform) { mData->DrawList.push_back(DrawCommand(&meshComp, transform)); }
        FORCEINLINE void SubmitPointLight(const PointLightComponent& pointLight, glm::vec3 position)
        {
            PointLight light;
            light.Position = position;
            light.Intensity = pointLight.Intensity;
            light.Radius = pointLight.Radius;
            light.Color = pointLight.Color;
            mData->PointLights.push_back(light);
        }

        virtual void BeginRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer, const Ref<Framebuffer>& framebuffer) = 0;
        virtual void EndRenderPass(const Ref<RenderCommandBuffer>& cmdBuffer) = 0;

        RendererData* GetData() { return mData.get(); }
        Ref<Shader>& GetShader(const String& name);
        Ref<Framebuffer>& GetFramebuffer(); //TODO REMOVE: Have something like FramebufferSet(similar to ShaderSet)

    protected:
        Scope<RendererData> mData;
    };
} // namespace Surge