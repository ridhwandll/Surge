// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Memory.hpp"

namespace Surge
{
    class SURGE_API Renderer : public RefCounted
    {
    public:
        void Initialize();
        void Shutdown();
    };
}
