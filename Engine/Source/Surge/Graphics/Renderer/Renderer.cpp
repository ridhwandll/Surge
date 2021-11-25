// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include "SurgeMath/Math.hpp"
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

        FramebufferSpecification spec = {};
        spec.Formats = {ImageFormat::RGBA8, ImageFormat::Depth32};
        spec.Width = 1280;
        spec.Height = 720;
        mData->OutputFrambuffer = Framebuffer::Create(spec);

        // Creating n descriptor pools for allocating descriptor sets by the application (n = frames in flight)

        // TODO: Come up with a better way of managing descriptor sets
        Ref<Shader> mainPBRShader = Core::GetRenderer()->GetShader("Simple");

        mData->LightDescriptorSet = DescriptorSet::Create(mainPBRShader, false);
        mData->LightUniformBuffer = UniformBuffer::Create(sizeof(LightUniformBufferData));

        // Geometry Pipeline - TODO: Move to Render Procedure API
        GraphicsPipelineSpecification pipelineSpec {};
        pipelineSpec.Shader = mainPBRShader;
        pipelineSpec.Topology = PrimitiveTopology::TriangleList;
        pipelineSpec.CullingMode = CullMode::Back;
        pipelineSpec.UseDepth = true;
        pipelineSpec.UseStencil = false;
        pipelineSpec.DebugName = "MeshPipeline";
        pipelineSpec.LineWidth = 1.0f;
        pipelineSpec.TargetFramebuffer = Core::GetRenderer()->GetFramebuffer();
        mData->mGeometryPipeline = GraphicsPipeline::Create(pipelineSpec);
    }

    void Renderer::Shutdown()
    {
        SURGE_PROFILE_FUNC("Renderer::Shutdown()");
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
        // ViewProjection and Transform
        glm::mat4 pushConstantData[2] = {};
        pushConstantData[0] = mData->ViewProjection;

        // Light UBO data
        mData->LightData.CameraPosition = mData->CameraPosition;
        mData->LightData.PointLightCount = Uint(mData->PointLights.size());
        for (Uint i = 0; i < mData->LightData.PointLightCount; i++)
            mData->LightData.PointLights[i] = mData->PointLights[i];

        mData->LightUniformBuffer->SetData(&mData->LightData);
        mData->LightDescriptorSet->Update(mData->LightUniformBuffer);
        mData->LightDescriptorSet->Bind(mData->RenderCmdBuffer, mData->mGeometryPipeline);

        mData->OutputFrambuffer->BeginRenderPass(mData->RenderCmdBuffer);
        for (const DrawCommand& object : mData->DrawList)
        {
            const Ref<Mesh>& mesh = object.MeshComp->Mesh;
            mData->mGeometryPipeline->Bind(mData->RenderCmdBuffer);

            mesh->GetVertexBuffer()->Bind(mData->RenderCmdBuffer);
            mesh->GetIndexBuffer()->Bind(mData->RenderCmdBuffer);

            object.MeshComp->Material->Bind(mData->RenderCmdBuffer, mData->mGeometryPipeline);

            const Submesh* submeshes = mesh->GetSubmeshes().data();
            for (Uint i = 0; i < mesh->GetSubmeshes().size(); i++)
            {
                const Submesh& submesh = submeshes[i];
                pushConstantData[1] = object.Transform * submesh.Transform;
                mData->mGeometryPipeline->SetPushConstantData(mData->RenderCmdBuffer, "uFrameData", &pushConstantData);
                mData->mGeometryPipeline->DrawIndexed(mData->RenderCmdBuffer, submesh.IndexCount, submesh.BaseIndex, submesh.BaseVertex);
            }
        }
        mData->OutputFrambuffer->EndRenderPass(mData->RenderCmdBuffer);

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

    Ref<Framebuffer>& Renderer::GetFramebuffer()
    {
        return mData->OutputFrambuffer;
    }
} // namespace Surge