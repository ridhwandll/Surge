// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/RenderProcedure/GeometryProcedure.hpp"

namespace Surge
{
    void GeometryProcedure::Init(RendererData* rendererData)
    {
        mProcData = {};
        mRendererData = rendererData;

        FramebufferSpecification spec = {};
        spec.Formats = {ImageFormat::RGBA8, ImageFormat::Depth32};
        spec.Width = 1280;
        spec.Height = 720;
        mProcData.OutputFrambuffer = Framebuffer::Create(spec);

        Ref<Shader> mainPBRShader = mRendererData->ShaderSet.GetShader("Simple");
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
        // ViewProjection and Transform
        glm::mat4 pushConstantData[2] = {};
        pushConstantData[0] = mRendererData->ViewProjection;

        // Light UBO data
        mRendererData->LightData.CameraPosition = mRendererData->CameraPosition;
        mRendererData->LightData.PointLightCount = Uint(mRendererData->PointLights.size());
        for (Uint i = 0; i < mRendererData->LightData.PointLightCount; i++)
            mRendererData->LightData.PointLights[i] = mRendererData->PointLights[i];

        mRendererData->LightUniformBuffer->SetData(&mRendererData->LightData);
        mRendererData->LightDescriptorSet->Update(mRendererData->LightUniformBuffer);
        mRendererData->LightDescriptorSet->Bind(mRendererData->RenderCmdBuffer, mProcData.GeometryPipeline);

        mProcData.OutputFrambuffer->BeginRenderPass(mRendererData->RenderCmdBuffer);
        for (const DrawCommand& object : mRendererData->DrawList)
        {
            const Ref<Mesh>& mesh = object.MeshComp->Mesh;
            mProcData.GeometryPipeline->Bind(mRendererData->RenderCmdBuffer);

            mesh->GetVertexBuffer()->Bind(mRendererData->RenderCmdBuffer);
            mesh->GetIndexBuffer()->Bind(mRendererData->RenderCmdBuffer);

            Vector<Ref<Material>>& materials = object.MeshComp->Mesh->GetMaterials();
            const Submesh* submeshes = mesh->GetSubmeshes().data();
            for (Uint i = 0; i < mesh->GetSubmeshes().size(); i++)
            {
                const Submesh& submesh = submeshes[i];
                pushConstantData[1] = object.Transform * submesh.Transform;

                materials[submesh.MaterialIndex]->UpdateForRendering();
                materials[submesh.MaterialIndex]->Bind(mRendererData->RenderCmdBuffer, mProcData.GeometryPipeline);

                mProcData.GeometryPipeline->SetPushConstantData(mRendererData->RenderCmdBuffer, "uFrameData", &pushConstantData);
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