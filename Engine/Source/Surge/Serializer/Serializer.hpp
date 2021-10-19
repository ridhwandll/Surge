// Copyright (c) - SurgeTechnologies - All rights reserved
#pragma once
#include "Surge/Core/Defines.hpp"
#include "Surge/ECS/Scene.hpp"

namespace Surge::Serializer
{
    template <typename T>
    void Serialize(const Path& path, T* out)
    {
        static_assert(false);
    }

    template <>
    void Serialize(const Path& path, Scene* out);

} // namespace Surge::Serializer