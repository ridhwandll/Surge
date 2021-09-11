// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Graphics/Shader.hpp"

#define BASE_SHADER_PATH "Engine/Assets/Shaders/"

namespace Surge
{
    class RendererBase
    {
    private:
        RendererBase() = default;
        ~RendererBase() = default;

        void Initialize();
        void Shutdown();

        Ref<Shader>& GetShader(const String& name);
    private:
        Ref<Shader> mDummyShader = nullptr;
        Vector<Ref<Shader>> mAllShaders;
        // TODO: All constant buffers, and other stuff maybe
    
        friend class Renderer;
    };
}
