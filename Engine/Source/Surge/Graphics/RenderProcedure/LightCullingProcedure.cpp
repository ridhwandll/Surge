// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/RenderProcedure/LightCullingProcedure.hpp"

namespace Surge
{
    void LightCullingProcedure::Init(RendererData* rendererData)
    {
        mRendererData = rendererData;
        Ref<Shader>& lightCullingShader = mRendererData->ShaderSet.GetShader("LightCulling");
        mProcData.LightCullingPipeline = ComputePipeline::Create(lightCullingShader);

        mProcData.LightListStorageBuffer = StorageBuffer::Create(16);
        mProcData.LightListDescriptorSet = DescriptorSet::Create(lightCullingShader, 5, false);
    }

    void LightCullingProcedure::Update()
    {
        mProcData.LightCullingPipeline->Bind(mRendererData->RenderCmdBuffer);
        mProcData.LightListDescriptorSet->SetBuffer(mProcData.LightListStorageBuffer, 0);
        mProcData.LightListDescriptorSet->UpdateForRendering();
        mProcData.LightListDescriptorSet->Bind(mRendererData->RenderCmdBuffer, mProcData.LightCullingPipeline);
        mProcData.LightCullingPipeline->Dispatch(mRendererData->RenderCmdBuffer, 16, 16, 1);
    }

    void LightCullingProcedure::Shutdown()
    {
    }

} // namespace Surge

SURGE_REFLECT_CLASS_REGISTER_BEGIN(Surge::LightCullingProcedure)
SURGE_REFLECT_CLASS_REGISTER_END(Surge::LightCullingProcedure)