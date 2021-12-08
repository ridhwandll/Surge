// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include "SurgeMath/Math.hpp"
#include "Surge/ECS/Scene.hpp"
#include "Surge/Graphics/RenderProcedure/PreDepthProcedure.hpp"
#include "Surge/Graphics/RenderProcedure/ShadowMapProcedure.hpp"
#include "Surge/Graphics/RenderProcedure/GeometryProcedure.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace Surge
{
    void Renderer::Initialize()
    {
        SURGE_PROFILE_FUNC("Renderer::Initialize()");
        mData = CreateScope<RendererData>();
        mData->RenderCmdBuffer = RenderCommandBuffer::Create(false);
        mData->ShaderSet.Initialize(BASE_SHADER_PATH);
        mData->ShaderSet.AddShader("PBR.glsl");
        mData->ShaderSet.AddShader("ShadowMap.glsl");
        mData->ShaderSet.AddShader("PreDepth.glsl");
        mData->ShaderSet.LoadAll();

        Ref<Shader> mainPBRShader = Core::GetRenderer()->GetShader("PBR");
        mData->LightDescriptorSet = DescriptorSet::Create(mainPBRShader, 4, false);
        mData->LightUniformBuffer = UniformBuffer::Create(sizeof(LightUniformBufferData));

        mData->CameraUniformBuffer = UniformBuffer::Create(sizeof(glm::mat4) * 3);
        mData->CameraDescriptorSet = DescriptorSet::Create(mainPBRShader, 0, false);

        Uint whiteTextureData = 0xffffffff;
        mData->WhiteTexture = Texture2D::Create(ImageFormat::RGBA8, 1, 1, &whiteTextureData);

        mProcManager.Init(mData);
        mProcManager.AddProcedure<PreDepthProcedure>();
        mProcManager.AddProcedure<ShadowMapProcedure>();
        mProcManager.AddProcedure<GeometryProcedure>();
        mProcManager.Sort<PreDepthProcedure, ShadowMapProcedure, GeometryProcedure>();
    }

    void Renderer::Shutdown()
    {
        SURGE_PROFILE_FUNC("Renderer::Shutdown()");
        mProcManager.Shutdown();
        mData->ShaderSet.Shutdown();
        mData.reset();
    }

    void Renderer::BeginFrame(const Camera& camera, const glm::mat4& transform)
    {
        SURGE_PROFILE_FUNC("Renderer::BeginFrame(Camera)");
        glm::vec3 translation, rotation, scale;
        Math::DecomposeTransform(transform, translation, rotation, scale);

        mData->ViewMatrix = glm::inverse(transform);
        mData->ProjectionMatrix = camera.GetProjectionMatrix();
        mData->ViewProjection = mData->ProjectionMatrix * mData->ViewMatrix;
        mData->CameraPosition = translation;
        mData->RenderCmdBuffer->BeginRecording();

        GeometryProcedure::InternalData* geometryProcData = Core::GetRenderer()->GetRenderProcManager()->GetRenderProcData<GeometryProcedure>();
        glm::mat4 cameraData[2] = {mData->ViewMatrix, mData->ProjectionMatrix};
        mData->CameraUniformBuffer->SetData(cameraData);
        mData->CameraDescriptorSet->SetBuffer(mData->CameraUniformBuffer, 0);
        mData->CameraDescriptorSet->UpdateForRendering();
        mData->CameraDescriptorSet->Bind(mData->RenderCmdBuffer, geometryProcData->GeometryPipeline);
    }

    void Renderer::BeginFrame(const EditorCamera& camera)
    {
        SURGE_PROFILE_FUNC("Renderer::BeginFrame(EditorCamera)");
        mData->ViewMatrix = camera.GetViewMatrix();
        mData->ProjectionMatrix = camera.GetProjectionMatrix();
        mData->ViewProjection = mData->ProjectionMatrix * mData->ViewMatrix;
        mData->CameraPosition = camera.GetPosition();
        mData->RenderCmdBuffer->BeginRecording();

        GeometryProcedure::InternalData* geometryProcData = Core::GetRenderer()->GetRenderProcManager()->GetRenderProcData<GeometryProcedure>();
        glm::mat4 cameraData[2] = {mData->ViewMatrix, mData->ProjectionMatrix};
        mData->CameraUniformBuffer->SetData(cameraData);
        mData->CameraDescriptorSet->SetBuffer(mData->CameraUniformBuffer, 0);
        mData->CameraDescriptorSet->UpdateForRendering();
        mData->CameraDescriptorSet->Bind(mData->RenderCmdBuffer, geometryProcData->GeometryPipeline);
    }

    void Renderer::EndFrame()
    {
        SURGE_PROFILE_FUNC("Renderer::EndFrame()");

        mProcManager.UpdateAll();
        mData->RenderCmdBuffer->EndRecording();

        mData->RenderCmdBuffer->Submit();

        mData->DrawList.clear();
        mData->PointLights.clear();
    }

    Ref<Shader>& Renderer::GetShader(const String& name)
    {
        Ref<Shader>& result = mData->ShaderSet.GetShader(name);
        return result;
    }

    Ref<Framebuffer>& Renderer::GetFinalPassFramebuffer()
    {
        GeometryProcedure::InternalData* passData = mProcManager.GetRenderProcData<GeometryProcedure>();
        return passData->OutputFrambuffer;
    }

} // namespace Surge
