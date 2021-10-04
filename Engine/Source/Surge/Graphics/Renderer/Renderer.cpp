// Copyright (c) - SurgeTechnologies - All rights reserved
#include "Surge/Graphics/Renderer/Renderer.hpp"
#include "Surge/Utility/Filesystem.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace Surge
{
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
