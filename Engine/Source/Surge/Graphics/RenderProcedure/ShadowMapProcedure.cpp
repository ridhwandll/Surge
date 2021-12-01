// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/RenderProcedure/ShadowMapProcedure.hpp"

namespace Surge
{
    void ShadowMapProcedure::Init(RendererData* rendererData)
    {
        mRendererData = rendererData;
    }

    void ShadowMapProcedure::Update()
    {
        SURGE_PROFILE_FUNC("ShadowMapProcedure::Update");
    }

    void ShadowMapProcedure::Shutdown()
    {
    }

} // namespace Surge

// Empty Reflection, register nothing
SURGE_REFLECT_CLASS_REGISTER_BEGIN(Surge::ShadowMapProcedure)
SURGE_REFLECT_CLASS_REGISTER_END(Surge::ShadowMapProcedure)