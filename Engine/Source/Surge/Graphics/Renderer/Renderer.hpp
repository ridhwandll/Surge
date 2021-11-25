// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"
#include "Surge/Graphics/Camera/EditorCamera.hpp"
#include "Surge/Graphics/Mesh.hpp"
#include "Surge/Graphics/Interface/RenderCommandBuffer.hpp"
#include "Surge/Graphics/Shader/Shader.hpp"
#include "Surge/Graphics/Shader/ShaderSet.hpp"
#include "Surge/Graphics/Interface/Texture.hpp"
#include "Surge/Graphics/Renderer/Lights.hpp"
#include "Surge/Graphics/Interface/DescriptorSet.hpp"
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
        Ref<UniformBuffer> LightUniformBuffer;
        Ref<DescriptorSet> LightDescriptorSet;
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

        void Initialize();
        void Shutdown();

        void BeginFrame(const Camera& camera, const glm::mat4& transform);
        void BeginFrame(const EditorCamera& camera);
        void EndFrame();

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

        RendererData* GetData() { return mData.get(); }
        Ref<Shader>& GetShader(const String& name);
        Ref<Framebuffer>& GetFramebuffer(); //TODO REMOVE: Have something like FramebufferSet(similar to ShaderSet)

    protected:
        Scope<RendererData> mData;
    };
} // namespace Surge