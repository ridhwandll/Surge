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
#include "Surge/Graphics/RenderProcedure/RenderProcedureManager.hpp"
#include "Surge/ECS/Components.hpp"

#define FRAMES_IN_FLIGHT 3
#define BASE_SHADER_PATH "Engine/Assets/Shaders" //Sadkek, we don't have an asset manager yet

namespace Surge
{
    struct DrawCommand
    {
        DrawCommand(MeshComponent* meshComp, const glm::mat4& transform) : MeshComp(meshComp), Transform(transform) {}

        MeshComponent* MeshComp;
        glm::mat4 Transform;
    };

    class SURGE_API Scene;
    struct RendererData
    {
        Ref<RenderCommandBuffer> RenderCmdBuffer;
        Vector<DrawCommand> DrawList;
        Surge::ShaderSet ShaderSet;

        Ref<UniformBuffer> CameraUniformBuffer;
        Ref<UniformBuffer> RendererDataUniformBuffer;
        Ref<DescriptorSet> DescriptorSet0;

        Ref<Texture2D> WhiteTexture;
        Scene* SceneContext;

        // Lights
        LightUniformBufferData LightData;
        Ref<UniformBuffer> LightUniformBuffer;
        Vector<PointLight> PointLights;
        DirectionalLight DirLight;

        // Camera
        glm::vec3 CameraPosition;
        glm::mat4 ViewMatrix;
        glm::mat4 ProjectionMatrix;
        glm::mat4 ViewProjection;
    };

    class SURGE_API Renderer
    {
    public:
        Renderer() = default;
        ~Renderer() = default;

        void Initialize();
        void Shutdown();

        void BeginFrame(const Camera& camera, const glm::mat4& transform);
        void BeginFrame(const EditorCamera& camera);
        void EndFrame();
        void SetRenderArea(Uint width, Uint height);

        FORCEINLINE void SubmitMesh(MeshComponent& meshComp, const glm::mat4& transform) { mData->DrawList.push_back(DrawCommand(&meshComp, transform)); }
        FORCEINLINE void SubmitPointLight(const PointLightComponent& pointLight, const glm::vec3& position)
        {
            PointLight light;
            light.Position = position;
            light.Intensity = pointLight.Intensity;
            light.Radius = pointLight.Radius;
            light.Color = pointLight.Color;
            light.Falloff = pointLight.Falloff;
            mData->PointLights.push_back(light);
        }
        FORCEINLINE void SubmitDirectionalLight(const DirectionalLightComponent& dirLight, const glm::vec3& direction)
        {
            DirectionalLight light;
            light.Direction = direction;
            light.Intensity = dirLight.Intensity;
            light.Color = dirLight.Color;
            light.Size = dirLight.Size;
            mData->DirLight = light;
        }

        RenderProcedureManager* GetRenderProcManager() { return &mProcManager; }
        RendererData* GetData() { return mData.get(); }
        Ref<Shader>& GetShader(const String& name);
        Ref<Framebuffer>& GetFinalPassFramebuffer(); //TODO REMOVE: Have something like FramebufferSet(similar to ShaderSet)
        void SetSceneContext(Ref<Scene>& scene) { mData->SceneContext = scene.Raw(); }

    private:
        RenderProcedureManager mProcManager;
        Scope<RendererData> mData;
    };
} // namespace Surge