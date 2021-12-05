// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/RenderProcedure/GeometryProcedure.hpp"
#include "ShadowMapProcedure.hpp"

namespace Surge
{
    void GeometryProcedure::Init(RendererData* rendererData)
    {
        mProcData = {};
        mRendererData = rendererData;

        FramebufferSpecification spec = {};
        spec.Formats = {ImageFormat::RGBA16F, ImageFormat::Depth32};
        spec.Width = 1280;
        spec.Height = 720;
        mProcData.OutputFrambuffer = Framebuffer::Create(spec);

        Ref<Shader> mainPBRShader = mRendererData->ShaderSet.GetShader("PBR");
        GraphicsPipelineSpecification pipelineSpec {};
        pipelineSpec.Shader = mainPBRShader;
        pipelineSpec.Topology = PrimitiveTopology::TriangleList;
        pipelineSpec.CullingMode = CullMode::Back;
        pipelineSpec.UseDepth = true;
        pipelineSpec.UseStencil = false;
        pipelineSpec.DebugName = "MeshPipeline";
        pipelineSpec.LineWidth = 1.0f;
        pipelineSpec.TargetFramebuffer = mProcData.OutputFrambuffer;
        mProcData.GeometryPipeline = GraphicsPipeline::Create(pipelineSpec);
    }

    void GeometryProcedure::Update()
    {
        SURGE_PROFILE_FUNC("GeometryProcedure::Update");

        // Light UBO data
        mRendererData->LightData.CameraPosition = mRendererData->CameraPosition;
        mRendererData->LightData.PointLightCount = Uint(mRendererData->PointLights.size());
        for (Uint i = 0; i < mRendererData->LightData.PointLightCount; i++)
            mRendererData->LightData.PointLights[i] = mRendererData->PointLights[i];
        mRendererData->LightData.DirLight = mRendererData->DirLight;
        mRendererData->LightUniformBuffer->SetData(&mRendererData->LightData);

        mRendererData->LightDescriptorSet->SetBuffer(mRendererData->LightUniformBuffer, 0);
        mRendererData->LightDescriptorSet->UpdateForRendering();
        mRendererData->LightDescriptorSet->Bind(mRendererData->RenderCmdBuffer, mProcData.GeometryPipeline);

        ShadowMapProcedure::InternalData* shadowProcData = Core::GetRenderer()->GetRenderProcManager()->GetRenderProcData<ShadowMapProcedure>();
        shadowProcData->ShadowDesciptorSet->SetBuffer(shadowProcData->ShadowUniformBuffer, 0);
        shadowProcData->ShadowDesciptorSet->UpdateForRendering();
        shadowProcData->ShadowDesciptorSet->Bind(mRendererData->RenderCmdBuffer, mProcData.GeometryPipeline);

        mProcData.OutputFrambuffer->BeginRenderPass(mRendererData->RenderCmdBuffer);
        mProcData.GeometryPipeline->Bind(mRendererData->RenderCmdBuffer);
        for (const DrawCommand& object : mRendererData->DrawList)
        {
            const Ref<Mesh>& mesh = object.MeshComp->Mesh;

            mesh->GetVertexBuffer()->Bind(mRendererData->RenderCmdBuffer);
            mesh->GetIndexBuffer()->Bind(mRendererData->RenderCmdBuffer);

            Vector<Ref<Material>>& materials = object.MeshComp->Mesh->GetMaterials();

            for (auto& mat : materials)
                mat->UpdateForRendering();

            const Submesh* submeshes = mesh->GetSubmeshes().data();
            for (Uint i = 0; i < mesh->GetSubmeshes().size(); i++)
            {
                const Submesh& submesh = submeshes[i];
                glm::mat4 meshData[2] = {object.Transform * submesh.Transform, mRendererData->ViewProjection};
                materials[submesh.MaterialIndex]->Bind(mRendererData->RenderCmdBuffer, mProcData.GeometryPipeline);

                mProcData.GeometryPipeline->SetPushConstantData(mRendererData->RenderCmdBuffer, "uMesh", meshData);
                mProcData.GeometryPipeline->DrawIndexed(mRendererData->RenderCmdBuffer, submesh.IndexCount, submesh.BaseIndex, submesh.BaseVertex);
            }
        }
        mProcData.OutputFrambuffer->EndRenderPass(mRendererData->RenderCmdBuffer);
    }

    void GeometryProcedure::Shutdown()
    {
    }

} // namespace Surge

// Empty Reflection, register nothing
SURGE_REFLECT_CLASS_REGISTER_BEGIN(Surge::GeometryProcedure)
SURGE_REFLECT_CLASS_REGISTER_END(Surge::GeometryProcedure)