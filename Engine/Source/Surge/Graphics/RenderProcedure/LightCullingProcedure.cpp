// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/RenderProcedure/LightCullingProcedure.hpp"
#include "PreDepthProcedure.hpp"
#include "GeometryProcedure.hpp"

namespace Surge
{
    void LightCullingProcedure::Init(RendererData* rendererData)
    {
        mRendererData = rendererData;
        Ref<Shader>& lightCullingShader = mRendererData->ShaderSet.GetShader("LightCulling");
        mProcData.LightCullingPipeline = ComputePipeline::Create(lightCullingShader);

        mProcData.LightListStorageBuffer = StorageBuffer::Create(1);
        mProcData.LightListDescriptorSet = DescriptorSet::Create(lightCullingShader, 5, false);
    }

    void LightCullingProcedure::Update()
    {
        const Ref<Image2D>& preDepthImage = Core::GetRenderer()->GetRenderProcManager()->GetRenderProcData<PreDepthProcedure>()->OutputFrambuffer->GetDepthAttachment();
        Ref<RenderCommandBuffer>& cmd = mRendererData->RenderCmdBuffer;
        mProcData.LightCullingPipeline->Bind(cmd);

        mProcData.LightListDescriptorSet->SetBuffer(mProcData.LightListStorageBuffer, 0);
        mProcData.LightListDescriptorSet->SetImage2D(preDepthImage, 1);
        mProcData.LightListDescriptorSet->UpdateForRendering();
        mProcData.LightListDescriptorSet->Bind(cmd, mProcData.LightCullingPipeline);

        mRendererData->DescriptorSet0->Bind(cmd, mProcData.LightCullingPipeline);

        mRendererData->LightData.CameraPosition = mRendererData->CameraPosition;
        mRendererData->LightData.PointLightCount = Uint(mRendererData->PointLights.size());
        for (Uint i = 0; i < mRendererData->LightData.PointLightCount; i++)
            mRendererData->LightData.PointLights[i] = mRendererData->PointLights[i];
        mRendererData->LightData.DirLight = mRendererData->DirLight;
        mRendererData->LightUniformBuffer->SetData(&mRendererData->LightData);
        mRendererData->LightDescriptorSet->SetBuffer(mRendererData->LightUniformBuffer, 0);
        mRendererData->LightDescriptorSet->UpdateForRendering();
        mRendererData->LightDescriptorSet->Bind(cmd, mProcData.LightCullingPipeline);

        mProcData.LightCullingPipeline->SetPushConstantData(cmd, "uScreenData", &mScreenSize);
        mProcData.LightCullingPipeline->Dispatch(cmd, mLightCullingWorkGroups.x, mLightCullingWorkGroups.y, mLightCullingWorkGroups.z);
    }

    void LightCullingProcedure::Resize(Uint newWidth, Uint newHeight)
    {
        mLightCullingWorkGroups = {(newWidth + newWidth % 16) / 16, (newHeight + newHeight % 16) / 16, 1};
        mProcData.TileCountX = mLightCullingWorkGroups.x;
        int newBufferSize = mLightCullingWorkGroups.x * mLightCullingWorkGroups.y * 4096;
        mProcData.LightListStorageBuffer->Resize(newBufferSize);
        mScreenSize = {newWidth, newHeight};
    }

    void LightCullingProcedure::Shutdown()
    {
    }

} // namespace Surge

SURGE_REFLECT_CLASS_REGISTER_BEGIN(Surge::LightCullingProcedure)
SURGE_REFLECT_CLASS_REGISTER_END(Surge::LightCullingProcedure)