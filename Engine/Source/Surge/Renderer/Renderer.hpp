// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once

#include "Surge/Renderer/Device.h"

namespace Surge
{
    struct RendererData
    {
        Ref<Device> device;
    };

    class SURGE_API Renderer
    {
    public:
        static void Init();
        static void Shutdown();

        static RendererData GetData() { return sData; }
    private:
        static RendererData sData;
    };
}