// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include "SurgeMath/Math.hpp"
#include "Surge/Graphics/RenderProcedure/GeometryProcedure.hpp"
#include "Surge/Graphics/RenderProcedure/ShadowMapProcedure.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace Surge
{
    void Renderer::Initialize()
    {
        SURGE_PROFILE_FUNC("Renderer::Initialize()");
        mData = CreateScope<RendererData>();
        mData->RenderCmdBuffer = RenderCommandBuffer::Create(false);
        mData->ShaderSet.Initialize(BASE_SHADER_PATH);
        mData->ShaderSet.AddShader("Simple.glsl");
        mData->ShaderSet.LoadAll();

        Ref<Shader> mainPBRShader = Core::GetRenderer()->GetShader("Simple");
        mData->LightDescriptorSet = DescriptorSet::Create(mainPBRShader, false);
        mData->LightUniformBuffer = UniformBuffer::Create(sizeof(LightUniformBufferData));

        Uint whiteTextureData = 0xffffffff;
        mData->WhiteTexture = Texture2D::Create(ImageFormat::RGBA8, 1, 1, &whiteTextureData);

        mProcManager.Init(mData);
        mProcManager.AddProcedure<ShadowMapProcedure>();
        mProcManager.AddProcedure<GeometryProcedure>();
        mProcManager.Sort<ShadowMapProcedure, GeometryProcedure>();
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
        mData->ViewMatrix = glm::inverse(transform);
        mData->ProjectionMatrix = camera.GetProjectionMatrix();
        mData->ViewProjection = mData->ProjectionMatrix * mData->ViewMatrix;

        glm::vec3 translation, rotation, scale;
        Math::DecomposeTransform(transform, translation, rotation, scale);
        mData->CameraPosition = translation;
    }

    void Renderer::BeginFrame(const EditorCamera& camera)
    {
        SURGE_PROFILE_FUNC("Renderer::BeginFrame(EditorCamera)");
        mData->ViewMatrix = camera.GetViewMatrix();
        mData->ProjectionMatrix = camera.GetProjectionMatrix();
        mData->ViewProjection = mData->ProjectionMatrix * mData->ViewMatrix;
        mData->CameraPosition = camera.GetPosition();
    }

    void Renderer::EndFrame()
    {
        SURGE_PROFILE_FUNC("Renderer::EndFrame()");

        mData->RenderCmdBuffer->BeginRecording();
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
